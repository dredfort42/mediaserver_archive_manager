#include "cameraClient.hpp"

CameraClient::CameraClient(volatile sig_atomic_t *isInterrupted, std::string cameraUUID, std::string cameraUrl)
{
    _isInterrupted = isInterrupted;
    _streamUUID = cameraUUID;
    _cameraUrl = cameraUrl;
    _connectionParameters = nullptr;
    _videoStreamIndex = -1;
    _audioStreamIndex = -1;
    _dataStreamIndex = -1;
    _subtitleStreamIndex = -1;
    _attachmentStreamIndex = -1;
    _codec = nullptr;

    lastFrameT = 0;
    codecContext = nullptr;
    avioContext = nullptr;
    formatContext = avformat_alloc_context();
    nbStreams = 0;
    if (!formatContext)
        print(LogType::ERROR, _streamUUID + " | Can't  allocate AVFormatContext");
}

CameraClient::~CameraClient()
{
    if (codecContext)
        avcodec_free_context(&codecContext);

    if (formatContext)
    {
        avformat_close_input(&formatContext);
        // avformat_close_input already frees formatContext, no need to call avformat_free_context
    }

    if (avioContext)
        avio_context_free(&avioContext);

    if (_connectionParameters)
        av_dict_free(&_connectionParameters);

    if (_codec)
        delete[] _codec;
}

int CameraClient::setConnectionParameter(const char *key, const char *value)
{
    if (av_dict_set(&_connectionParameters, key, value, 0) < 0)
    {
        print(LogType::ERROR, _streamUUID + " | Error setting connection parameters [ key: " + std::string(key) + " value: " + std::string(value) + " ]");
        return -1;
    }
    return 0;
}

void CameraClient::connectToCamera(int timeout)
{
    if (!formatContext)
    {
        print(LogType::ERROR, _streamUUID + " | formatContext is null, cannot connect");
        return;
    }

    int attemps = 0;
    bool success = false;

    while (!success && !*_isInterrupted)
    {
        if (attemps > 0)
            sleep(timeout);

        print(LogType::DEBUGER, "[Attempt: " + std::to_string(attemps + 1) + "] Connecting to camera: " + _cameraUrl);

        if (avformat_open_input(&formatContext, _cameraUrl.c_str(), nullptr, &_connectionParameters) != 0)
        {
            print(LogType::ERROR, _streamUUID + " | Can't open the input stream");
            attemps++;
            continue;
        }

        if (avformat_find_stream_info(formatContext, nullptr) < 0)
        {
            print(LogType::ERROR, _streamUUID + " | Can't find the stream information");
            avformat_close_input(&formatContext);
            attemps++;
            continue;
        }

        nbStreams = formatContext->nb_streams;

        for (unsigned int i = 0; i < nbStreams; i++)
        {
            switch (formatContext->streams[i]->codecpar->codec_type)
            {
            case AVMEDIA_TYPE_VIDEO:
                _videoStreamIndex = i;
                break;
            case AVMEDIA_TYPE_AUDIO:
                _audioStreamIndex = i;
                break;
            case AVMEDIA_TYPE_DATA:
                _dataStreamIndex = i;
                break;
            case AVMEDIA_TYPE_SUBTITLE:
                _subtitleStreamIndex = i;
                break;
            case AVMEDIA_TYPE_ATTACHMENT:
                _attachmentStreamIndex = i;
                break;

            default:
                print(LogType::ERROR, _streamUUID + " | Unknown Media Type with stream index: " + std::to_string(i));
                break;
            }
        }

        if (_videoStreamIndex == -1)
        {
            print(LogType::ERROR, _streamUUID + " | Can't find the video stream");
            avformat_close_input(&formatContext);
            attemps++;
            continue;
        }

        _codec = new Codec[formatContext->nb_streams];
        print(LogType::DEBUGER, "Connected to camera: " + _cameraUrl);

        success = true;
    }
}

int CameraClient::getVideoStreamIndex()
{
    return _videoStreamIndex;
}

int CameraClient::getAudioStreamIndex()
{
    return _audioStreamIndex;
}

int CameraClient::getDataStreamIndex()
{
    return _dataStreamIndex;
}

int CameraClient::getSubtitleStreamIndex()
{
    return _subtitleStreamIndex;
}

int CameraClient::getAttachmentStreamIndex()
{
    return _attachmentStreamIndex;
}

std::string CameraClient::getStreamUUID()
{
    return _streamUUID;
}

Codec CameraClient::getCodec(int streamIndex)
{
    if (*_isInterrupted == 0)
        _defineCodecContext(streamIndex);

    return _codec[streamIndex];
}

void CameraClient::_defineCodecContext(int streamIndex)
{
    if (streamIndex < 0 || streamIndex >= (int)nbStreams)
    {
        print(LogType::ERROR, _streamUUID + " | Invalid stream index: " + std::to_string(streamIndex));
        return;
    }

    // Check if codec for this stream was already defined
    if (_codec[streamIndex].codecType != -1)
    {
        return; // Already defined, skip to avoid memory leak
    }

    AVCodecParameters *codecParameters = formatContext->streams[streamIndex]->codecpar;
    const AVCodec *codec = avcodec_find_decoder(codecParameters->codec_id);

    if (!codec)
    {
        print(LogType::ERROR, _streamUUID + " | Can't find the codec");
        return;
    }

    AVCodecContext *tempCodecContext = avcodec_alloc_context3(codec);

    if (!tempCodecContext)
    {
        print(LogType::ERROR, _streamUUID + " | Can't allocate the codec context");
        return;
    }

    if (avcodec_parameters_to_context(tempCodecContext, codecParameters) < 0)
    {
        print(LogType::ERROR, _streamUUID + " | Can't copy codec parameters to codec context");
        avcodec_free_context(&tempCodecContext);
        return;
    }

    if (avcodec_open2(tempCodecContext, codec, nullptr) < 0)
    {
        print(LogType::ERROR, _streamUUID + " | Can't open the codec");
        avcodec_free_context(&tempCodecContext);
        return;
    }

    Codec &codecInfo = _codec[streamIndex];
    codecInfo.codecType = codecParameters->codec_type;
    codecInfo.codecId = codecParameters->codec_id;
    codecInfo.codecTag = codecParameters->codec_tag;
    codecInfo.extradata = codecParameters->extradata;
    codecInfo.extradataSize = codecParameters->extradata_size;
    codecInfo.format = codecParameters->format;
    const char *formatName = av_get_pix_fmt_name((AVPixelFormat)codecParameters->format);
    codecInfo.formatName = formatName ? std::string(formatName) : "unknown";
    codecInfo.bitRate = codecParameters->bit_rate;
    codecInfo.bitsPerCodedSample = codecParameters->bits_per_coded_sample;
    codecInfo.bitsPerRawSample = codecParameters->bits_per_raw_sample;
    codecInfo.profile = codecParameters->profile;
    codecInfo.level = codecParameters->level;
    codecInfo.width = codecParameters->width;
    codecInfo.height = codecParameters->height;
    codecInfo.sampleAspectRatioNum = codecParameters->sample_aspect_ratio.num;
    codecInfo.sampleAspectRatioDen = codecParameters->sample_aspect_ratio.den;
    codecInfo.framerateNum = tempCodecContext->framerate.num;
    codecInfo.framerateDen = tempCodecContext->framerate.den;
    codecInfo.fieldOrder = codecParameters->field_order;
    codecInfo.colorRange = codecParameters->color_range;
    codecInfo.colorPrimaries = codecParameters->color_primaries;
    codecInfo.colorTransfer = codecParameters->color_trc;
    codecInfo.colorSpace = codecParameters->color_space;
    codecInfo.chromaLocation = codecParameters->chroma_location;
    codecInfo.videoDelay = codecParameters->video_delay;
    codecInfo.sampleRate = codecParameters->sample_rate;
    codecInfo.blockAlign = codecParameters->block_align;
    codecInfo.frameSize = codecParameters->frame_size;
    codecInfo.initialPadding = codecParameters->initial_padding;
    codecInfo.trailingPadding = codecParameters->trailing_padding;
    codecInfo.seekPreroll = codecParameters->seek_preroll;

    // Free the temporary codec context as we only needed it for extracting parameters
    avcodec_free_context(&tempCodecContext);
}
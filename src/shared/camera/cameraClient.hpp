#ifndef CAMERA_CLIENT_HPP
#define CAMERA_CLIENT_HPP

#include <csignal>
#include <chrono>
#include <iostream>
#include <string>
#include <unistd.h>

#include "logPrinter.hpp"

extern "C"
{
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

/**
 * @brief The codec struct
 */
struct Codec
{
    int codecType = -1;
    int codecId;
    uint32_t codecTag;
    uint8_t *extradata;
    int extradataSize;
    int format;
    std::string formatName;
    int64_t bitRate;
    int bitsPerCodedSample;
    int bitsPerRawSample;
    int profile;
    int level;
    int width;
    int height;
    int sampleAspectRatioNum;
    int sampleAspectRatioDen;
    int framerateNum;
    int framerateDen;
    int fieldOrder;
    int colorRange;
    int colorPrimaries;
    int colorTransfer;
    int colorSpace;
    int chromaLocation;
    int videoDelay;
    int chLayoutOrder;
    int chLayoutNbChannels;
    int sampleRate;
    int blockAlign;
    int frameSize;
    int initialPadding;
    int trailingPadding;
    int seekPreroll;
};

/**
 * @brief The camera client class
 */
class CameraClient
{
private:
    volatile sig_atomic_t *_isInterrupted;
    std::string _streamUUID;
    std::string _cameraUrl;
    AVDictionary *_connectionParameters;
    int _videoStreamIndex;
    int _audioStreamIndex;
    int _dataStreamIndex;
    int _subtitleStreamIndex;
    int _attachmentStreamIndex;
    Codec *_codec;

    CameraClient();

    /**
     * @brief Define codec context
     */
    void _defineCodecContext(int streamIndex);

public:
    int64_t lastFrameT;
    AVCodecContext *codecContext;
    AVIOContext *avioContext;
    AVFormatContext *formatContext;
    uint nbStreams; // Number of AVMEDIA_TYPE_XXX streams

    CameraClient(volatile sig_atomic_t *isInterrupted, std::string cameraUUID, std::string cameraUrl);

    ~CameraClient();

    /**
     * @brief Set connection parameter
     * @param key Parameter key
     * @param value Parameter value
     * @return 0 if success, -1 if error
     */
    int setConnectionParameter(const char *key, const char *value);

    /**
     * @brief Connect to camera
     * @param timeout Timeout before trying to connect again
     */
    void connectToCamera(int timeout);

    /**
     * @brief Get camera CameraUUID
     * @return Stream CameraUUID
     */
    std::string getStreamUUID();

    /**
     * @brief Get camera URL
     * @return Stream URL
     */
    int getVideoStreamIndex();

    /**
     * @brief Get audio stream index
     * @return Audio stream index
     */
    int getAudioStreamIndex();

    /**
     * @brief Get data stream index
     * @return Data stream index
     */
    int getDataStreamIndex();

    /**
     * @brief Get subtitle stream index
     * @return Subtitle stream index
     */
    int getSubtitleStreamIndex();

    /**
     * @brief Get attachment stream index
     * @return Attachment stream index
     */
    int getAttachmentStreamIndex();

    /**
     * @brief Get codec information
     * @param streamIndex Stream index
     * @return Codec
     */
    Codec getCodec(int streamIndex);
};

#endif // CAMERA_CLIENT_HPP
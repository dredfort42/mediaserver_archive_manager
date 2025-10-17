#include "recorder.hpp"

static volatile sig_atomic_t isInterrupted = 0;
std::string logMessage = "";

static void sigterm(int sig)
{
    if (!isInterrupted)
        print(LogType::WARNING, "WRITER SIGTERM:\t" + std::to_string(sig) + " | PID:\t" + std::to_string(getpid()));
    isInterrupted = 1;
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        print(LogType::ERROR, "Incorrect startup parameters!");
        print(LogType::ERROR, "Usage: ./archive_recorder <streamUUID> <--config=/path/to/my_config.ini>");
        return RTN_ERROR;
    }

    std::string streamUUID = argv[1];

    logMessage = "RECORDER WITH PID " + std::to_string(getpid()) + " FOR STREAM WITH ID " + streamUUID + " ";

    print(LogType::INFO, logMessage + "STARTING");

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    signal(SIGINT, sigterm);
    signal(SIGTERM, sigterm);

    print(LogType::DEBUGER, logMessage + "INITIALIZING");

    ConfigMap config(argc, argv);

    RecorderConfig recorderConfig = getRecorderConfig(&config);
    MessengerConfig messengerConfig = getMessengerConfig(&config);

    openlog(recorderConfig.appName.c_str(), LOG_PID | LOG_CONS, LOG_USER);

    LogTable table("/// RECORDER STARTED");
    table.addRow("PID", std::to_string(getpid()));
    table.addRow("Stream UUID", streamUUID);
    table.addRow("Storage path", recorderConfig.storagePath);
    table.addRow("Fragment length (seconds)", std::to_string(recorderConfig.fragmentLengthInSeconds));
    table.printLogTable(LogType::DEBUGER);

    Messenger messenger(&isInterrupted);
    Messenger::topics_t topics;
    // AVPacketBuffer avPackets;

    std::string err = initMessenger(messenger, messengerConfig, recorderConfig.appUUID);
    if (!err.empty())
    {
        print(LogType::ERROR, err);
        return RTN_ERROR;
    }

    std::list<Messenger::packet_t> avPackets;
    std::thread thConsumer(consumeAVPackets,
                           &messenger,
                           &avPackets,
                           &streamUUID);

    // int configurationError = 0;

    // configurationError += messenger.setConfigurationProperty(Messenger::ClientType::BOTH, "metadata.broker.list", config.getProperty("kafka.host") + ":" + config.getProperty("kafka.port"));
    // configurationError += messenger.setConfigurationProperty(Messenger::ClientType::BOTH, "client.id", "archive-writer");
    // configurationError += messenger.setConfigurationProperty(Messenger::ClientType::PRODUCER, "acks", config.getProperty("kafka.acks"));
    // configurationError += messenger.setConfigurationProperty(Messenger::ClientType::CONSUMER, "group.id", "archive-writers");
    // configurationError += messenger.setConfigurationProperty(Messenger::ClientType::CONSUMER, "enable.auto.commit", "false");
    // configurationError += messenger.setConfigurationProperty(Messenger::ClientType::CONSUMER, "auto.offset.reset", "latest");
    // configurationError += messenger.setConfigurationProperty(Messenger::ClientType::CONSUMER, "enable.partition.eof", "true");
    // configurationError += messenger.setConfigurationProperty(Messenger::ClientType::CONSUMER, "enable.auto.offset.store", "true");

    // if (configurationError)
    // {
    //     print(LogType::ERROR, std::to_string(getpid()) + "System configuration error");
    //     return -1;
    // }

    // messenger.createMessenger();
    // print(LogType::INFO, "Service created");

    // std::thread thConsumer(startConsumer,
    //                        &messenger,
    //                        &packets,
    //                        cameraUUID);

    std::string cameraUUID = streamUUID.substr(0, streamUUID.find('_'));

    std::thread thFileWriter(writeAVPacketsToFile,
                             &messenger,
                             &messengerConfig.topicIFrameByteOffsets,
                             &cameraUUID,
                             &avPackets,
                             &recorderConfig.storagePath,
                             &recorderConfig.fragmentLengthInSeconds);

    // -----
    while (!isInterrupted)
    {
        print(LogType::DEBUGER, "Recorder for " + cameraUUID + " has AVPacket queue size: " + std::to_string(avPackets.size()));
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    // thConsumer.join();

    thConsumer.join();
    thFileWriter.join();
    // thAVPacket.join();

    google::protobuf::ShutdownProtobufLibrary();
    closelog();

    print(LogType::WARNING, "Writer process with pid " + std::to_string(getpid()) + " stopped successfully");
    return 0;
}
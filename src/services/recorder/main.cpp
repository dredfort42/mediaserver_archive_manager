#include "recorder.hpp"

static volatile sig_atomic_t isInterrupted = 0;

static void sigterm(int sig)
{
    if (!isInterrupted)
        print(LogType::WARNING, "WRITER SIGTERM:\t" + std::to_string(sig) + " | PID:\t" + std::to_string(getpid()));
    isInterrupted = 1;
}

int main(int argc, char **argv)
{
    if (argc < 4)
    {
        print(LogType::ERROR, "Incorrect startup parameters!");
        print(LogType::ERROR, "Usage: ./writer <cameraUUID> <storagePath> <fragmentLengthInSeconds>");
        return 1;
    }
    else if (!std::filesystem::is_directory(argv[2]))
    {
        print(LogType::ERROR, "Incorrect startup parameters!");
        print(LogType::ERROR, "Usage: ./recorder <cameraUUID> <storagePath> <fragmentLengthInSeconds>");
        print(LogType::ERROR, "ATTENTION! <storagePath> must be a path to an existing directory");
        return 1;
    }

    std::string cameraUUID = argv[1];
    std::string storagePath = argv[2];
    int fragmentLengthInSeconds = std::stoi(argv[3]);

    storagePath += "/" + cameraUUID;

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    signal(SIGINT, sigterm);
    signal(SIGTERM, sigterm);

    // if (DEBUG)
    // {
    LogTable table("/// RECORDER STARTED");
    table.addRow("Stream UUID", cameraUUID);
    table.addRow("Storage path", storagePath);
    table.addRow("Fragment length", std::to_string(fragmentLengthInSeconds));
    table.printLogTable(LogType::DEBUGER);
    // }

    while (!isInterrupted)
        sleep(1);
    // Messenger messenger(&isInterrupted);
    // ConfigMap config;
    // std::list<Messenger::packet_t> packets;

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

    // std::thread thFileWriter(fileWriter,
    //                          &messenger,
    //                          config.getProperty("kafka.topic.compact.clean.hour.fileOffsets"),
    //                          cameraUUID,
    //                          &packets,
    //                          storagePath,
    //                          fragmentLengthInSeconds);

    // thConsumer.join();
    // thFileWriter.join();

    // google::protobuf::ShutdownProtobufLibrary();

    print(LogType::WARNING, "Writer process with pid " + std::to_string(getpid()) + " stopped successfully");
    return 0;
}
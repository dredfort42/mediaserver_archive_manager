#include "recorder.hpp"

std::string appName = "mediaserver_archive_recorder";
std::string serviceUUID = generateUUID();
std::string appVersion = "1.0.0";

RecorderConfig getRecorderConfig(ConfigMap *config)
{
    RecorderConfig recorderConfig;

    recorderConfig.appName = config->getProperty("archive_recorder.name");
    if (recorderConfig.appName.empty())
    {
        print(LogType::DEBUGER, "Archive recorder name not defined, using default name: " + appName);
        recorderConfig.appName = appName;
    }
    else
    {
        appName = recorderConfig.appName;
    }

    recorderConfig.appUUID = serviceUUID;
    recorderConfig.appVersion = appVersion;

    recorderConfig.storagePath = config->getProperty("archive_recorder.storage_path");
    if (recorderConfig.storagePath.empty())
    {
        print(LogType::WARNING, "Archive storage path not defined, using default path: /app/storage");
        recorderConfig.storagePath = "/app/storage";
    }

    if (!std::filesystem::is_directory(recorderConfig.storagePath))
    {
        print(LogType::ERROR, "Archive storage path must be a path to an existing directory");
        exit(RTN_ERROR);
    }

    try
    {
        recorderConfig.fragmentLengthInSeconds = std::stoi(config->getProperty("archive_recorder.fragment_length_in_seconds"));
    }
    catch (const std::exception &e)
    {
        print(LogType::DEBUGER, "Archive recorder fragment length not defined, using default value: 300");
        recorderConfig.fragmentLengthInSeconds = 300;
    }

    if (recorderConfig.fragmentLengthInSeconds < 300 || recorderConfig.fragmentLengthInSeconds > 3600)
    {
        print(LogType::DEBUGER, "Archive recorder fragment length not in the range 300-3600, using default value: 300");
        recorderConfig.fragmentLengthInSeconds = 300;
    }

    return recorderConfig;
}

MessengerConfig getMessengerConfig(ConfigMap *config)
{
    MessengerConfig messengerConfig;

    messengerConfig.brokers = config->getProperty("kafka.brokers");
    if (messengerConfig.brokers.empty())
    {
        print(LogType::DEBUGER, "Kafka brokers not defined, using default value: localhost:9092");
        messengerConfig.brokers = "localhost:9092";
    }

    messengerConfig.clientId = config->getProperty("kafka.archive_recorder_client_id");
    if (messengerConfig.clientId.empty())
    {
        print(LogType::DEBUGER, "Kafka client id not defined, using default value: " + appName + "_" + serviceUUID);
        messengerConfig.clientId = appName + "_" + serviceUUID;
    }

    messengerConfig.groupId = config->getProperty("kafka.archive_recorder_group_id");
    if (messengerConfig.groupId.empty())
    {
        print(LogType::DEBUGER, "Kafka group id not defined, using default value: " + appName + "_group_" + serviceUUID);
        messengerConfig.groupId = appName + "_group_" + serviceUUID;
    }

    messengerConfig.topicIFrameByteOffsets = config->getProperty("kafka.topic_archive_iframe_byte_offsets");
    if (messengerConfig.topicIFrameByteOffsets.empty())
    {
        print(LogType::ERROR, "Kafka topic for iframe byte offsets not defined");
        exit(RTN_ERROR);
    }

    print(LogType::DEBUGER, "Messenger configuration readed");

    return messengerConfig;
}
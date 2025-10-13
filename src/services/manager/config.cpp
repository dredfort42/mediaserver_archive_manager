#include "manager.hpp"
#include <random>

std::string generateUUID()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream hexstream;
    for (int i = 0; i < 8; i++)
    {
        hexstream << std::hex << dis(gen);
    }

    hexstream << "-";
    for (int i = 0; i < 4; i++)
    {
        hexstream << std::hex << dis(gen);
    }

    hexstream << "-4";
    for (int i = 0; i < 3; i++)
    {
        hexstream << std::hex << dis(gen);
    }

    hexstream << "-";
    hexstream << std::hex << dis2(gen);
    for (int i = 0; i < 3; i++)
    {
        hexstream << std::hex << dis(gen);
    }

    hexstream << "-";
    for (int i = 0; i < 12; i++)
    {
        hexstream << std::hex << dis(gen);
    }

    return hexstream.str();
}

std::string serviceUUID = generateUUID();
std::string appVersion = "1.0.0";

ArchiveManagerConfig getArchiveManagerConfig(ConfigMap *config)
{
    ArchiveManagerConfig archiveManagerConfig;

    archiveManagerConfig.appName = config->getProperty("archive_manager.name");
    if (archiveManagerConfig.appName.empty())
    {
        print(LogType::DEBUGER, "Archive manager name not defined, using default name: archive_manager");
        archiveManagerConfig.appName = "mediaserver_archive_manager";
    }

    archiveManagerConfig.appVersion = appVersion;
    archiveManagerConfig.appUUID = serviceUUID;

    archiveManagerConfig.configPath = config->getConfigFile();

    archiveManagerConfig.archiveReaderPath = config->getProperty("archive_manager.reader_path");
    if (archiveManagerConfig.archiveReaderPath.empty())
    {
        print(LogType::WARNING, "Reader path not defined, using default path: /app/reader");
        archiveManagerConfig.archiveReaderPath = "/app/reader";
    }

    archiveManagerConfig.archiveRecorderPath = config->getProperty("archive_manager.recorder_path");
    if (archiveManagerConfig.archiveRecorderPath.empty())
    {
        print(LogType::WARNING, "Recorder path not defined, using default path: /app/recorder");
        archiveManagerConfig.archiveRecorderPath = "/app/recorder";
    }

    archiveManagerConfig.archiveStoragePath = config->getProperty("archive_manager.storage_path");
    if (archiveManagerConfig.archiveStoragePath.empty())
    {
        print(LogType::WARNING, "Archive storage path not defined, using default path: /app/storage");
        archiveManagerConfig.archiveStoragePath = "/app/storage";
    }

    archiveManagerConfig.archiveFragmentLengthInSeconds = config->getProperty("archive_manager.fragment_length_in_seconds");
    if (archiveManagerConfig.archiveFragmentLengthInSeconds.empty())
    {
        print(LogType::WARNING, "Archive fragment length not defined, using default value: 300 seconds");
        archiveManagerConfig.archiveFragmentLengthInSeconds = "300";
    }
    if (std::stoi(archiveManagerConfig.archiveFragmentLengthInSeconds) <= 0)
    {
        print(LogType::WARNING, "Archive fragment length must be greater than 0, using default value: 300 seconds");
        archiveManagerConfig.archiveFragmentLengthInSeconds = "300";
    }

    print(LogType::DEBUGER, "Archive Manager configuration readed");

    return archiveManagerConfig;
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

    messengerConfig.clientId = config->getProperty("kafka.archive_manager_client_id");
    if (messengerConfig.clientId.empty())
    {
        print(LogType::DEBUGER, "Kafka client id not defined, using default value: archive_manager");
        messengerConfig.clientId = "archive_manager";
    }

    messengerConfig.groupId = config->getProperty("kafka.archive_manager_group_id");
    if (messengerConfig.groupId.empty())
    {
        print(LogType::DEBUGER, "Kafka group id not defined, using default value: archive_manager_group");
        messengerConfig.groupId = "archive_manager_group";
    }

    messengerConfig.topicSystemDigest = config->getProperty("kafka.topic_system_digest");
    if (messengerConfig.topicSystemDigest.empty())
    {
        print(LogType::ERROR, "Kafka topic for system digest not defined");
        exit(RTN_ERROR);
    }

    messengerConfig.topicCameras = config->getProperty("kafka.topic_cameras");
    if (messengerConfig.topicCameras.empty())
    {
        print(LogType::ERROR, "Kafka topic for cameras not defined");
        exit(RTN_ERROR);
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

DatabaseConfig getDatabaseConfig(ConfigMap *config)
{
    DatabaseConfig databaseConfig;

    databaseConfig.host = config->getProperty("database.host");
    if (databaseConfig.host.empty())
    {
        print(LogType::DEBUGER, "Database host not defined, using default value: localhost");
        databaseConfig.host = "localhost";
    }

    databaseConfig.port = config->getProperty("database.port");
    if (databaseConfig.port.empty())
    {
        print(LogType::DEBUGER, "Database port not defined, using default value: 5432");
        databaseConfig.port = "5432";
    }

    databaseConfig.dbName = config->getProperty("database.name");
    if (databaseConfig.dbName.empty())
    {
        print(LogType::ERROR, "Database name not defined, using default value: archive_manager");
        exit(RTN_ERROR);
    }

    databaseConfig.user = config->getProperty("database.user");
    if (databaseConfig.user.empty())
    {
        print(LogType::ERROR, "Database user not defined, using default value: archive_manager");
        exit(RTN_ERROR);
    }

    databaseConfig.password = config->getProperty("database.password");
    if (databaseConfig.password.empty())
    {
        print(LogType::ERROR, "Database password not defined, using default value: archive_manager");
        exit(RTN_ERROR);
    }

    databaseConfig.sslMode = config->getProperty("database.ssl_mode");
    if (databaseConfig.sslMode.empty())
    {
        print(LogType::DEBUGER, "Database SSL mode not defined, using default value: disable");
        databaseConfig.sslMode = "disable";
    }

    print(LogType::DEBUGER, "Database configuration readed");

    return databaseConfig;
}

#include "manager.hpp"

std::string appName = "mediaserver_archive_manager";
std::string serviceUUID = generateUUID();
std::string appVersion = "1.0.0";

ArchiveManagerConfig getArchiveManagerConfig(ConfigMap *config)
{
    ArchiveManagerConfig archiveManagerConfig;

    archiveManagerConfig.appName = config->getProperty("archive_manager.name");
    if (archiveManagerConfig.appName.empty())
    {
        print(LogType::DEBUGER, "Archive manager name not defined, using default name: " + appName);
        archiveManagerConfig.appName = appName;
    }
    else
    {
        appName = archiveManagerConfig.appName;
    }

    archiveManagerConfig.appUUID = serviceUUID;
    archiveManagerConfig.appVersion = appVersion;

    archiveManagerConfig.configPath = config->getConfigFile();

    archiveManagerConfig.archiveRecorderPath = config->getProperty("archive_manager.recorder_path");
    if (archiveManagerConfig.archiveRecorderPath.empty())
    {
        print(LogType::WARNING, "Archive recorder path not defined, using default path: /app/archive_recorder");
        archiveManagerConfig.archiveRecorderPath = "/app/archive_recorder";
    }

    if (!std::filesystem::exists(archiveManagerConfig.archiveRecorderPath) || std::filesystem::is_directory(archiveManagerConfig.archiveRecorderPath))
    {
        print(LogType::ERROR, "Archive recorder path must be a path to an existing file");
        exit(RTN_ERROR);
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
        print(LogType::DEBUGER, "Kafka client id not defined, using default value: " + appName + "_" + serviceUUID);
        messengerConfig.clientId = appName + "_" + serviceUUID;
    }

    messengerConfig.groupId = config->getProperty("kafka.archive_manager_group_id");
    if (messengerConfig.groupId.empty())
    {
        print(LogType::DEBUGER, "Kafka group id not defined, using default value: " + appName + "_group_" + serviceUUID);
        messengerConfig.groupId = appName + "_group_" + serviceUUID;
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
        print(LogType::ERROR, "Database name not defined");
        exit(RTN_ERROR);
    }

    databaseConfig.user = config->getProperty("database.user");
    if (databaseConfig.user.empty())
    {
        print(LogType::ERROR, "Database user not defined");
        exit(RTN_ERROR);
    }

    databaseConfig.password = config->getProperty("database.password");
    if (databaseConfig.password.empty())
    {
        print(LogType::ERROR, "Database password not defined");
        exit(RTN_ERROR);
    }

    databaseConfig.sslMode = config->getProperty("database.ssl_mode");
    if (databaseConfig.sslMode.empty())
    {
        print(LogType::DEBUGER, "Database SSL mode not defined, using default value: disable");
        databaseConfig.sslMode = "disable";
    }

    databaseConfig.table_iframe_byte_offsets = config->getProperty("database.table_iframe_byte_offsets");
    if (databaseConfig.table_iframe_byte_offsets.empty())
    {
        print(LogType::DEBUGER, "Database table for iframe byte offsets not defined, using default value: iframe_byte_offsets");
        databaseConfig.table_iframe_byte_offsets = "iframe_byte_offsets";
    }

    print(LogType::DEBUGER, "Database configuration readed");

    return databaseConfig;
}

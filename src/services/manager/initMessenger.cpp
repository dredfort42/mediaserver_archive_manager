#include "manager.hpp"

std::string initMessenger(Messenger &messenger, MessengerConfig &messengerConfig, Messenger::topics_t &topics, std::string &appUUID)
{
    short configurationError = 0;
    configurationError += messenger.setConfigurationProperty(Messenger::ClientType::BOTH, "bootstrap.servers", messengerConfig.brokers);
    configurationError += messenger.setConfigurationProperty(Messenger::ClientType::BOTH, "client.id", messengerConfig.clientId + "_" + appUUID);
    configurationError += messenger.setConfigurationProperty(Messenger::ClientType::CONSUMER, "group.id", messengerConfig.groupId + "_" + appUUID);
    configurationError += messenger.setConfigurationProperty(Messenger::ClientType::CONSUMER, "enable.auto.commit", "false");
    configurationError += messenger.setConfigurationProperty(Messenger::ClientType::CONSUMER, "auto.offset.reset", "earliest");
    configurationError += messenger.setConfigurationProperty(Messenger::ClientType::CONSUMER, "enable.partition.eof", "true");
    configurationError += messenger.setConfigurationProperty(Messenger::ClientType::CONSUMER, "enable.auto.offset.store", "false");

    // Add session timeout and heartbeat configurations to prevent rebalancing issues
    configurationError += messenger.setConfigurationProperty(Messenger::ClientType::CONSUMER, "session.timeout.ms", "30000");
    configurationError += messenger.setConfigurationProperty(Messenger::ClientType::CONSUMER, "heartbeat.interval.ms", "10000");
    configurationError += messenger.setConfigurationProperty(Messenger::ClientType::CONSUMER, "max.poll.interval.ms", "300000");

    configurationError += messenger.setConfigurationProperty(Messenger::ClientType::PRODUCER, "acks", "all");

    if (configurationError)
        return "Kafka configuration error";

    if (messenger.createMessenger())
        return "Failed to create messenger";

    topics.insert(messengerConfig.topicIFrameByteOffsets);

    return "";
}

void produceServiceDigest(Messenger &messenger,
                          std::string &topic,
                          const ArchiveManagerConfig &archiveManagerConfig,
                          ServiceStatus statusCode)
{
    proto::ProtoServiceDigest serviceDigest;

    serviceDigest.set_service_uuid(archiveManagerConfig.appUUID);
    serviceDigest.set_service_name(archiveManagerConfig.appName);
    serviceDigest.set_service_version(archiveManagerConfig.appVersion);
    serviceDigest.set_service_status_code(static_cast<int>(statusCode));
    serviceDigest.add_endpoints("0.0.0.0");
    serviceDigest.set_last_heartbeat(static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count()));

    std::string serializedState;
    serviceDigest.SerializePartialToString(&serializedState);

    messenger.produceMessage(topic, archiveManagerConfig.appUUID, serializedState);
}
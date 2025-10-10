#include "manager.hpp"

// void consumeMessages(volatile sig_atomic_t *wasInrerrupted,
//                      Messenger::messenger_content_t *messengerContent,
//                      Messenger::topics_t *topics,
//                      ConfigMap *config)
// {
//     if (DEBUG)
//         print(LogType::INFO, ">>> Start consumer thread");

//     Messenger messenger(wasInrerrupted);

//     topics->insert(config->getProperty("kafka.topic.compact.camerasData"));
//     topics->insert(config->getProperty("kafka.topic.compact.archiverTasks"));
//     topics->insert(config->getProperty("kafka.topic.compact.clean.hour.fileOffsets"));

//     int configurationError = 0;
//     configurationError += messenger.setConfigurationProperty(Messenger::ClientType::BOTH, "metadata.broker.list", config->getProperty("kafka.host") + ":" + config->getProperty("kafka.port"));
//     configurationError += messenger.setConfigurationProperty(Messenger::ClientType::BOTH, "client.id", "archive");
//     configurationError += messenger.setConfigurationProperty(Messenger::ClientType::PRODUCER, "acks", config->getProperty("kafka.acks"));
//     configurationError += messenger.setConfigurationProperty(Messenger::ClientType::CONSUMER, "group.id", "archives");
//     configurationError += messenger.setConfigurationProperty(Messenger::ClientType::CONSUMER, "enable.auto.commit", "false");
//     configurationError += messenger.setConfigurationProperty(Messenger::ClientType::CONSUMER, "auto.offset.reset", "earliest");
//     configurationError += messenger.setConfigurationProperty(Messenger::ClientType::CONSUMER, "enable.partition.eof", "true");
//     configurationError += messenger.setConfigurationProperty(Messenger::ClientType::CONSUMER, "enable.auto.offset.store", "false");

//     if (configurationError)
//     {
//         print(LogType::ERROR, "Configuration error");
//         *wasInrerrupted = 1;
//     }
//     else
//     {
//         messenger.createMessenger();
//         if (DEBUG)
//             print(LogType::INFO, "Messanger created");

//         messenger.consumeMessages(messengerContent, topics);
//     }

//     if (DEBUG)
//         print(LogType::INFO, "<<< Stop consumer thread");
// }

void consumeMessages(Messenger *messenger,
                     Messenger::messenger_content_t *messengerContent,
                     Messenger::topics_t *topics)
{
    print(LogType::DEBUGER, ">>> Start consumer thread");

    messenger->consumeMessages(messengerContent, topics);

    print(LogType::DEBUGER, "<<< Stop consumer thread");
}
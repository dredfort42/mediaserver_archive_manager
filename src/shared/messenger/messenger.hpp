#ifndef MESSENGER_HPP
#define MESSENGER_HPP

#include <csignal>
#include <iostream>
#include <string>
#include <set>
#include <map>
#include <list>
#include <filesystem>
#include <unistd.h>
#include <librdkafka/rdkafkacpp.h>

#include "logPrinter.hpp"

namespace MessengerConstants
{
    constexpr int FLASH_TIMEOUT_MS = 10 * 1000;
    constexpr int CONSUMER_DESTROY_TIMEOUT_MS = 10 * 1000;
    constexpr int TIMEOUT_MS = 1000;
}

/**
 * @brief Simple message wrapper without mutex
 * The mutex protection is now at the map level, not per-message
 */
struct MxMessage
{
    std::string message;

    MxMessage(std::string msg) : message(msg) {}

    MxMessage(const MxMessage &) = default;
    MxMessage &operator=(const MxMessage &) = default;
    MxMessage(MxMessage &&) noexcept = default;
    MxMessage &operator=(MxMessage &&) noexcept = default;

    ~MxMessage() = default;
};

class Messenger
{
public:
    /**
     * @brief The client type
     */
    enum class ClientType
    {
        PRODUCER,
        CONSUMER,
        BOTH,
    };

    /**
     * @brief The set of all topics names to consume from
     */
    typedef typename std::set<std::string> topics_t;

    /**
     * @brief The message type
     * @param first The key of the message
     * @param second The value of the message
     */
    typedef typename std::pair<std::string, std::string> message_t;

    /**
     * @brief The video frames packet type
     * @param first The timestamp of the packet
     * @param second The payload of the packet
     */
    typedef typename std::pair<int64_t, std::string> packet_t;

    /**
     * @brief The messages map
     * @param first The key of the message
     * @param second The message with mutex
     */
    typedef typename std::map<std::string, MxMessage> messages_map_t;

    /**
     * @brief The topics content type
     */
    typedef typename std::pair<std::string, messages_map_t> topic_messages_t;

    /**
     * @brief All messages in different topics
     * @param first The topic name
     * @param second The messages map in the topic
     */
    typedef typename std::map<std::string, messages_map_t> messenger_content_t;

private:
    volatile sig_atomic_t *_isInterrupted;
    RdKafka::Conf *_confProducer;
    RdKafka::Conf *_confConsumer;
    RdKafka::Producer *_producer;
    RdKafka::KafkaConsumer *_consumer;
    std::string _errstr;

    Messenger();
    Messenger(const Messenger &);
    // Messenger &operator=(const Messenger &);

    /**
     * @brief Process message
     * @param message The message for processing
     */
    message_t _consumeMessage(RdKafka::Message *message);

public:
    /**
     * @brief Create a Messenger
     * @param isInterrupted The pointer to the interrupt signal variable
     */
    Messenger(volatile sig_atomic_t *isInterrupted);

    /**
     * @brief Destroy a Messenger
     */
    ~Messenger();

    /**
     * @brief Set a configuration property for the Messenger
     * @param clientType The type of the client
     * @param name The name of the configuration property
     * @param value The value of the configuration property
     * @return 0 on success, -1 on failure
     */
    int setConfigurationProperty(const ClientType clientType, const std::string &name, const std::string &value);

    /**
     * @brief Prepare the Messenger for use
     * @return 0 on success, -1 on failure
     */
    int createMessenger();

    /**
     * @brief Prepare a producer for use
     * @return 0 on success, -1 on failure
     */
    int createProducer();

    /**
     * @brief Produce a message to a topic
     * @param topic The topic to produce
     * @param key The key to produce
     * @param value The value to produce
     * @return 0 on success, -1 on failure
     */
    int produceMessage(const std::string &topic, const std::string &key, const std::string &value);

    // /**
    //  * @brief Produce service state
    //  * @param state The service state code to produce
    //  * @param message The error message to produce if state is an error
    //  * @return 0 on success, -1 on failure
    //  */
    // int produceServiceDigest(int state, std::string &error_message);

    /**
     * @brief Consume a messages from a topics and add last of them to a map
     * @param messages The messages map to consume
     * @param topics The topics to consume from
     * @return 0 on success, -1 on failure
     */
    int consumeMessages(messenger_content_t *messages, const topics_t *topics);

    /**
     * @brief Consume messages with avpackets from a particular topic and add all of them to a list
     * @param packets The packets list to consume in format key: timestamp value: payload
     * @param topic The topic to consume from
     * @return 0 on success, -1 on failure
     */
    int consumeMessages(std::list<packet_t> *packets, const std::string *topic);

    /**
     * @brief Get the interrupt signal variable
     * @return The interrupt signal variable
     */
    volatile sig_atomic_t *getInterruptSignal();
};

#endif // MESSENGER_HPP
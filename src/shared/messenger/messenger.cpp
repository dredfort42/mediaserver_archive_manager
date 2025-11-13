#include "messenger.hpp"

Messenger::Messenger(volatile sig_atomic_t *isInterrupted)
{
    _isInterrupted = isInterrupted;
    _confConsumer = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    _confProducer = RdKafka::Conf::create(RdKafka::Conf::CONF_GLOBAL);
    _consumer = nullptr;
    _producer = nullptr;
    _errstr = "";
}

Messenger::~Messenger()
{
    if (_confConsumer)
        delete _confConsumer;

    if (_consumer)
    {
        _consumer->close();
        RdKafka::wait_destroyed(MessengerConstants::CONSUMER_DESTROY_TIMEOUT_MS);
        print(LogType::DEBUGER, "Consumer " + _consumer->name() + " closed");

        delete _consumer;
        print(LogType::DEBUGER, "Consumer deleted");
    }

    if (_confProducer)
        delete _confProducer;

    if (_producer)
    {
        _producer->flush(MessengerConstants::FLASH_TIMEOUT_MS);
        print(LogType::DEBUGER, "Producer " + _producer->name() + " flushed");
        delete _producer;
        print(LogType::DEBUGER, "Producer deleted");
    }
}

int Messenger::setConfigurationProperty(const Messenger::ClientType clientType, const std::string &name, const std::string &value)
{
    switch (clientType)
    {
    case Messenger::ClientType::BOTH:
        if (_confProducer->set(name, value, _errstr) != RdKafka::Conf::CONF_OK)
        {
            print(LogType::ERROR, _errstr);
            return -1;
        }
        if (_confConsumer->set(name, value, _errstr) != RdKafka::Conf::CONF_OK)
        {
            print(LogType::ERROR, _errstr);
            return -1;
        }
        break;

    case Messenger::ClientType::PRODUCER:
        if (_confProducer->set(name, value, _errstr) != RdKafka::Conf::CONF_OK)
        {
            print(LogType::ERROR, _errstr);
            return -1;
        }
        break;

    case Messenger::ClientType::CONSUMER:
        if (_confConsumer->set(name, value, _errstr) != RdKafka::Conf::CONF_OK)
        {
            print(LogType::ERROR, _errstr);
            return -1;
        }
        break;

    default:
        print(LogType::ERROR, "Unknown client type for configuration property");
        break;
    }

    print(LogType::DEBUGER, "Set configuration property " + name + " : " + value);

    return 0;
}

int Messenger::createMessenger()
{
    _producer = RdKafka::Producer::create(_confProducer, _errstr);
    _consumer = RdKafka::KafkaConsumer::create(_confConsumer, _errstr);

    if (!_producer || !_consumer)
    {
        print(LogType::ERROR, _errstr);
        return -1;
    }

    print(LogType::DEBUGER, "Created producer " + _producer->name());
    print(LogType::DEBUGER, "Created consumer " + _consumer->name());

    return 0;
}

int Messenger::createProducer()
{
    _producer = RdKafka::Producer::create(_confProducer, _errstr);

    if (!_producer)
    {
        print(LogType::ERROR, _errstr);
        return -1;
    }

    print(LogType::DEBUGER, "Created producer " + _producer->name());

    return 0;
}

int Messenger::produceMessage(const std::string &topic, const std::string &key, const std::string &value)
{
retry:
    RdKafka::ErrorCode err = _producer->produce(
        topic,
        RdKafka::Topic::PARTITION_UA,
        RdKafka::Producer::RK_MSG_COPY,
        const_cast<char *>(value.c_str()), value.size(),
        const_cast<char *>(key.c_str()), key.size(),
        0,
        NULL,
        NULL);

    if (err != RdKafka::ERR_NO_ERROR && !_isInterrupted)
    {
        print(LogType::ERROR, "Failed to produce to topic " + topic + ": " + RdKafka::err2str(err));

        if (err == RdKafka::ERR__QUEUE_FULL && !_isInterrupted)
        {
            _producer->poll(MessengerConstants::TIMEOUT_MS);
            goto retry;
        }
    }

    // Print kafka messege size and topic name
    // print(LogType::DEBUGER, std::to_string(value.size()) + " bytes \t Enqueued for topic " + topic);

    _producer->poll(0);
    _producer->flush(MessengerConstants::FLASH_TIMEOUT_MS);

    if (_producer->outq_len() > 0)
        print(LogType::WARNING, std::to_string(_producer->outq_len()) + " messages were not delivered");

    return 0;
}

int Messenger::consumeMessages(Messenger::messenger_content_t *messages, const Messenger::topics_t *topics)
{
    std::vector<std::string> topics_vector(topics->begin(), topics->end());

    if (_consumer->subscribe(topics_vector) != RdKafka::ERR_NO_ERROR)
    {
        print(LogType::ERROR, "Failed to subscribe to topics");
        return -1;
    }

    for (auto topic : *topics)
    {
        if (messages->find(topic) == messages->end())
            messages->insert(Messenger::topic_messages_t(topic, Messenger::messages_map_t()));
        else
            messages->at(topic) = Messenger::messages_map_t();
    }

    while (!*_isInterrupted)
    {
        RdKafka::Message *message = _consumer->consume(1000);
        Messenger::message_t tmp = _consumeMessage(message);

        if (!tmp.first.empty() && message)
        {
            std::string topicName = message->topic_name();

            // Check if topic exists in messages map
            if (messages->find(topicName) == messages->end())
            {
                delete message;
                continue;
            }

            Messenger::messages_map_t *topicMessages = &messages->at(topicName);

            if (topicMessages->find(tmp.first) == topicMessages->end())
                topicMessages->insert(std::pair<std::string, MxMessage>(tmp.first, MxMessage(tmp.second)));
            else
            {
                std::lock_guard<std::mutex> lock(topicMessages->at(tmp.first).mutex);
                topicMessages->at(tmp.first).message = tmp.second;
            }
        }

        delete message;
    }

    return 0;
}

int Messenger::consumeMessages(std::list<Messenger::packet_t> *packets, const std::string *topic)
{
    std::vector<std::string> topics_vector;
    topics_vector.push_back(*topic);

    if (_consumer->subscribe(topics_vector) != RdKafka::ERR_NO_ERROR)
    {
        print(LogType::ERROR, "Failed to subscribe to topic " + *topic);
        return -1;
    }

    packets->clear();

    while (!*_isInterrupted)
    {
        RdKafka::Message *message = _consumer->consume(1000);
        Messenger::message_t tmp = _consumeMessage(message);

        if (!tmp.first.empty())
            packets->emplace_back(message->timestamp().timestamp, tmp.second);

        delete message;
    }

    return 0;
}

Messenger::message_t Messenger::_consumeMessage(RdKafka::Message *message)
{
    switch (message->err())
    {
    case RdKafka::ERR__TIMED_OUT:
        // print(LogType::DEBUGER, "Consume timed out");
        return message_t("", "");

    case RdKafka::ERR_NO_ERROR:
        return message_t(_getMessageKey(message), std::string(static_cast<const char *>(message->payload()), message->len()));

    case RdKafka::ERR__PARTITION_EOF:
        print(LogType::DEBUGER, "Reached end of partition");
        return message_t("", "");

    case RdKafka::ERR__UNKNOWN_TOPIC:
    case RdKafka::ERR__UNKNOWN_PARTITION:
        print(LogType::ERROR, "Consume failed: " + message->errstr());
        return message_t("", "");

    default:
        print(LogType::ERROR, "Consume failed: " + message->errstr());
        *_isInterrupted = 1;
        return message_t("", "");
    }
}

std::string Messenger::_getMessageKey(RdKafka::Message *message)
{
    if (message->key() && message->key_len() > 0)
        return std::string(message->key()->c_str());
    return "";
}

volatile sig_atomic_t *Messenger::getInterruptSignal()
{
    return _isInterrupted;
}
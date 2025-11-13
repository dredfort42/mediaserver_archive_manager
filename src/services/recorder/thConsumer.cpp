#include "recorder.hpp"

void consumeAVPackets(Messenger *messenger,
                      std::list<Messenger::packet_t> *avPackets,
                      std::mutex *avPacketsMutex,
                      std::string *topic)
{
    print(LogType::DEBUGER, ">>> Start consumer thread");

    // Consume messages using messenger's internal implementation
    // We need to wrap it to add mutex protection
    std::vector<std::string> topics_vector;
    topics_vector.push_back(*topic);

    // Note: This calls the messenger's consumeMessages which doesn't have mutex protection
    // We need to modify the approach to add packets with protection
    volatile sig_atomic_t *isInterrupted = messenger->getInterruptSignal();

    if (messenger->_consumer && messenger->_consumer->subscribe(topics_vector) != RdKafka::ERR_NO_ERROR)
    {
        print(LogType::ERROR, "Failed to subscribe to topic " + *topic);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(*avPacketsMutex);
        avPackets->clear();
    }

    while (!*isInterrupted)
    {
        RdKafka::Message *message = messenger->_consumer->consume(1000);

        switch (message->err())
        {
        case RdKafka::ERR__TIMED_OUT:
            delete message;
            continue;

        case RdKafka::ERR_NO_ERROR:
        {
            std::lock_guard<std::mutex> lock(*avPacketsMutex);
            avPackets->emplace_back(message->timestamp().timestamp,
                                    std::string(static_cast<const char *>(message->payload()), message->len()));
        }
            delete message;
            break;

        case RdKafka::ERR__PARTITION_EOF:
            print(LogType::DEBUGER, "Reached end of partition");
            delete message;
            continue;

        case RdKafka::ERR__UNKNOWN_TOPIC:
        case RdKafka::ERR__UNKNOWN_PARTITION:
            print(LogType::ERROR, "Consume failed: " + message->errstr());
            delete message;
            continue;

        default:
            print(LogType::ERROR, "Consume failed: " + message->errstr());
            delete message;
            *isInterrupted = 1;
            break;
        }
    }

    print(LogType::DEBUGER, "<<< Stop consumer thread");
}
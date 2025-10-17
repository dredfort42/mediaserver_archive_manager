#include "recorder.hpp"

void consumeAVPackets(Messenger *messenger,
                      std::list<Messenger::packet_t> *avPackets,
                      std::string *topic)
{
    print(LogType::DEBUGER, ">>> Start consumer thread");

    messenger->consumeMessages(avPackets, topic);

    print(LogType::DEBUGER, "<<< Stop consumer thread");
}
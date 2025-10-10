#include "manager.hpp"

void consumeMessages(Messenger *messenger,
                     Messenger::messenger_content_t *messengerContent,
                     Messenger::topics_t *topics)
{
    print(LogType::DEBUGER, ">>> Start consumer thread");

    messenger->consumeMessages(messengerContent, topics);

    print(LogType::DEBUGER, "<<< Stop consumer thread");
}
#include "manager.hpp"

Messenger::messages_map_t *getOffsetsTopic(volatile sig_atomic_t *isInrerrupted,
                                           std::string *offsetsTopic,
                                           Messenger::messenger_content_t *messengerContent)
{
    while (!*isInrerrupted)
    {
        auto it = messengerContent->find(*offsetsTopic);
        if (it != messengerContent->end())
            return &it->second;

        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }

    return nullptr;
}

void storeOffsets(volatile sig_atomic_t *isInrerrupted,
                  PGconn *dbConnect,
                  Messenger::messenger_content_t *messengerContent,
                  std::string *offsetsTopic,
                  std::string *dbTable)
{
    print(LogType::DEBUGER, ">>> Start store offsets thread");

    Messenger::messages_map_t *offsetsTopicMessages = getOffsetsTopic(isInrerrupted, offsetsTopic, messengerContent);

    while (!*isInrerrupted)
    {
        std::cout << "***\tWaiting for offsets..." << std::endl;

        if (offsetsTopicMessages->begin() != offsetsTopicMessages->end())
        {
            // Get the first message and copy it out
            auto it = offsetsTopicMessages->begin();
            Messenger::message_t message;
            message.first = it->first;
            message.second = it->second.message;

            // Erase the message from the map
            offsetsTopicMessages->erase(it);

            std::string cameraUUID = message.first;
            proto::ProtoOffset protoOffset;

            std::cout << "***\tParsing offset..." << std::endl;
            protoOffset.ParseFromString(message.second);

            if (cameraUUID.empty() ||
                protoOffset.folder() <= 0 ||
                protoOffset.file() <= 0 ||
                protoOffset.timestamp() <= 0 ||
                protoOffset.offset() <= 0)
            {
                print(LogType::DEBUGER, "Offset from " + cameraUUID + " is empty or invalid");
                continue;
            }

            std::string query = "INSERT INTO public." + *dbTable + " (stream_uuid, folder, file, iframe_indexes) VALUES ('" +
                                cameraUUID + "', " +
                                std::to_string(protoOffset.folder()) + ", " +
                                std::to_string(protoOffset.file()) + ", " +
                                "ARRAY[(" + std::to_string(protoOffset.timestamp()) + ", " + std::to_string(protoOffset.offset()) + ")::iframe_index]) " +
                                "ON CONFLICT (stream_uuid, folder, file) DO UPDATE SET " +
                                "iframe_indexes = " + *dbTable + ".iframe_indexes || ARRAY[(" + std::to_string(protoOffset.timestamp()) + ", " + std::to_string(protoOffset.offset()) + ")::iframe_index];";

            PGresult *res;
        sendOffset:
            res = PQexec(dbConnect, query.c_str());
            if (!*isInrerrupted && PQresultStatus(res) != PGRES_COMMAND_OK)
            {
                print(LogType::ERROR, "Add data to table failed: " + std::string(PQerrorMessage(dbConnect)));
                goto sendOffset;
            }

            print(LogType::DEBUGER, "Offset from " + cameraUUID + " for " + std::to_string(protoOffset.timestamp()) + " was sent to DB");

            protoOffset.Clear();
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    print(LogType::DEBUGER, "<<< Stop store offsets thread");
}
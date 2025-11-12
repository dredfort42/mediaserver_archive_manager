#include "manager.hpp"

ThreadSafeMessagesMap *getOffsetsTopic(volatile sig_atomic_t *isInrerrupted,
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

    ThreadSafeMessagesMap *offsetsTopicMessages = getOffsetsTopic(isInrerrupted, offsetsTopic, messengerContent);

    if (offsetsTopicMessages == nullptr)
    {
        print(LogType::ERROR, "Failed to get offsets topic");
        return;
    }

    while (!*isInrerrupted)
    {
        std::cout << "***\tWaiting for offsets..." << std::endl;

        // Safely extract ONE message at a time
        std::string cameraUUID;
        std::string messageData;
        bool hasMessage = false;

        // Critical section: grab and remove the first message with proper locking
        {
            std::lock_guard<std::mutex> mapLock(offsetsTopicMessages->mapMutex);

            auto &messagesMap = offsetsTopicMessages->map;

            if (!messagesMap.empty())
            {
                auto it = messagesMap.begin();
                cameraUUID = it->first;

                // Lock the individual message mutex to read its content
                {
                    std::lock_guard<std::mutex> msgLock(it->second.mutex);
                    messageData = it->second.message;
                }

                // Remove the message from the map
                messagesMap.erase(it);
                hasMessage = true;
            }
        }

        if (!hasMessage)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        // Now process outside the critical section
        proto::ProtoOffset protoOffset;

        std::cout << "***\tParsing offset..." << std::endl;
        if (!protoOffset.ParseFromString(messageData))
        {
            print(LogType::ERROR, "Failed to parse protobuf message from " + cameraUUID);
            continue;
        }

        // Skip offsets older than 5 minutes
        auto currentTime = std::chrono::system_clock::now();
        auto currentTimestamp = std::chrono::duration_cast<std::chrono::seconds>(currentTime.time_since_epoch()).count();
        std::cout << "***\tCurrent timestamp: " << currentTimestamp << std::endl;

        auto offsetAge = currentTimestamp - protoOffset.timestamp() / 1000;
        if (offsetAge > 300) // 5 minutes = 300 seconds
        {
            print(LogType::DEBUGER, "Skipping offset from " + cameraUUID + " - too old (age: " + std::to_string(offsetAge) + " seconds)");
            continue;
        }

        if (cameraUUID.empty() ||
            protoOffset.folder() <= 0 ||
            protoOffset.file() <= 0 ||
            protoOffset.timestamp() <= 0 ||
            protoOffset.offset() <= 0)
        {
            print(LogType::DEBUGER, "Offset from " + cameraUUID + " is empty or invalid");
            continue;
        }

        std::string query = "INSERT INTO public." + *dbTable + " (camera_id, folder, file, iframe_indexes) VALUES ('" +
                            cameraUUID + "', " +
                            std::to_string(protoOffset.folder()) + ", " +
                            std::to_string(protoOffset.file()) + ", " +
                            "ARRAY[(" + std::to_string(protoOffset.timestamp()) + ", " + std::to_string(protoOffset.offset()) + ")::iframe_index]) " +
                            "ON CONFLICT (camera_id, folder, file) DO UPDATE SET " +
                            "iframe_indexes = " + *dbTable + ".iframe_indexes || ARRAY[(" + std::to_string(protoOffset.timestamp()) + ", " + std::to_string(protoOffset.offset()) + ")::iframe_index];";

        PGresult *res;
    sendOffset:
        res = PQexec(dbConnect, query.c_str());
        if (!*isInrerrupted && PQresultStatus(res) != PGRES_COMMAND_OK)
        {
            print(LogType::ERROR, "Add data to table failed: " + std::string(PQerrorMessage(dbConnect)));
            PQclear(res);
            goto sendOffset;
        }
        PQclear(res);

        print(LogType::DEBUGER, "Offset from " + cameraUUID + " for " + std::to_string(protoOffset.timestamp()) + " was sent to DB");
    }

    print(LogType::DEBUGER, "<<< Stop store offsets thread");
}
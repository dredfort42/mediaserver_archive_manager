#include "manager.hpp"

void waitingForOffsetsData(volatile sig_atomic_t *isInterrupted, Messenger::messenger_content_t *messengerContent, std::string *offsetsTopic)
{
    while (!*isInterrupted && messengerContent->find(*offsetsTopic) == messengerContent->end())
    {
        print(LogType::DEBUGER, "Waiting for offsets topic...");
        std::this_thread::sleep_for(std::chrono::milliseconds(ArchiveManagerConstants::DELAY_MS));
    }
}

void storeOffsets(volatile sig_atomic_t *isInterrupted,
                  PGconn *dbConnect,
                  Messenger::messenger_content_t *messengerContent,
                  std::string *offsetsTopic,
                  std::string *dbTable)
{
    print(LogType::DEBUGER, ">>> Start store offsets thread");

    waitingForOffsetsData(isInterrupted, messengerContent, offsetsTopic);

    while (!*isInterrupted)
    {
        Messenger::messages_map_t *messages = &messengerContent->at(*offsetsTopic);

        for (auto it = messages->begin(); !*isInterrupted && it != messages->end();)
        {
            std::string cameraID;
            std::string messageCopy;
            {
                std::lock_guard<std::mutex> lock(it->second.mutex);
                cameraID = it->first;
                messageCopy = it->second.message;
            }

            if (!messageCopy.empty())
            {
                proto::ProtoOffset protoOffset;
                if (!protoOffset.ParseFromString(messageCopy))
                {
                    print(LogType::ERROR, "Failed to parse protobuf message from " + cameraID);
                    it = messages->erase(it);
                    continue;
                }

                if (cameraID.empty() ||
                    protoOffset.folder() <= 0 ||
                    protoOffset.file() <= 0 ||
                    protoOffset.timestamp() <= 0 ||
                    protoOffset.offset() <= 0)
                {
                    print(LogType::DEBUGER, "Offset from " + cameraID + " is empty or invalid");
                    it = messages->erase(it);
                    continue;
                }

                std::string query = "INSERT INTO public." + *dbTable + " (camera_id, folder, file, iframe_indexes) VALUES ('" +
                                    cameraID + "', " +
                                    std::to_string(protoOffset.folder()) + ", " +
                                    std::to_string(protoOffset.file()) + ", " +
                                    "ARRAY[(" + std::to_string(protoOffset.timestamp()) + ", " + std::to_string(protoOffset.offset()) + ")::iframe_index]) " +
                                    "ON CONFLICT (camera_id, folder, file) DO UPDATE SET " +
                                    "iframe_indexes = " + *dbTable + ".iframe_indexes || ARRAY[(" + std::to_string(protoOffset.timestamp()) + ", " + std::to_string(protoOffset.offset()) + ")::iframe_index];";

                PGresult *res;
            sendOffset:
                res = PQexec(dbConnect, query.c_str());
                if (!*isInterrupted && PQresultStatus(res) != PGRES_COMMAND_OK)
                {
                    print(LogType::ERROR, "Add data to table failed: " + std::string(PQerrorMessage(dbConnect)));
                    PQclear(res);
                    goto sendOffset;
                }
                PQclear(res);

                print(LogType::DEBUGER, "Offset from " + cameraID + " for " + std::to_string(protoOffset.timestamp()) + " was sent to DB");
            }

            it = messages->erase(it);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(ArchiveManagerConstants::DELAY_MS));
    }

    print(LogType::DEBUGER, "<<< Stop store offsets thread");
}
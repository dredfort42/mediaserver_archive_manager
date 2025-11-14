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
        // Use find() to safely check if topic exists
        auto topicIt = messengerContent->find(*offsetsTopic);
        if (topicIt == messengerContent->end())
        {
            print(LogType::DEBUGER, "Offsets topic not found, waiting...");
            std::this_thread::sleep_for(std::chrono::milliseconds(ArchiveManagerConstants::DELAY_MS));
            continue;
        }

        Messenger::messages_map_t *messages = &topicIt->second;

        // Create snapshot of keys to avoid iterator invalidation during concurrent access
        std::vector<std::string> keysToProcess;
        for (const auto &entry : *messages)
        {
            keysToProcess.push_back(entry.first);
        }

        for (const auto &cameraID : keysToProcess)
        {
            if (*isInterrupted)
                break;

            auto msgIt = messages->find(cameraID);
            if (msgIt == messages->end())
                continue;

            std::string messageCopy;
            {
                std::lock_guard<std::mutex> lock(msgIt->second.mutex);
                messageCopy = msgIt->second.message;
            }

            if (!messageCopy.empty())
            {
                proto::ProtoOffset protoOffset;
                if (!protoOffset.ParseFromString(messageCopy))
                {
                    print(LogType::ERROR, "Failed to parse protobuf message from " + cameraID);
                    messages->erase(cameraID);
                    continue;
                }

                // Validate data - folder, file, and offset can be 0 (valid), but timestamp should be positive
                if (cameraID.empty() ||
                    protoOffset.folder() < 0 ||
                    protoOffset.file() < 0 ||
                    protoOffset.timestamp() <= 0 ||
                    protoOffset.offset() < 0)
                {
                    print(LogType::DEBUGER, "Offset from " + cameraID + " is empty or invalid");
                    messages->erase(cameraID);
                    continue;
                }

                // Use parameterized query to prevent SQL injection
                std::string query = "INSERT INTO public." + *dbTable + " (camera_id, folder, file, iframe_indexes) "
                                                                       "VALUES ($1, $2, $3, ARRAY[($4, $5)::iframe_index]) "
                                                                       "ON CONFLICT (camera_id, folder, file) DO UPDATE SET "
                                                                       "iframe_indexes = " +
                                    *dbTable + ".iframe_indexes || ARRAY[($4, $5)::iframe_index]";

                const char *paramValues[5];
                paramValues[0] = cameraID.c_str();
                std::string folderStr = std::to_string(protoOffset.folder());
                std::string fileStr = std::to_string(protoOffset.file());
                std::string timestampStr = std::to_string(protoOffset.timestamp());
                std::string offsetStr = std::to_string(protoOffset.offset());
                paramValues[1] = folderStr.c_str();
                paramValues[2] = fileStr.c_str();
                paramValues[3] = timestampStr.c_str();
                paramValues[4] = offsetStr.c_str();

                PGresult *res = nullptr;
                int retryCount = 0;
                const int maxRetries = 5;
                const int retryDelayMs = 1000;

                while (retryCount < maxRetries && !*isInterrupted)
                {
                    res = PQexecParams(dbConnect, query.c_str(), 5, nullptr, paramValues, nullptr, nullptr, 0);

                    if (PQresultStatus(res) == PGRES_COMMAND_OK)
                    {
                        PQclear(res);
                        res = nullptr;
                        print(LogType::DEBUGER, "Offset from " + cameraID + " for " + std::to_string(protoOffset.timestamp()) + " was sent to DB");
                        break;
                    }
                    else
                    {
                        print(LogType::ERROR, "Add data to table failed: " + std::string(PQerrorMessage(dbConnect)));
                        PQclear(res);
                        res = nullptr;
                        retryCount++;

                        if (retryCount < maxRetries && !*isInterrupted)
                        {
                            std::this_thread::sleep_for(std::chrono::milliseconds(retryDelayMs));
                        }
                        else
                        {
                            print(LogType::ERROR, "Failed to insert offset after " + std::to_string(maxRetries) + " retries, skipping");
                        }
                    }
                }

                // Ensure cleanup if interrupted
                if (res != nullptr)
                {
                    PQclear(res);
                }
            }

            messages->erase(cameraID);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(ArchiveManagerConstants::DELAY_MS));
    }

    print(LogType::DEBUGER, "<<< Stop store offsets thread");
}
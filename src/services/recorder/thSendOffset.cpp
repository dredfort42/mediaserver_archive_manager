#include "writer.hpp"

void sendOffset(Messenger *messenger,
                std::string topic,
                std::string cameraUUID,
                int64_t timestamp,
                int64_t offset,
                int folder,
                int file)
{
    if (DEBUG)
        print(LogType::INFO, ">>> Start send offset thread");

    proto::ProtoOffset protoOffset;
    protoOffset.set_timestamp(timestamp);
    protoOffset.set_offset(offset);
    protoOffset.set_folder(folder);
    protoOffset.set_file(file);

    std::string offsetString;
    protoOffset.SerializeToString(&offsetString);

    messenger->produceMessage(topic, cameraUUID, offsetString);

    if (DEBUG)
    {
        print(LogType::INFO, "Send offset: " + std::to_string(offset) + " for timestamp: " + std::to_string(timestamp));
        print(LogType::INFO, "<<< Stop send offset thread");
    }
}
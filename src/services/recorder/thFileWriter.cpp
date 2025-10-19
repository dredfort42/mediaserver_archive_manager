#include "recorder.hpp"

void writeAVPacketsToFile(Messenger *messenger,
                          std::string *topicForOffsets,
                          std::string *cameraUUID,
                          std::list<Messenger::packet_t> *avPackets,
                          std::string *storagePath,
                          int *fragmentLengthInSeconds)
{
    print(LogType::DEBUGER, ">>> Start writer thread");

    int64_t offset = 0;
    std::string fileName = "";
    std::string currentFileName = "";
    std::string filePath;
    std::ofstream avFile;
    std::ofstream jsonFile;

    while (!*messenger->getInterruptSignal())
    {

        if (avPackets->empty())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(ArchiveRecorderConstants::DELAY_MS_WHEN_NO_AVPACKETS));
            continue;
        }

        Messenger::packet_t packet;

        if (!avPackets->front().second.empty())
            packet = avPackets->front();
        else
            continue;

        proto::ProtoPacket protoPacket;
        protoPacket.ParseFromString(packet.second);

        bool iFrame = isIFrame(protoPacket.data().c_str(), protoPacket.size());

        if (iFrame)
        {
            // TODO: add offset to separate structure and send it in a separate thread
            // START FROM THIS POINT ON MONDAY

            //     std::thread thSendOffset(sendOffset,
            //                              messenger,
            //                              topicForOffsets,
            //                              cameraUUID,
            //                              packet.first,
            //                              offset,
            //                              std::atoi(filePath.c_str()),
            //                              std::atoi(currentFileName.c_str()));

            // sendOffset(*messenger,
            //            *topicForOffsets,
            //            *cameraUUID,
            //            packet.first,
            //            offset,
            //            std::atoi(filePath.c_str()),
            //            std::atoi(currentFileName.c_str()));
            fileName = std::to_string(ChronoName::getFileName(avPackets->front().first, *fragmentLengthInSeconds));
            //     thSendOffset.detach();
        }

        if (currentFileName != fileName && iFrame)
        {
            if (jsonFile.is_open())
            {
                jsonFile << "\n]";
                jsonFile.close();
            }

            if (avFile.is_open())
            {
                avFile.close();
                offset = 0;
            }

            currentFileName = fileName;
            filePath = std::to_string(ChronoName::getDaysSinceEpoch(avPackets->front().first));
            std::filesystem::create_directories(*storagePath + "/" + *cameraUUID + "/" + filePath);

            avFile = std::ofstream(*storagePath + "/" + *cameraUUID + "/" + filePath + "/" + currentFileName, std::ios::binary | std::ios::app);
            jsonFile = std::ofstream(*storagePath + "/" + *cameraUUID + "/" + filePath + "/" + currentFileName + ".json", std::ios::binary | std::ios::app);

            if (jsonFile.is_open())
                jsonFile << "[\n";
        }

        if (iFrame && jsonFile.is_open() && offset)
            jsonFile << ",\n";

        if (iFrame && jsonFile.is_open())
            jsonFile << "\t{\n"
                     << "\t\t\"timestamp\":\"" << avPackets->front().first << "\",\n"
                     << "\t\t\"offset\":\"" << offset << "\",\n"
                     << "\t\t\"path\":\"" << filePath + "/" + currentFileName << "\""
                     << "\n\t}";

        if (avFile.is_open())
        {
            avFile.write(protoPacket.data().c_str(), protoPacket.size());
            offset += protoPacket.size();
        }

        // LogTable table("/// AV PACKET WRITTEN TO FILE");
        // table.addRow("Day", std::to_string(ChronoName::getDaysSinceEpoch(packet.first)));
        // table.addRow("File", currentFileName);
        // table.addRow("Offset", std::to_string(offset));
        // table.printLogTable(LogType::DEBUGER);

        protoPacket.Clear();
        avPackets->pop_front();
    }

    if (jsonFile.is_open())
    {
        jsonFile << "\n]";
        jsonFile.close();
    }

    if (avFile.is_open())
        avFile.close();

    print(LogType::DEBUGER, "<<< Stop writer thread");
}
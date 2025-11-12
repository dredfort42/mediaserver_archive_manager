#include "manager.hpp"

struct CameraArchiveInfo
{
    ArchiveParameters archive; // main stream only
    uint8_t statusCode;
};

std::string getConnectioTypeDescription(int connectionType)
{
    switch (connectionType)
    {
    case CONNECTION_TYPE_ACTIVE_PERMANENT:
        return "ACTIVE Permanent";
    case CONNECTION_TYPE_ACTIVE_ON_DEMAND:
        return "ACTIVE OnDemand";
    case CONNECTION_TYPE_STANDBY:
        return "STANDBY";
    default:
        return "UNKNOWN CONNECTION TYPE";
    }
}

void printCameraMessage(const std::string &cameraUuid, proto::ProtoCamera &protoCamera)
{
    if (!getDebug())
        return;

    LogTable table("/// RECEIVED CAMERA DATA");
    table.addRow("Camera Connector UUID", protoCamera.camera_connector_uuid());
    table.addRow("Camera UUID", cameraUuid);
    table.addRow("Main RTSP URL", protoCamera.main_rtsp_url());
    table.addRow("Main Connection Type", getConnectioTypeDescription(protoCamera.main_connection_type()));
    table.addRow("Sub RTSP URL", protoCamera.sub_rtsp_url());
    table.addRow("Sub Connection Type", getConnectioTypeDescription(protoCamera.sub_connection_type()));
    table.addRow("Status Code", protoCamera.status_code() == CONNECTION_STATUS_CODE_ON ? "ON" : "OFF");
    table.addRow("Archive Retention Days", std::to_string(protoCamera.archive_retention_days()));
    table.printLogTable(LogType::DEBUGER);
}

void printAddedArchiveInfo(const std::string &streamUUID, ArchiveParameters &archive)
{
    if (!getDebug())
        return;

    LogTable table("/// STREAM ADDED");
    table.addRow("Stream UUID", streamUUID);
    table.addRow("Archive Retention Days", std::to_string(archive.getArchiveRetentionDays()));
    table.printLogTable(LogType::DEBUGER);
}

void printRemovedArchiveInfo(const std::string &streamUUID)
{
    if (!getDebug())
        return;

    LogTable table("/// STREAM REMOVED");
    table.addRow("Stream UUID", streamUUID);
    table.printLogTable(LogType::DEBUGER);
}

void waitingForCamerasData(volatile sig_atomic_t *isInterrupted, Messenger::messenger_content_t *messengerContent, std::string &camerasTopic)
{
    while (!*isInterrupted && messengerContent->find(camerasTopic) == messengerContent->end())
    {
        print(LogType::DEBUGER, "Waiting for cameras topic...");
        std::this_thread::sleep_for(std::chrono::milliseconds(ArchiveManagerConstants::DELAY_MS));
    }
}

CameraArchiveInfo getCameraArchiveInfo(const std::string &cameraUuid, const std::string &message)
{
    CameraArchiveInfo CAI;

    try
    {
        proto::ProtoCamera protoCamera;

        if (!protoCamera.ParseFromString(message))
            throw std::runtime_error("Error parsing ProtoCamera");

        printCameraMessage(cameraUuid, protoCamera);

        CAI.statusCode = protoCamera.status_code();

        if (!protoCamera.main_rtsp_url().empty())
            CAI.archive = ArchiveParameters(cameraUuid + "_main",
                                            protoCamera.archive_retention_days());
    }
    catch (const std::exception &e)
    {
        print(LogType::ERROR, "Error parsing camera protoData: " + std::string(e.what()));
    }

    return CAI;
}

void addArchiveToMap(std::map<std::string, ArchiveParameters> *archivesToManage, std::mutex *archivesToManageMx, ArchiveParameters archive)
{
    if (archivesToManage->find(archive.getStreamUUID()) == archivesToManage->end())
        archivesToManage->insert(std::pair<std::string, ArchiveParameters>(archive.getStreamUUID(), archive));
    else
    {
        archivesToManageMx->lock();
        archivesToManage->at(archive.getStreamUUID()) = archive;
        archivesToManageMx->unlock();
    }

    printAddedArchiveInfo(archive.getStreamUUID(), archive);
}

void removeArchiveFromMap(std::map<std::string, ArchiveParameters> *archivesToManage, std::mutex *archivesToManageMx, ArchiveParameters *archive)
{
    if (archivesToManage->find(archive->getStreamUUID()) != archivesToManage->end())
    {
        archivesToManageMx->lock();
        archivesToManage->erase(archive->getStreamUUID());
        archivesToManageMx->unlock();

        printRemovedArchiveInfo(archive->getStreamUUID());
    }
}

void processArchive(std::map<std::string, ArchiveParameters> *archivesToManage, std::mutex *archivesToManageMx, ArchiveParameters *archive)
{
    if (archive->getStreamUUID().empty() || !archive->getArchiveRetentionDays())
    {
        removeArchiveFromMap(archivesToManage, archivesToManageMx, archive);
        print(LogType::DEBUGER, "Archive " + archive->getStreamUUID() + " is in STANDBY or has empty RTSP URL, removing it from the map if exists.");
    }
    else
    {
        addArchiveToMap(archivesToManage, archivesToManageMx, *archive);
        print(LogType::DEBUGER, "Archive " + archive->getStreamUUID() + " is ACTIVE, adding/updating it in the map.");
    }
}

void readCameras(volatile sig_atomic_t *isInterrupted,
                 Messenger::messenger_content_t *messengerContent,
                 MessengerConfig *messengerConfig,
                 std::map<std::string, ArchiveParameters> *archivesToManage,
                 std::mutex *archivesToManageMx)
{
    print(LogType::DEBUGER, ">>> Start read cameras thread");

    print(LogType::DEBUGER, "Cameras topic: " + messengerConfig->topicCameras);
    std::string camerasTopic = messengerConfig->topicCameras;

    waitingForCamerasData(isInterrupted, messengerContent, camerasTopic);

    while (!*isInterrupted)
    {
        // Safely check if topic still exists before accessing it
        auto topicIt = messengerContent->find(camerasTopic);
        if (topicIt == messengerContent->end())
        {
            print(LogType::WARNING, "Cameras topic disappeared, waiting...");
            waitingForCamerasData(isInterrupted, messengerContent, camerasTopic);
            continue;
        }

        // Lock the map mutex to safely iterate and modify
        std::lock_guard<std::mutex> mapLock(topicIt->second.mapMutex);
        Messenger::messages_map_t *messages = &topicIt->second.map;

        for (auto it = messages->begin(); !*isInterrupted && it != messages->end();)
        {
            std::string cameraUUID = it->first;
            std::string messageData;

            // Lock the individual message mutex to read its content
            {
                std::lock_guard<std::mutex> msgLock(it->second.mutex);
                messageData = it->second.message;
            }

            if (!messageData.empty())
            {
                CameraArchiveInfo CAI = getCameraArchiveInfo(cameraUUID, messageData);

                print(LogType::DEBUGER, "Processing archive for camera with UUID: " + cameraUUID);

                if (CAI.statusCode == CONNECTION_STATUS_CODE_OFF || CAI.archive.getArchiveRetentionDays() == 0)
                    removeArchiveFromMap(archivesToManage, archivesToManageMx, &CAI.archive);
                else
                    processArchive(archivesToManage, archivesToManageMx, &CAI.archive);
            }

            it = messages->erase(it);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(ArchiveManagerConstants::DELAY_MS));
    }

    print(LogType::DEBUGER, "<<< Stop read cameras thread");
}

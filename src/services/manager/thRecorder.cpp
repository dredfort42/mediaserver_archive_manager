#include "manager.hpp"

void startWriter(Archive *archive, ConfigMap *config)
{
    if (archive->getStatusCode() == CONNECTION_STATUS_CODE_ON && archive->getArchiveRetentionDays())
    {
        try
        {
            char command[] = "./recorder";
            char *cameraUUID = strdup(archive->getStreamUUID().c_str());
            char *path = strdup(config->getProperty("archive.archiver.storagePath").c_str());                 // TO CHECK
            char *fragment = strdup(config->getProperty("archive.archiver.fragmentLengthInSeconds").c_str()); // TO CHECK
            char *args[] = {command, cameraUUID, path, fragment, nullptr};

            pid_t pid = fork();

            switch (pid)
            {
            case -1:
                print(LogType::ERROR, "Error creating worker process...");
                break;
            case 0:
                if (execvp(command, args) == -1)
                {
                    print(LogType::ERROR, "Error executing worker process..." + cameraUUID);
                    break;
                }
                break;
            default:
                camera->setPID(pid);
            }

            archive->getInfo();

            free(streamUUID);
            free(path);
            free(fragment);

            if (DEBUG)
                archive->getInfo();
        }
        catch (const std::exception &e)
        {
            print(LogType::ERROR, "Strdup alloc:  " + std::string(e.what()));
        }
    }
}

void stopWriter(Camera *camera)
{
    if (camera->getPID() > 0)
        kill(camera->getPID(), SIGTERM);
}

// void readCameras(volatile sig_atomic_t *wasInrerrupted,
//                  Messenger::messenger_content_t *messengerContent,
//                  std::map<std::string, Camera> *cameras,
//                  ConfigMap *config)
// {
//     if (DEBUG)
//         print(LogType::INFO, ">>> Start camera thread");

//     std::string camerasTopic = config->getProperty("kafka.topic.compact.camerasData");

//     while (!*wasInrerrupted)
//     {
//         if (messengerContent->find(camerasTopic) != messengerContent->end())
//         {
//             for (auto camera : messengerContent->at(camerasTopic))
//             {
//                 Camera jCamera;

//                 try
//                 {
//                     Entry parsedJson = nlohmann::json::parse(std::string(camera.second));
//                     jCamera = Camera(parsedJson.UUID,
//                                      parsedJson.MainRTSPURL,
//                                      parsedJson.SubRTSPURL,
//                                      parsedJson.StreamingType,
//                                      parsedJson.StatusCode,
//                                      parsedJson.Archive);
//                 }
//                 catch (const std::exception &e)
//                 {
//                     print(LogType::ERROR, "Error parsing JSON: " + std::string(e.what()));
//                     return;
//                 }

//                 if (cameras->find(jCamera.getUUID()) == cameras->end())
//                 {
//                     print(LogType::WARNING, "Camera added: " + jCamera.getUUID());
//                     startWriter(&jCamera, config);
//                     cameras->insert(std::pair<std::string, Camera>(jCamera.getUUID(), jCamera));
//                 }
//                 else if (cameras->at(jCamera.getUUID()) != jCamera)
//                 {
//                     print(LogType::WARNING, "Camera updated: " + jCamera.getUUID());
//                     stopWriter(&(cameras->at(jCamera.getUUID())));
//                     startWriter(&jCamera, config);
//                     cameras->at(jCamera.getUUID()) = jCamera;
//                 }
//             }
//         }

//         std::this_thread::sleep_for(std::chrono::milliseconds(1000));
//     }

//     if (DEBUG)
//         print(LogType::INFO, "<<< Stop camera thread");
// }

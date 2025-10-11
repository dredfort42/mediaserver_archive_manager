#include "manager.hpp"

std::atomic<ServiceStatus> serviceDigest(ServiceStatus::STARTING);

static volatile sig_atomic_t isInterrupted = 0;

static void sigterm(int sig)
{
    isInterrupted = 1;
    serviceDigest = ServiceStatus::STOPPING;
    print(serviceDigest);
}

int main(int argc, char *argv[])
{
    print(ServiceStatus::STARTING);

    signal(SIGINT, sigterm);
    signal(SIGTERM, sigterm);

    ConfigMap config(argc, argv);
    setDebug(config.getProperty("debug") == "true" || config.getProperty("debug") == "1");

    config.printConfiguration();

    ArchiveManagerConfig archiveManagerConfig = getArchiveManagerConfig(&config);
    DatabaseConfig databaseConfig = getDatabaseConfig(&config);
    MessengerConfig messengerConfig = getMessengerConfig(&config);
    std::string error;

    openlog(archiveManagerConfig.appName.c_str(), LOG_PID | LOG_CONS, LOG_USER);

    Messenger messenger(&isInterrupted);
    Messenger::topics_t topics;
    Messenger::messenger_content_t messengerContent;
    std::map<std::string, Archive> archivesToManage; // <streamUUID, Archive>
    std::mutex archivesToManageMx;

    std::string err = initMessenger(messenger, messengerConfig, topics, archiveManagerConfig.appUUID);
    if (!err.empty())
    {
        print(ServiceStatus::ERROR);
        print(LogType::ERROR, err);
        print(ServiceStatus::STOPPING);
        print(ServiceStatus::STOPPED);
        return RTN_ERROR;
    }

    PGconn *dbConnect = nullptr;

    error = initDatabase(dbConnect, databaseConfig);
    if (!error.empty())
    {
        print(ServiceStatus::ERROR);
        print(LogType::ERROR, error);
        print(ServiceStatus::STOPPING);
        print(ServiceStatus::STOPPED);
        return RTN_ERROR;
    }

    produceServiceDigest(messenger,
                         messengerConfig.topicSystemDigest,
                         archiveManagerConfig,
                         serviceDigest);

    std::thread thConsumer(consumeMessages, &messenger, &messengerContent, &topics);

    std::thread thCameras(readCameras,
                          &isInterrupted,
                          &messengerContent,
                          &messengerConfig,
                          &archivesToManage,
                          &archivesToManageMx);

    //---

    // Messenger::topics_t topics;
    // Messenger::messenger_content_t messengerContent;
    // std::map<std::string, Camera> cameras;
    // std::map<std::string, ArchiveTask> tasks;
    // uint currentTasksQueueSize = 0;

    // std::thread thConsumer(consumeMessages,
    //                        &wasInrerrupted,
    //                        &messengerContent,
    //                        &topics,
    //                        &config);

    // std::thread thCameras(readCameras,
    //                       &wasInrerrupted,
    //                       &messengerContent,
    //                       &cameras,
    //                       &config);

    // std::thread thTasks(readTasks,
    //                     &wasInrerrupted,
    //                     dbConnect,
    //                     &messengerContent,
    //                     &tasks,
    //                     &currentTasksQueueSize,
    //                     &config);

    // std::thread thCamerasController(controlCamerasWorkers,
    //                                 &wasInrerrupted,
    //                                 &cameras);

    // std::thread thTasksController(controlTasksWorkers,
    //                               &wasInrerrupted,
    //                               &tasks,
    //                               &currentTasksQueueSize);

    // std::thread thDatabse(storeOffsets,
    //                       &wasInrerrupted,
    //                       dbConnect,
    //                       &messengerContent,
    //                       &config);

    // print(LogType::WARNING, "Archive process with pid " + std::to_string(getpid()) + " stopped successfully");

    std::this_thread::sleep_for(std::chrono::seconds(1));
    serviceDigest = ServiceStatus::READY;
    print(serviceDigest);
    produceServiceDigest(messenger,
                         messengerConfig.topicSystemDigest,
                         archiveManagerConfig,
                         serviceDigest);

    std::chrono::time_point<std::chrono::system_clock> start = std::chrono::system_clock::now();

    while (!isInterrupted)
    {
        std::chrono::time_point<std::chrono::system_clock> time = std::chrono::system_clock::now();
        if (std::chrono::duration_cast<std::chrono::milliseconds>(time - start).count() >= ArchiveManagerConstants::STATE_POST_INTERVAL_MS)
        {
            start = time;
            // print(serviceDigest);
            produceServiceDigest(messenger,
                                 messengerConfig.topicSystemDigest,
                                 archiveManagerConfig,
                                 serviceDigest);
        }
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    thConsumer.join();
    // thCameras.join();
    // thTasks.join();
    // thCamerasController.join();
    // thTasksController.join();
    // thDatabase.join();

    serviceDigest = ServiceStatus::STOPPED;
    print(serviceDigest);
    produceServiceDigest(messenger,
                         messengerConfig.topicSystemDigest,
                         archiveManagerConfig,
                         serviceDigest);

    closeDatabase(dbConnect);
    closelog();

    return RTN_SUCCESS;
}
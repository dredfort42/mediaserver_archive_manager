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

    GOOGLE_PROTOBUF_VERIFY_VERSION;

    signal(SIGINT, sigterm);
    signal(SIGTERM, sigterm);

    ConfigMap config(argc, argv);
    config.printConfiguration();

    ArchiveManagerConfig archiveManagerConfig = getArchiveManagerConfig(&config);
    DatabaseConfig databaseConfig = getDatabaseConfig(&config);
    MessengerConfig messengerConfig = getMessengerConfig(&config);
    std::string error;

    openlog(archiveManagerConfig.appName.c_str(), LOG_PID | LOG_CONS, LOG_USER);

    Messenger messenger(&isInterrupted);
    Messenger::topics_t topics;
    Messenger::messenger_content_t messengerContent;
    std::map<std::string, ArchiveParameters> archivesToManage; // <streamUUID, ArchiveParameters>
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

    print(LogType::INFO, "Connected to Kafka brokers: " + messengerConfig.brokers);

    PGconn *dbConnect = initDatabase(databaseConfig);
    if (dbConnect == nullptr)
    {
        print(ServiceStatus::ERROR);
        print(LogType::ERROR, "Connection to database failed");
        print(ServiceStatus::STOPPING);
        print(ServiceStatus::STOPPED);
        return RTN_ERROR;
    }

    print(LogType::INFO, "Connected to database: " + databaseConfig.dbName);

    if (!isTableExists(dbConnect, databaseConfig.table_iframe_byte_offsets))
    {
        print(ServiceStatus::ERROR);
        print(LogType::ERROR, "Required table '" + databaseConfig.table_iframe_byte_offsets + "' does not exist in database");
        print(ServiceStatus::STOPPING);
        print(ServiceStatus::STOPPED);
        closeDatabase(dbConnect);
        return RTN_ERROR;
    }

    print(LogType::INFO, "Required table '" + databaseConfig.table_iframe_byte_offsets + "' exists in database");

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

    std::thread thRecorder(recorderController,
                           &isInterrupted,
                           &serviceDigest,
                           &archiveManagerConfig,
                           &archivesToManage,
                           &archivesToManageMx);

    std::thread thDatabase(storeOffsets,
                           &isInterrupted,
                           dbConnect,
                           &messengerContent,
                           &messengerConfig.topicIFrameByteOffsets,
                           &databaseConfig.table_iframe_byte_offsets);

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
    thCameras.join();
    thRecorder.join();
    thDatabase.join();

    // thTasks.join();
    // thCamerasController.join();
    // thTasksController.join();

    serviceDigest = ServiceStatus::STOPPED;
    print(serviceDigest);
    produceServiceDigest(messenger,
                         messengerConfig.topicSystemDigest,
                         archiveManagerConfig,
                         serviceDigest);

    google::protobuf::ShutdownProtobufLibrary();
    closeDatabase(dbConnect);
    closelog();

    return RTN_SUCCESS;
}
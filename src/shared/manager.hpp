#ifndef MANAGER_HPP
#define MANAGER_HPP

#ifndef DEBUG
#define DEBUG 0
#endif

#include <csignal>
// #include <nlohmann/json.hpp>
#include <thread>
#include <libpq-fe.h>
#include <chrono>

#include "logPrinter.hpp"
#include "configMap.hpp"
// #include "archiveTask.hpp"
#include "archive.hpp"
// #include "chronoName.hpp"
#include "messenger.hpp"
#include "protoData.pb.h"

#define RTN_ERROR -1
#define RTN_SUCCESS 0

#define CONNECTION_STATUS_CODE_ON 1
#define CONNECTION_STATUS_CODE_OFF 0

namespace ArchiveManagerConstants
{
    constexpr int DELAY_MS = 1000;
    constexpr int STATE_POST_INTERVAL_MS = 10 * 1000;
}

/*
 * @brief The archive manager configuration
 */
struct ArchiveManagerConfig
{
    std::string appName;
    std::string appVersion;
    std::string appUUID;
    std::string configPath;
    std::string readerPath;
    std::string writerPath;
    std::string archivePath;
};

/*
 * @brief Get the archive manager configuration
 * @param config The configuration map
 * @return The archive manager configuration
 */
ArchiveManagerConfig getArchiveManagerConfig(ConfigMap *config);

/*
 * @brief The PostgreSQL database configuration
 */
struct DatabaseConfig
{
    std::string host;
    std::string port;
    std::string dbName;
    std::string user;
    std::string password;
    std::string sslMode;
};

/*
 * @brief Get the database configuration
 * @param config The configuration map
 * @return The database configuration
 */
DatabaseConfig getDatabaseConfig(ConfigMap *config);

/*
 * @brief Initialize the database
 * @param dbConnect The database connection
 * @param dbConfig The database configuration
 * @return The error message or empty string if success
 */
std::string initDatabase(PGconn *dbConnect, DatabaseConfig dbConfig);

/*
 * @brief Close the database connection
 * @param dbConnect The database connection
 */
void closeDatabase(PGconn *dbConnect);

/*
 * @brief The Messenger configuration
 */
struct MessengerConfig
{
    // Kafka configuration
    std::string brokers;

    // Connection manager IDs
    std::string clientId;
    std::string groupId;

    // Topics
    std::string topicSystemDigest;
    std::string topicCameras;
    std::string topicIFrameByteOffsets;
};

/*
 * @brief Get the messenger configuration
 * @param config The configuration map
 * @return The messenger configuration
 */
MessengerConfig getMessengerConfig(ConfigMap *config);

/*
 * @brief Initialize the messenger
 * @param messenger The messenger
 * @param messengerConfig The messenger configuration
 * @param topics The topics
 * @param appUUID The application UUID
 * @return The error message or empty string if success
 */
std::string initMessenger(Messenger &messenger,
                          MessengerConfig &messengerConfig,
                          Messenger::topics_t &topics,
                          std::string &appUUID);

/*
 * @brief Produce service state
 * @param messenger The messenger
 * @param topic The topic for the message the service state
 * @param archiveManagerConfig The archive manager configuration
 * @param statusCode The service status code
 */
void produceServiceDigest(Messenger &messenger,
                          std::string &topic,
                          const ArchiveManagerConfig &archiveManagerConfig,
                          ServiceStatus statusCode);

/*
 * @brief The Kafka consumer thread
 * @param messenger The messenger
 * @param messengerContent The messenger content
 * @param topics The topics to consume
 */
void consumeMessages(Messenger *messenger,
                     Messenger::messenger_content_t *messengerContent,
                     Messenger::topics_t *topics);

// ---

// /*
//  * @brief The struct that matches the JSON camera entry structure
//  */
// struct Entry
// {
//     std::string UUID;
//     std::string MainRTSPURL;
//     std::string SubRTSPURL;
//     int StreamingType;
//     int StatusCode;
//     int Archive;
// };

// /*
//  * @brief The specialization of the nlohmann::adl_serializer for the Entry struct
//  */
// namespace nlohmann
// {
//     template <>
//     struct adl_serializer<Entry>
//     {
//         static void to_json(json &j, const Entry &e)
//         {
//             j = json{{"uuid", e.UUID},
//                      {"main_rtsp_url", e.MainRTSPURL},
//                      {"sub_rtsp_url", e.SubRTSPURL},
//                      {"streaming_type", e.StreamingType},
//                      {"archive", e.Archive}};
//         }

//         static void from_json(const json &j, Entry &e)
//         {
//             j.at("uuid").get_to(e.UUID);
//             j.at("main_rtsp_url").get_to(e.MainRTSPURL);
//             j.at("sub_rtsp_url").get_to(e.SubRTSPURL);
//             j.at("streaming_type").get_to(e.StreamingType);
//             j.at("status_code").get_to(e.StatusCode);
//             j.at("archive").get_to(e.Archive);
//         }
//     };
// }

// /*
//  * @brief The Kafka consumer thread
//  * @param wasInrerrupted The flag to stop the thread
//  * @param messengerContent The messenger content
//  * @param topics The topics to subscribe
//  * @param config The configuration map
//  */
// void consumeMessages(volatile sig_atomic_t *wasInrerrupted,
//                      Messenger::messenger_content_t *messengerContent,
//                      Messenger::topics_t *topics,
//                      ConfigMap *config);

/*
 * @brief The read cameras configuration thread
 * @param isInterrupted The flag to stop the thread
 * @param messengerContent The messenger content
 * @param messengerConfig The messenger configuration
 * @param streamsToArchive The map of streams to archive <streamUUID, Archive>
 * @param streamsToArchiveMx The mutex for the streams to archive map
 */
void readCameras(volatile sig_atomic_t *isInterrupted,
                 Messenger::messenger_content_t *messengerContent,
                 MessengerConfig *messengerConfig,
                 std::map<std::string, Archive> *streamsToArchive,
                 std::mutex *streamsToArchiveMx);

// /*
//  * @brief The read tasks configuration thread
//  * @param wasInrerrupted The flag to stop the thread
//  * @param dbConnect The connection to PostgreSQL
//  * @param tasksTopic The topic name with tasks info
//  * @param messengerContent The messenger content
//  * @param tasks The tasks map
//  * @param currentTasksQueueSize The current tasks queue size
//  * @param config The configuration map
//  */
// void readTasks(volatile sig_atomic_t *wasInrerrupted,
//                PGconn *dbConnect,
//                Messenger::messenger_content_t *messengerContent,
//                std::map<std::string, ArchiveTask> *tasks,
//                uint *currentTasksQueueSize,
//                ConfigMap *config);

// /*
//  * @brief The control cameras workers thread
//  * @param wasInrerrupted The flag to stop the thread
//  * @param cameras The cameras map
//  */
// void controlCamerasWorkers(volatile sig_atomic_t *wasInrerrupted,
//                            std::map<std::string, Camera> *cameras);

// /*
//  * @brief The control tasks workers thread
//  * @param wasInrerrupted The flag to stop the thread
//  * @param tasks The tasks map
//  * @param currentTasksQueueSize The current tasks queue size
//  */
// void controlTasksWorkers(volatile sig_atomic_t *wasInrerrupted,
//                          std::map<std::string, ArchiveTask> *tasks,
//                          uint *currentTasksQueueSize);

// /*
//  * @brief The store offsets in database thread
//  * @param wasInrerrupted The flag to stop the thread
//  * @param dbConnect The connection to PostgreSQL
//  * @param messengerContent The messenger content
//  * @param config The configuration map
//  */
// void storeOffsets(volatile sig_atomic_t *wasInrerrupted,
//                   PGconn *dbConnect,
//                   Messenger::messenger_content_t *messengerContent,
//                   ConfigMap *postgresConfig);

#endif // MANAGER_HPP
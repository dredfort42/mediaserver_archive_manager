#ifndef WORKER_HPP
#define WORKER_HPP

#include <csignal>
#include <chrono>
#include <unistd.h>
#include <arpa/inet.h>
#include <string>
#include <thread>
#include <syslog.h>

#include "protoData.pb.h"
#include "logPrinter.hpp"
#include "configMap.hpp"
#include "chronoName.hpp"
#include "frameParser.hpp"
#include "messenger.hpp"

#define RTN_ERROR -1
#define RTN_SUCCESS 0

namespace ArchiveRecorderConstants
{
    constexpr int DELAY_MS = 10;
    constexpr int64_t DELAY_MS_WHEN_NO_AVPACKETS = 100;
    // constexpr int STATE_POST_INTERVAL_MS = 10 * 1000;
    // constexpr size_t MAX_AVPACKET_BUFFER_SIZE = 1000;
}

struct RecorderConfig
{
    std::string appName;
    std::string appUUID;
    std::string storagePath;
    int fragmentLengthInSeconds;
};

/**
 * @brief Get the recorder configuration
 * @param config The configuration map
 * @return The recorder configuration
 */
RecorderConfig getRecorderConfig(ConfigMap *config);

// ----

/**
 * @brief The Messenger configuration
 */
struct MessengerConfig
{
    // Kafka configuration
    std::string brokers;

    // Recorder IDs
    std::string clientId;
    std::string groupId;

    // Topics
    std::string topicIFrameByteOffsets;
};

/**
 * @brief Get the messenger configuration
 * @param config The configuration map
 * @return The messenger configuration
 */
MessengerConfig getMessengerConfig(ConfigMap *config);

/**
 * @brief Initialize the messenger
 * @param messenger The messenger
 * @param messengerConfig The messenger configuration
 * @param appUUID The application UUID
 * @return The error message or empty string if success
 */
std::string initMessenger(Messenger &messenger,
                          MessengerConfig &messengerConfig,
                          std::string &appUUID);

/**
 * @brief The offset producer function
 * @param messenger The messenger object
 * @param topic The topic to produce file offsets
 * @param cameraUUID The camera UUID
 * @param timestamp The timestamp of the frame
 * @param offset The offset of the iFrame since the beginning of the file
 * @param folder The folder name
 * @param file The file name
 */
void produceOffset(Messenger *messenger,
                   std::string *topic,
                   std::string *cameraUUID,
                   int64_t timestamp,
                   int64_t offset,
                   int folder,
                   int file);

/**
 * @brief The Kafka AVPackets consumer thread
 * @param messenger The messenger
 * @param avPackets The list of AV packets
 * @param topic The topic to consume
 */
void consumeAVPackets(Messenger *messenger,
                      std::list<Messenger::packet_t> *avPackets,
                      std::string *topic);

/**
 * @brief The file writer thread
 * @param messenger The messenger object
 * @param topic The topic to produce file offsets
 * @param cameraUUID The camera UUID
 * @param packets The list of consumed packets
 * @param storagePath The path to the storage
 * @param fragmentLengthInSeconds The length of the fragment in seconds
 */
void writeAVPacketsToFile(Messenger *messenger,
                          std::string *topic,
                          std::string *cameraUUID,
                          std::list<Messenger::packet_t> *avPackets,
                          std::string *storagePath,
                          int *fragmentLengthInSeconds);

#endif // WORKER_HPP
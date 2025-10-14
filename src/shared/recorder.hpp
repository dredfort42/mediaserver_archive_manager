#ifndef WORKER_HPP
#define WORKER_HPP

#ifndef DEBUG
#define DEBUG 0
#endif

#include <csignal>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <string>
#include <chrono>

#include "protoData.pb.h"
#include "logPrinter.hpp"
// #include "configMap.hpp"
// #include "chronoName.hpp"
// #include "frameParser.hpp"
#include "messenger.hpp"

/*
 * @brief The Kafka consumer thread
 * @param messenger The messenger object
 * @param messengerContent The list of consumed messages
 * @param topic The topic to subscribe
 */
void startConsumer(Messenger *messenger,
                   std::list<Messenger::packet_t> *packets,
                   std::string topic);

/*
 * @brief The file writer thread
 * @param messenger The messenger object
 * @param topic The topic to produce file offsets
 * @param cameraUUID The camera UUID
 * @param packets The list of consumed packets
 * @param storagePath The path to the storage
 * @param fragmentLengthInSeconds The length of the fragment in seconds
 */
void fileWriter(Messenger *messenger,
                std::string topic,
                std::string cameraUUID,
                std::list<Messenger::packet_t> *packets,
                std::string storagePath,
                int fragmentLengthInSeconds);

/*
 * @brief The offset sender function
 * @param messenger The messenger object
 * @param topic The topic to produce file offsets
 * @param cameraUUID The camera UUID
 * @param timestamp The timestamp of the frame
 * @param offset The offset of the iFrame since the beginning of the file
 * @param folder The folder name
 * @param file The file name
 */
void sendOffset(Messenger *messenger,
                std::string topic,
                std::string cameraUUID,
                int64_t timestamp,
                int64_t offset,
                int folder,
                int file);

#endif // WORKER_HPP
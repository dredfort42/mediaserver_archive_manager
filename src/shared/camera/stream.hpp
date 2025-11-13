#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <iostream>
#include <string>
#include <string.h>
#include <unistd.h>
#include <csignal>
#include <sys/types.h>
#include <sys/wait.h>

#include "logPrinter.hpp"

#define CONNECTION_TYPE_STANDBY 0
#define CONNECTION_TYPE_ACTIVE_ON_DEMAND 1
#define CONNECTION_TYPE_ACTIVE_PERMANENT 8

/**
 * @brief The stream class
 */
class Stream
{
private:
    std::string _streamUUID; // stream type (main_/sub_) + camera UUID
    std::string _RTSPURL;
    uint8_t _connectionType; // 0 - On demand [StandBy], 1 - On demand [Active], 8 - Always on
    pid_t _pid;

public:
    Stream();

    Stream(std::string streamUUID,
           std::string RTSPURL,
           uint8_t connectionType);

    ~Stream();

    bool operator==(const Stream &c);
    bool operator!=(const Stream &c);

    pid_t getPID() const;
    std::string getStreamUUID() const;
    std::string getRTSPURL() const;
    std::string getConnectioTypeDescription() const;
    uint8_t getConnectionType() const;
    void printInfo() const;

    void setPID(pid_t pid);
};

#endif
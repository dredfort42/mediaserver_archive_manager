#ifndef ARCHIVE_PARAMETERS_HPP
#define ARCHIVE_PARAMETERS_HPP

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

/*
 * @brief The archive class
 */
class ArchiveParameters
{
private:
    std::string _streamUUID; // stream type (_main) + camera UUID
    uint32_t _archiveRetentionDays;
    pid_t _pid;

public:
    ArchiveParameters();

    ArchiveParameters(std::string streamUUID,
                      uint32_t archiveRetentionDays);

    ~ArchiveParameters();

    bool operator==(const ArchiveParameters &c);
    bool operator!=(const ArchiveParameters &c);

    pid_t getPID() const;
    std::string getStreamUUID() const;
    uint32_t getArchiveRetentionDays() const;
    void printInfo() const;

    void setPID(pid_t pid);
};

#endif // ARCHIVE_PARAMETERS_HPP
#include "archiveParameters.hpp"

ArchiveParameters::ArchiveParameters()
{
    _streamUUID = "";
    _archiveRetentionDays = 0;
    _pid = 0;
}

ArchiveParameters::ArchiveParameters(std::string streamUUID,
                                     uint32_t archiveRetentionDays)
{
    _streamUUID = streamUUID;
    _archiveRetentionDays = archiveRetentionDays;
    _pid = 0;
}

ArchiveParameters::~ArchiveParameters()
{
}

bool ArchiveParameters::operator==(const ArchiveParameters &c)
{
    return _streamUUID == c._streamUUID &&
           _archiveRetentionDays == c._archiveRetentionDays;
}

bool ArchiveParameters::operator!=(const ArchiveParameters &c)
{
    return !(*this == c);
}

pid_t ArchiveParameters::getPID() const
{
    return _pid;
}

std::string ArchiveParameters::getStreamUUID() const
{
    return _streamUUID;
}

uint32_t ArchiveParameters::getArchiveRetentionDays() const
{
    return _archiveRetentionDays;
}

void ArchiveParameters::printInfo() const
{
    if (!getDebug())
        return;

    LogTable streamInfoTable;
    streamInfoTable.setHeader("/// ARCHIVE PARAMETERS INFO");
    streamInfoTable.addRow("Stream UUID", _streamUUID);
    streamInfoTable.addRow("Archive Retention Days", std::to_string(_archiveRetentionDays));
    streamInfoTable.addRow("PID", std::to_string(_pid));
    streamInfoTable.printLogTable(LogType::DEBUGER);
}

void ArchiveParameters::setPID(pid_t pid)
{
    _pid = pid;
}
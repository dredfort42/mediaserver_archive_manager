#include "archive.hpp"

Archive::Archive()
{
    _streamUUID = "";
    _RTSPURL = "";
    _connectionType = CONNECTION_TYPE_STANDBY;
    _archiveRetentionDays = 0;
    _pid = 0;
}

Archive::Archive(std::string cameraUUID,
                 std::string RTSPURL,
                 uint8_t connectionType,
                 uint32_t archiveRetentionDays)
{
    _streamUUID = cameraUUID;
    _RTSPURL = RTSPURL;
    _connectionType = connectionType;
    _archiveRetentionDays = archiveRetentionDays;
    _pid = 0;
}

Archive::~Archive()
{
}

bool Archive::operator==(const Archive &c)
{
    return _streamUUID == c._streamUUID &&
           _RTSPURL == c._RTSPURL &&
           _connectionType == c._connectionType &&
           _archiveRetentionDays == c._archiveRetentionDays;
}

bool Archive::operator!=(const Archive &c)
{
    return !(*this == c);
}

pid_t Archive::getPID() const
{
    return _pid;
}

std::string Archive::getStreamUUID() const
{
    return _streamUUID;
}

std::string Archive::getRTSPURL() const
{
    return _RTSPURL;
}

uint8_t Archive::getConnectionType() const
{
    return _connectionType;
}

std::string Archive::getConnectioTypeDescription() const
{
    switch (_connectionType)
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

uint32_t Archive::getArchiveRetentionDays() const
{
    return _archiveRetentionDays;
}

void Archive::printInfo() const
{
    if (!getDebug())
        return;

    LogTable streamInfoTable;
    streamInfoTable.setHeader("/// STREAM INFO");
    streamInfoTable.addRow("Stream UUID", _streamUUID);
    streamInfoTable.addRow("RTSP URL", _RTSPURL);
    streamInfoTable.addRow("Connection Type", getConnectioTypeDescription());
    streamInfoTable.addRow("Archive Retention Days", std::to_string(_archiveRetentionDays));
    streamInfoTable.addRow("PID", std::to_string(_pid));
    streamInfoTable.printLogTable(LogType::DEBUGER);
}

void Archive::setPID(pid_t pid)
{
    _pid = pid;
}
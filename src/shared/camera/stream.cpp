#include "stream.hpp"

Stream::Stream()
{
    _streamUUID = "";
    _RTSPURL = "";
    _connectionType = CONNECTION_TYPE_STANDBY;
    _pid = 0;
}

Stream::Stream(std::string cameraUUID,
               std::string RTSPURL,
               uint8_t connectionType)
{
    _streamUUID = cameraUUID;
    _RTSPURL = RTSPURL;
    _connectionType = connectionType;
    _pid = 0;
}

Stream::~Stream()
{
}

bool Stream::operator==(const Stream &c)
{
    return _streamUUID == c._streamUUID &&
           _RTSPURL == c._RTSPURL &&
           _connectionType == c._connectionType;
}

bool Stream::operator!=(const Stream &c)
{
    return !(*this == c);
}

pid_t Stream::getPID() const
{
    return _pid;
}

std::string Stream::getStreamUUID() const
{
    return _streamUUID;
}

std::string Stream::getRTSPURL() const
{
    return _RTSPURL;
}

uint8_t Stream::getConnectionType() const
{
    return _connectionType;
}

std::string Stream::getConnectioTypeDescription() const
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

void Stream::printInfo() const
{
    if (!getDebug())
        return;

    LogTable streamInfoTable;
    streamInfoTable.setHeader("/// STREAM INFO");
    streamInfoTable.addRow("Stream UUID", _streamUUID);
    streamInfoTable.addRow("RTSP URL", _RTSPURL);
    streamInfoTable.addRow("Connection Type", getConnectioTypeDescription());
    streamInfoTable.addRow("PID", std::to_string(_pid));

    streamInfoTable.printLogTable(LogType::DEBUGER);
}

void Stream::setPID(pid_t pid)
{
    _pid = pid;
}
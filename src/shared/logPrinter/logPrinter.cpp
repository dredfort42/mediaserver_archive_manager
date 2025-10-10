#include "logPrinter.hpp"

std::mutex logMutex;
bool DEBUG = false;

void setDebug(bool debug)
{
    DEBUG = debug;
}

bool getDebug()
{
    return DEBUG;
}

void print(const LogType &type, const std::string &message)
{
    if (type == LogType::DEBUGER && !DEBUG)
        return;

    std::time_t now = std::time(nullptr);
    std::tm *localTime = std::localtime(&now);

    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S", localTime);

    std::string log = buffer;
    log += " ";

    switch (type)
    {
    case LogType::DEBUGER:
        syslog(LOG_DEBUG, "%s", message.c_str());
        log += BLUE;
        log += "DEBUG: ";
        log += RESET;
        break;
    case LogType::INFO:
        syslog(LOG_INFO, "%s", message.c_str());
        log += GREEN;
        log += "INFO: ";
        log += RESET;
        break;
    case LogType::WARNING:
        syslog(LOG_WARNING, "%s", message.c_str());
        log += YELLOW;
        log += "WARNING: ";
        log += RESET;
        break;
    case LogType::ERROR:
        syslog(LOG_ERR, "%s", message.c_str());
        log += RED;
        log += "ERROR: ";
        log += RESET;
        break;
    default:
        break;
    }

    log += message;

    std::unique_lock<std::mutex> lock(logMutex);
    std::cout << log << std::endl;
    lock.unlock();
}

void print(const std::string &message)
{
    std::time_t now = std::time(nullptr);
    std::tm *localTime = std::localtime(&now);

    char buffer[20];
    std::strftime(buffer, sizeof(buffer), "%Y/%m/%d %H:%M:%S", localTime);

    std::string log = buffer;
    log += " ";
    log += message;

    std::unique_lock<std::mutex> lock(logMutex);
    std::cout << log << std::endl;
    lock.unlock();
}

void print(ServiceStatus state)
{
    std::string stateName = "SERVICE ";
    switch (state)
    {
    case ServiceStatus::UNKNOWN:
        stateName += "UNKNOWN";
        break;
    case ServiceStatus::STARTING:
        stateName += "STARTING";
        break;
    case ServiceStatus::READY:
        stateName += "READY";
        break;
    case ServiceStatus::DEGRADED:
        stateName += "DEGRADED";
        break;
    case ServiceStatus::MAINTENANCE:
        stateName += "MAINTENANCE";
        break;
    case ServiceStatus::STOPPING:
        stateName += "STOPPING";
        break;
    case ServiceStatus::STOPPED:
        stateName += "STOPPED";
        break;
    case ServiceStatus::ERROR:
        stateName += "ERROR";
        break;
    default:
        stateName += "UNKNOWN";
        break;
    }

    std::string border = "  +-----+";
    for (size_t i = 0; i < stateName.length() + 2; i++)
        border += "-";
    border += "+";

    print("");
    print(border);
    print("  | " + std::to_string(static_cast<int>(state)) + " | " + stateName + " |");
    print(border);
    print("");
}

void LogTable::setHeader(const std::string &header)
{
    this->header = header;
}

void LogTable::addRow(const std::string &key, const std::string &value)
{
    if (key.length() > _maxKeyLength)
        _maxKeyLength = key.length();

    if (value.length() > _maxValueLength)
        _maxValueLength = value.length();

    _rows.push_back(std::make_pair(key, value));
}

void LogTable::printLogTable(const LogType &logType)
{
    maxLineLength -= 7;
    size_t maxKeyLength = maxLineLength / 2;
    size_t maxValueLength = maxLineLength - maxKeyLength;

    if (_maxKeyLength < maxKeyLength)
        maxKeyLength = _maxKeyLength;

    if (_maxValueLength < maxValueLength)
        maxValueLength = _maxValueLength;

    if (header.length() > size_t(maxLineLength + 3))
        header = header.substr(0, maxLineLength) + "...";
    else
        header = header + std::string(maxKeyLength + maxValueLength + 3 - header.length(), ' ');

    if (!header.empty())
    {
        print(logType, "  +" + std::string(maxKeyLength + maxValueLength + 5, '-') + "+");
        print(logType, "  | " + header + " |");
    }

    std::string border = "  +" + std::string(maxKeyLength + 2, '-') + "+" + std::string(maxValueLength + 2, '-') + "+";

    print(logType, border);

    if (!_rows.empty())
    {

        for (auto row : _rows)
        {
            std::string key = row.first;
            std::string value = row.second;

            if (key.length() > maxKeyLength)
                key = key.substr(0, maxKeyLength - 3) + "...";
            else
                key = key + std::string(maxKeyLength - key.length(), ' ');

            if (value.length() > maxValueLength)
                value = value.substr(0, maxValueLength - 3) + "...";
            else
                value = value + std::string(maxValueLength - value.length(), ' ');

            print(logType, "  | " + key + " | " + value + " |");
        }

        print(logType, border);
    }
}
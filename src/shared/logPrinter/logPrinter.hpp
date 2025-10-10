#ifndef LOG_PRINTER_HPP
#define LOG_PRINTER_HPP

#include <iostream>
#include <mutex>
#include <string>
#include <cstring>
#include <ctime>
#include <syslog.h>
#include <vector>

// Colors for the log types
#define RED "\033[1;31m"
#define GREEN "\033[1;32m"
#define YELLOW "\033[1;33m"
#define BLUE "\033[1;34m"
#define RESET "\033[0m"

/*
 * @brief The service state
 */
enum class ServiceStatus : int
{
    UNKNOWN = 100,
    STARTING = 101,
    READY = 102,
    DEGRADED = 103,
    MAINTENANCE = 104,
    STOPPING = 105,
    STOPPED = 106,
    ERROR = 107
};

/*
 * @brief Set the debug mode
 * @param debug The debug mode
 */
void setDebug(bool debug);

/*
 * @brief Get the debug mode
 * @return The debug mode
 */
bool getDebug();

/*
 * @brief The type of log
 */
enum class LogType
{
    DEBUGER,
    INFO,
    WARNING,
    ERROR
};

/*
 * @brief Prints a message with a log type
 * @param type The log type
 * @param message The message to print
 */
void print(const LogType &type, const std::string &message);

/*
 * @brief Prints a message
 * @param message The message to print
 */
void print(const std::string &message);

/*
 * @brief Prints a message with a service state label
 * @param state The service state
 */
void print(ServiceStatus state);

class LogTable
{
private:
    size_t _maxKeyLength = 0;
    size_t _maxValueLength = 0;
    std::vector<std::pair<std::string, std::string>> _rows;

public:
    int maxLineLength = 100;
    std::string header;

    LogTable() {}
    LogTable(int maxLineLength) : maxLineLength(maxLineLength) {} // default maxLineLength = 100
    LogTable(std::string header) : header(header) {}
    LogTable(int maxLineLength, std::string header) : maxLineLength(maxLineLength), header(header) {}

    ~LogTable() {}

    /*
     * @brief Set the header of the log table
     * @param header The header of the log table
     */
    void setHeader(const std::string &header);

    /*
     * @brief Add a row to the log table
     * @param key The key of the row
     * @param value The value of the row
     */
    void addRow(const std::string &key, const std::string &value);

    /*
     * @brief Print the log table
     * @param logType The log type
     */
    void printLogTable(const LogType &logType);
};

#endif // LOG_PRINTER_HPP
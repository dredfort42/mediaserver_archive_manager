#ifndef CHRONO_NAME_HPP
#define CHRONO_NAME_HPP

#include <string>
// #include <ctime>
#include <chrono>

class ChronoName
{
public:
    static int getDaysSinceEpoch(int64_t &timestamp);
    static int getSecondsSinceMidnight(int64_t &timestamp);
    static int getFileName(int64_t &timestamp, int &fragmentLengthInSeconds);
};

#endif // CHRONO_NAME_HPP
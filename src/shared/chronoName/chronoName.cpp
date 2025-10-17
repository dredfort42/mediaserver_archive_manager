#include "chronoName.hpp"

// int ChronoName::getDayEpoch(int64_t timestamp)
// {
//     // Convert the timestamp to a tm struct
//     std::time_t time = static_cast<std::time_t>(timestamp / 1000);
//     std::tm *timeInfo = std::gmtime(&time);

//     // Extract the day of the year
//     int day_of_year = timeInfo->tm_yday + 1; // tm_yday is 0-indexed

//     // // Output the result
//     // std::cout << "Day of the year: " << day_of_year << std::endl;

//     return timeInfo->tm_yday + 1;
// }

int ChronoName::getDaysSinceEpoch(int64_t &timestamp)
{
    std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();
    std::chrono::system_clock::duration durationSinceEpoch = currentTime.time_since_epoch();
    std::chrono::hours daysSinceEpoch = std::chrono::duration_cast<std::chrono::hours>(durationSinceEpoch / 24);

    return daysSinceEpoch.count();
}

int ChronoName::getSecondsSinceMidnight(int64_t &timestamp)
{
    std::chrono::system_clock::time_point currentTime = std::chrono::system_clock::now();
    std::chrono::system_clock::duration durationSinceEpoch = currentTime.time_since_epoch();
    std::chrono::seconds secondsSinceEpoch = std::chrono::duration_cast<std::chrono::seconds>(durationSinceEpoch);

    return secondsSinceEpoch.count() % 86400;
}

int ChronoName::getFileName(int64_t &timestamp, int &fragmentLengthInSeconds)
{
    int secondsSinceMidnight = ChronoName::getSecondsSinceMidnight(timestamp);

    return secondsSinceMidnight - (secondsSinceMidnight % fragmentLengthInSeconds);
}
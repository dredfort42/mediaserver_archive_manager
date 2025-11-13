#include "uuid.hpp"

std::string generateUUID()
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, 15);
    std::uniform_int_distribution<> dis2(8, 11);

    std::stringstream hexstream;
    for (int i = 0; i < 8; i++)
    {
        hexstream << std::hex << dis(gen);
    }

    hexstream << "-";
    for (int i = 0; i < 4; i++)
    {
        hexstream << std::hex << dis(gen);
    }

    hexstream << "-4";
    for (int i = 0; i < 3; i++)
    {
        hexstream << std::hex << dis(gen);
    }

    hexstream << "-";
    hexstream << std::hex << dis2(gen);
    for (int i = 0; i < 3; i++)
    {
        hexstream << std::hex << dis(gen);
    }

    hexstream << "-";
    for (int i = 0; i < 12; i++)
    {
        hexstream << std::hex << dis(gen);
    }

    return hexstream.str();
}
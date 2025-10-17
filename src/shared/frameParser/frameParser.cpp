#include "frameParser.hpp"

bool isIFrame(const char *payload, int size)
{
    int posPayload = 0;

    while (posPayload < size - 4)
    {
        if (payload[posPayload] == 0 &&
            checkNal(payload + posPayload) &&
            (payload[posPayload + 4] & 0x1F) == 5)
            return true;
        posPayload++;
    }

    return false;
}

bool checkNal(const char *payload)
{
    static unsigned char nal[] = {0, 0, 0, 1};
    int posNal = 0;

    while (posNal < 4)
    {
        if (payload[posNal] != nal[posNal])
            return false;
        posNal++;
    }

    return true;
}
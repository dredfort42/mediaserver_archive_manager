#ifndef FRAME_PARSER_HPP
#define FRAME_PARSER_HPP

/**
 * @brief The iFrame detection function
 * @param payload The payload of the frame
 * @param size The size of the payload
 */
bool isIFrame(const char *payload, int size);

/**
 * @brief The nal detection function
 * @param payload The payload of the frame
 */
bool checkNal(const char *payload);

#endif // FRAME_PARSER_HPP
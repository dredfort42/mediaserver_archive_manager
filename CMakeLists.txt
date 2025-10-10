cmake_minimum_required(VERSION 3.15)
project(MediaServerArchiveManager VERSION 1.0.0 LANGUAGES CXX)

# Set C++ standard globally
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Export compile commands for tooling
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Detect macOS
if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
    set(MACOSX TRUE)
    message(STATUS "OS type: macOS")
endif()

# Enable ccache if available
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
    message(STATUS "Using ccache: ${CCACHE_PROGRAM}")
endif()

# Add custom CMake modules
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

# # FetchContent for external dependencies
# include(FetchContent)
# FetchContent_Declare(
#     json
#     URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
#     DOWNLOAD_EXTRACT_TIMESTAMP TRUE
# )
# FetchContent_MakeAvailable(json)

# Copy configuration files
if(MACOSX AND EXISTS "${CMAKE_SOURCE_DIR}/config/service.cfg")
    configure_file(
        "${CMAKE_SOURCE_DIR}/config/service.cfg"
        "${CMAKE_CURRENT_BINARY_DIR}/config/service.cfg"
        COPYONLY
    )
endif()

if(EXISTS "${CMAKE_SOURCE_DIR}/local.cfg")
    configure_file(
        "${CMAKE_SOURCE_DIR}/local.cfg"
        "${CMAKE_CURRENT_BINARY_DIR}/local.cfg"
        COPYONLY
    )
endif()

# Find required packages
find_package(FFmpeg REQUIRED)
find_package(Protobuf REQUIRED)

# Platform-specific package finding
if(MACOSX)
    find_package(PkgConfig REQUIRED)
    
    # Try to find PostgreSQL using pkg-config, fall back to custom Find modules
    pkg_check_modules(POSTGRES libpq)
    if(NOT POSTGRES_FOUND)
        find_package(POSTGRES REQUIRED)
        find_package(PQXX REQUIRED)
    else()
        pkg_check_modules(PQXX REQUIRED libpqxx)
    endif()
    
    pkg_check_modules(RDKAFKA rdkafka++)
else()
    find_package(POSTGRES REQUIRED)
    find_package(PQXX REQUIRED)
endif()

# Generate protobuf sources
set(PROTO_FILE "${CMAKE_SOURCE_DIR}/protoData.proto")
protobuf_generate_cpp(PROTO_SRCS PROTO_HDRS ${PROTO_FILE})

# Create an INTERFACE library for common compile options
add_library(common_compile_options INTERFACE)
target_compile_options(common_compile_options INTERFACE
    -Wall -Wextra -Werror -Wpedantic
    -Wno-unused-parameter
    -Wno-unused-variable
    -Wno-unused-const-variable
    -Wno-unused-function
    -Wno-unused-private-field
    -Wno-unused-lambda-capture
    -Wno-unused-local-typedefs
    -Wno-gcc-compat
    -Wno-gnu-zero-variadic-macro-arguments
)

# Create an INTERFACE library for common include directories
add_library(common_includes INTERFACE)
target_include_directories(common_includes INTERFACE
    "${CMAKE_SOURCE_DIR}/src/shared"
    # "${CMAKE_SOURCE_DIR}/src/shared/archiveTask"
    # "${CMAKE_SOURCE_DIR}/src/shared/camera"
    # "${CMAKE_SOURCE_DIR}/src/shared/chronoName"
    "${CMAKE_SOURCE_DIR}/src/shared/configReader"
    # "${CMAKE_SOURCE_DIR}/src/shared/frameParser"
    "${CMAKE_SOURCE_DIR}/src/shared/logPrinter"
    "${CMAKE_SOURCE_DIR}/src/shared/messenger"
    # "${CMAKE_SOURCE_DIR}/src/shared/timeIntervalParser"
    "${CMAKE_CURRENT_BINARY_DIR}"
    ${FFMPEG_INCLUDE_DIRS}
)

# Add platform-specific includes
if(MACOSX)
    if(PROTOBUF_INCLUDE_DIRS)
        target_include_directories(common_includes INTERFACE ${PROTOBUF_INCLUDE_DIRS})
    endif()
    if(POSTGRES_INCLUDE_DIRS)
        target_include_directories(common_includes INTERFACE ${POSTGRES_INCLUDE_DIRS})
    endif()
    if(POSTGRES_INCLUDE_DIRECTORIES)
        target_include_directories(common_includes INTERFACE ${POSTGRES_INCLUDE_DIRECTORIES})
    endif()
    if(PQXX_INCLUDE_DIRS)
        target_include_directories(common_includes INTERFACE ${PQXX_INCLUDE_DIRS})
    endif()
    if(PQXX_INCLUDE_DIRECTORIES)
        target_include_directories(common_includes INTERFACE ${PQXX_INCLUDE_DIRECTORIES})
    endif()
    if(RDKAFKA_INCLUDE_DIRS)
        target_include_directories(common_includes INTERFACE ${RDKAFKA_INCLUDE_DIRS})
    endif()
else()
    target_include_directories(common_includes INTERFACE
        ${POSTGRES_INCLUDE_DIRECTORIES}
        ${PQXX_INCLUDE_DIRECTORIES}
    )
endif()

# ==============================================================================
# ARCHIVE MANAGER executable
# ==============================================================================
set(ARCHIVE_MANAGER_NAME archive_manager)

add_executable(${ARCHIVE_MANAGER_NAME}
    # src/shared/archiveTask/archiveTask.cpp
    # src/shared/camera/camera.cpp
    src/shared/configReader/configMap.cpp
    src/shared/logPrinter/logPrinter.cpp
    src/shared/messenger/messenger.cpp
    ${PROTO_SRCS}
    # src/services/manager/thConsumer.cpp
    # src/services/manager/thController.cpp
    # src/services/manager/thDatabase.cpp
    # src/services/manager/thReader.cpp
    # src/services/manager/thWriter.cpp
	src/services/manager/config.cpp
	src/services/manager/initMessenger.cpp
	src/services/manager/initDatabase.cpp
    src/services/manager/main.cpp
)

target_link_libraries(${ARCHIVE_MANAGER_NAME}
    PRIVATE
        common_compile_options
        common_includes
        # nlohmann_json::nlohmann_json
)

if(MACOSX)
    target_link_libraries(${ARCHIVE_MANAGER_NAME} PRIVATE
        ${RDKAFKA_LIBRARIES}
        ${PQXX_LIBRARIES}
        ${PROTOBUF_LIBRARIES}
        absl_log_internal_check_op
        absl_log_internal_message
    )
else()
    target_link_libraries(${ARCHIVE_MANAGER_NAME} PRIVATE
        ${POSTGRES_LIBRARIES}
        ${PQXX_LIBRARIES}
        rdkafka++
        rdkafka
        protobuf
    )
endif()

install(TARGETS ${ARCHIVE_MANAGER_NAME} RUNTIME DESTINATION bin)

# # ==============================================================================
# # ARCHIVE WRITER executable
# # ==============================================================================
# set(ARCHIVE_WRITER_NAME archive_writer)

# add_executable(${ARCHIVE_WRITER_NAME}
#     src/shared/logPrinter/logPrinter.cpp
#     src/shared/configReader/configMap.cpp
#     src/shared/chronoName/chronoName.cpp
#     src/shared/frameParser/frameParser.cpp
#     src/shared/messenger/messenger.cpp
#     ${PROTO_SRCS}
#     src/writer/thConsumer.cpp
#     src/writer/thFileWriter.cpp
#     src/writer/thSendOffset.cpp
#     src/writer/main.cpp
# )

# target_link_libraries(${ARCHIVE_WRITER_NAME}
#     PRIVATE
#         common_compile_options
#         common_includes
#         ${FFMPEG_LIBRARIES}
#         ${SWSCALE_LIBRARIES}
# )

# if(MACOSX)
#     target_link_libraries(${ARCHIVE_WRITER_NAME} PRIVATE
#         ${RDKAFKA_LIBRARIES}
#         ${PROTOBUF_LIBRARIES}
#         absl_log_internal_check_op
#         absl_log_internal_message
#     )
# else()
#     target_link_libraries(${ARCHIVE_WRITER_NAME} PRIVATE
#         rdkafka++
#         rdkafka
#         protobuf
#     )
# endif()

# install(TARGETS ${ARCHIVE_WRITER_NAME} RUNTIME DESTINATION bin)

# # ==============================================================================
# # ARCHIVE READER executable
# # ==============================================================================
# set(ARCHIVE_READER_NAME archive_reader)

# add_executable(${ARCHIVE_READER_NAME}
#     src/shared/logPrinter/logPrinter.cpp
#     src/shared/configReader/configMap.cpp
#     src/shared/messenger/messenger.cpp
#     ${PROTO_SRCS}
#     src/reader/fileReader.cpp
#     src/reader/contentList.cpp
#     src/reader/timeIntervalParser.cpp
#     src/reader/main.cpp
# )

# target_link_libraries(${ARCHIVE_READER_NAME}
#     PRIVATE
#         common_compile_options
#         common_includes
# )

# if(MACOSX)
#     target_link_libraries(${ARCHIVE_READER_NAME} PRIVATE
#         ${RDKAFKA_LIBRARIES}
#         ${PROTOBUF_LIBRARIES}
#         absl_log_internal_check_op
#         absl_log_internal_message
#     )
# else()
#     target_link_libraries(${ARCHIVE_READER_NAME} PRIVATE
#         rdkafka++
#         rdkafka
#         protobuf
#     )
# endif()

# install(TARGETS ${ARCHIVE_READER_NAME} RUNTIME DESTINATION bin)
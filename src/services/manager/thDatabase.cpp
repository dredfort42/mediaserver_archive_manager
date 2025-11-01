#include "manager.hpp"

Messenger::messages_map_t *getOffsetsTopic(volatile sig_atomic_t *wasInrerrupted,
                                           std::string offsetsTopic,
                                           Messenger::messenger_content_t *messengerContent)
{
    while (!*wasInrerrupted)
    {
        if (messengerContent->find(offsetsTopic) != messengerContent->end())
        {
            return &messengerContent->at(offsetsTopic);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    }

    return nullptr;
}

void storeOffsets(volatile sig_atomic_t *wasInrerrupted,
                  PGconn *dbConnect,
                  Messenger::messenger_content_t *messengerContent,
                  ConfigMap *config)
{
    if (DEBUG)
        print(LogType::INFO, ">>> Start store offsets thread");

    std::string offsetsTopic = config->getProperty("kafka.topic.compact.clean.hour.fileOffsets");

    // std::string host = config->getProperty("db.host");
    // std::string port = config->getProperty("db.port");
    // std::string user = config->getProperty("db.user");
    // std::string password = config->getProperty("db.password");
    // std::string dbname = config->getProperty("db.database.name");
    std::string table = config->getProperty("db.database.table.archive");

    // std::string connectionString = "host=" + host + " port=" + port + " user=" + user + " password=" + password + " dbname=" + dbname;

    // PGconn *conn = PQconnectdb(connectionString.c_str());

    // while (!*wasInrerrupted && PQstatus(conn) != CONNECTION_OK)
    // {
    //     print(LogType::ERROR, "Connection to database failed: " + std::string(PQerrorMessage(conn)));
    //     PQfinish(conn);
    //     std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    //     conn = PQconnectdb(connectionString.c_str());
    // }

    if (DEBUG)
        print(LogType::INFO, "Connection to database established");

    // while (!*wasInrerrupted && createType(wasInrerrupted, dbConnect))
    // {
    //     std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    // }

    // while (!*wasInrerrupted && createTable(wasInrerrupted, dbConnect, table))
    // {
    //     std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    // }

    Messenger::messages_map_t *offsetsTopicMessages = getOffsetsTopic(wasInrerrupted, offsetsTopic, messengerContent);

    while (!*wasInrerrupted)
    {
        if (offsetsTopicMessages->begin() != offsetsTopicMessages->end())
        {
            Messenger::message_t message = *offsetsTopicMessages->begin();
            offsetsTopicMessages->erase(offsetsTopicMessages->begin());

            std::string cameraUUID = message.first;
            proto::ProtoOffset protoOffset;

            protoOffset.ParseFromString(message.second);

            if (cameraUUID.empty() ||
                protoOffset.folder() <= 0 ||
                protoOffset.file() <= 0 ||
                protoOffset.timestamp() <= 0 ||
                protoOffset.offset() <= 0)
            {
                if (DEBUG)
                    print(LogType::WARNING, "Offset from " + cameraUUID + " is empty or invalid");
                continue;
            }

            std::string query = "INSERT INTO public." + table + " (stream_uuid, folder, file, iframe_indexes) VALUES ('" +
                                cameraUUID + "', " +
                                std::to_string(protoOffset.folder()) + ", " +
                                std::to_string(protoOffset.file()) + ", " +
                                "ARRAY[(" + std::to_string(protoOffset.timestamp()) + ", " + std::to_string(protoOffset.offset()) + ")::iframe_index]) " +
                                "ON CONFLICT (stream_uuid, folder, file) DO UPDATE SET " +
                                "iframe_indexes = " + table + ".iframe_indexes || ARRAY[(" + std::to_string(protoOffset.timestamp()) + ", " + std::to_string(protoOffset.offset()) + ")::iframe_index];";

            PGresult *res;
        sendOffset:
            res = PQexec(dbConnect, query.c_str());
            if (!*wasInrerrupted && PQresultStatus(res) != PGRES_COMMAND_OK)
            {
                print(LogType::ERROR, "Add data to table failed: " + std::string(PQerrorMessage(dbConnect)));
                goto sendOffset;
            }

            if (DEBUG)
                print(LogType::INFO, "Offset from " + cameraUUID + " for " + std::to_string(protoOffset.timestamp()) + " was sent to DB");

            protoOffset.Clear();
        }
        else
            std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // std::cout << "\r[ - ]" << std::flush;
        // std::this_thread::sleep_for(std::chrono::milliseconds(250));
        // std::cout << "\r[ \\ ]" << std::flush;
        // std::this_thread::sleep_for(std::chrono::milliseconds(250));
        // std::cout << "\r[ | ]" << std::flush;
        // std::this_thread::sleep_for(std::chrono::milliseconds(250));
        // std::cout << "\r[ / ]" << std::flush;
        // std::this_thread::sleep_for(std::chrono::milliseconds(250));
    }

    if (DEBUG)
        print(LogType::INFO, "<<< Stop store offsets thread");
}
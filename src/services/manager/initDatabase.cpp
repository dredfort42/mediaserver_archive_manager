#include "manager.hpp"

std::string initDatabase(PGconn *dbConnect, DatabaseConfig dbConfig)
{
    std::string connectionString = "host=" + dbConfig.host +
                                   " port=" + dbConfig.port +
                                   " user=" + dbConfig.user +
                                   " password=" + dbConfig.password +
                                   " dbname=" + dbConfig.dbName +
                                   " sslmode=" + dbConfig.sslMode;

    dbConnect = PQconnectdb(connectionString.c_str());
    if (PQstatus(dbConnect) != CONNECTION_OK)
    {
        PQfinish(dbConnect);
        return "Connection to database failed";
    }

    return "";
}

void closeDatabase(PGconn *dbConnect)
{
    if (dbConnect != nullptr)
    {
        PQfinish(dbConnect);
        print(LogType::DEBUGER, "Database connection closed");
    }
}
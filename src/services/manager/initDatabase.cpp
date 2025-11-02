#include "manager.hpp"

PGconn *initDatabase(DatabaseConfig dbConfig)
{
    std::string connectionString = "host=" + dbConfig.host +
                                   " port=" + dbConfig.port +
                                   " user=" + dbConfig.user +
                                   " password=" + dbConfig.password +
                                   " dbname=" + dbConfig.dbName +
                                   " sslmode=" + dbConfig.sslMode;

    PGconn *dbConnect = PQconnectdb(connectionString.c_str());
    if (PQstatus(dbConnect) != CONNECTION_OK)
    {
        std::string err = PQerrorMessage(dbConnect ? dbConnect : nullptr);
        if (dbConnect)
            PQfinish(dbConnect);
        print(LogType::ERROR, "Connection to database failed: " + err);
        return nullptr;
    }

    return dbConnect;
}

void closeDatabase(PGconn *dbConnect)
{
    if (dbConnect != nullptr)
    {
        PQfinish(dbConnect);
        print(LogType::DEBUGER, "Database connection closed");
    }
}

bool isTableExists(PGconn *dbConnect, const std::string &tableName)
{
    std::string query = "SELECT to_regclass('" + tableName + "');";
    PGresult *res = PQexec(dbConnect, query.c_str());

    if (PQresultStatus(res) != PGRES_TUPLES_OK)
    {
        print(LogType::ERROR, "Failed to check if table exists: " + std::string(PQerrorMessage(dbConnect)));
        PQclear(res);
        return false;
    }

    bool exists = false;
    if (PQntuples(res) > 0 && PQgetisnull(res, 0, 0) == 0)
    {
        exists = true;
    }

    PQclear(res);
    return exists;
}
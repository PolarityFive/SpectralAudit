#pragma once

#include <string>
#include <sqlite3.h>
#include "../Model/Track.h"

class SqliteDatabase {
public:
    explicit SqliteDatabase(const std::string& dbPath);
    ~SqliteDatabase();

    void begin();
    void commit();
    void insertTrack(const Track& track);
private:
    void open(const std::string& path);
    void createSchema();

    sqlite3* db = nullptr;
    sqlite3_stmt* insertTrackStmt;
    sqlite3_stmt* insertFeaturesStmt;
};

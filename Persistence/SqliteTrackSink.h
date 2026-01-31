#pragma once
#include "TrackSink.h"
#include "SqliteDatabase.h"
#include "../Queue//BlockingQueue.h"

class SqliteTrackSink : public TrackSink {
public:
    explicit SqliteTrackSink(const std::string& dbPath, std::size_t queueCapacity = 256);
    ~SqliteTrackSink() override;

    void consume(Track&& track) override;
    void close() override;

private:
    void dbLoop();

    BlockingQueue<Track> queue;
    std::thread dbThread;
    SqliteDatabase db;
};

#include "SqliteTrackSink.h"

SqliteTrackSink::SqliteTrackSink(const std::string& dbPath, std::size_t capacity)
    : queue(capacity), db(dbPath) {
    db.begin();
    dbThread = std::thread(&SqliteTrackSink::dbLoop, this);
}

void SqliteTrackSink::consume(Track&& track) {
    queue.push(std::move(track));
}

void SqliteTrackSink::close() {
    queue.close();
    if (dbThread.joinable())
        dbThread.join();
}

SqliteTrackSink::~SqliteTrackSink() {
    close();
}

void SqliteTrackSink::dbLoop() {
    constexpr std::size_t BATCH_SIZE = 500;

    Track track;
    std::size_t batchCount = 0;

    while (queue.pop(track)) {
        db.insertTrack(track);
        ++batchCount;

        if (batchCount >= BATCH_SIZE) {
            db.commit();
            db.begin();
            batchCount = 0;
        }
    }

    if (batchCount > 0) {
        db.commit();
    }
}


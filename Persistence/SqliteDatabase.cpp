#include "SqliteDatabase.h"

#include <stdexcept>
#include <string>

#include "../Utilities/BlackMetalSanitizer.h"


static void exec(sqlite3* db, const char* sql) {
    char* err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "Unknown SQLite error";
        sqlite3_free(err);
        throw std::runtime_error(msg);
    }
}

SqliteDatabase::SqliteDatabase(const std::string& dbPath) {
    open(dbPath);
    createSchema();
}

SqliteDatabase::~SqliteDatabase() {
    if (db) {
        sqlite3_close(db);
    }
}

void SqliteDatabase::open(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open SQLite database");
    }
}

void SqliteDatabase::insertTrack(const Track& track) {
    const auto& metadata = track.getMetadata();
    const auto& features = track.getTrackFeatures();

    sqlite3_stmt* stmt = nullptr;

    sqlite3_prepare_v2(db,
        "INSERT OR IGNORE INTO tracks "
        "(path, title, artist, album, year, duration_seconds, sample_rate, total_samples, frame_count) "
        "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?);",
        -1, &stmt, nullptr);

    auto storedPath = BlackMetalSanitizer::toUtf8(metadata.path);
    sqlite3_bind_text(stmt, 1, storedPath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, metadata.title.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, metadata.artist.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, metadata.album.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 5, metadata.year);
    sqlite3_bind_double(stmt, 6, metadata.durationSeconds);
    sqlite3_bind_int(stmt, 7, metadata.sampleRate);
    sqlite3_bind_int64(stmt, 8, metadata.totalSamples);
    sqlite3_bind_int64(stmt, 9, metadata.frameCount);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        throw std::runtime_error(sqlite3_errmsg(db));
    }
    sqlite3_finalize(stmt);

    sqlite3_stmt* idStmt = nullptr;
    sqlite3_prepare_v2(db,
        "SELECT id FROM tracks WHERE path = ?;",
        -1, &idStmt, nullptr);

    sqlite3_bind_text(idStmt, 1, storedPath.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(idStmt) != SQLITE_ROW) {
        sqlite3_finalize(idStmt);
        throw std::runtime_error("Failed to resolve track id");
    }

    const sqlite3_int64 trackId = sqlite3_column_int64(idStmt, 0);
    sqlite3_finalize(idStmt);

    sqlite3_prepare_v2(db,
        R"sql(
    INSERT OR REPLACE INTO track_features (
        track_id,

        pcm_rms_mean, pcm_rms_median, pcm_rms_stddev,
        pcm_rms_p05, pcm_rms_p50, pcm_rms_p95,
        pcm_rms_min, pcm_rms_max,

        peak_mean, peak_median, peak_stddev,
        peak_p05, peak_p50, peak_p95,
        peak_min, peak_max,

        spectral_rms_mean, spectral_rms_median, spectral_rms_stddev,
        spectral_rms_p05, spectral_rms_p50, spectral_rms_p95,
        spectral_rms_min, spectral_rms_max,

        spectral_centroid_mean, spectral_centroid_median,
        spectral_centroid_stddev, spectral_centroid_p05,
        spectral_centroid_p50, spectral_centroid_p95,
        spectral_centroid_min, spectral_centroid_max,

        spectral_rolloff85_mean, spectral_rolloff85_median,
        spectral_rolloff85_stddev, spectral_rolloff85_p05,
        spectral_rolloff85_p50, spectral_rolloff85_p95,
        spectral_rolloff85_min, spectral_rolloff85_max,

        spectral_flatness_mean, spectral_flatness_median,
        spectral_flatness_stddev, spectral_flatness_p05,
        spectral_flatness_p50, spectral_flatness_p95,
        spectral_flatness_min, spectral_flatness_max,

        hf_ratio_mean, hf_ratio_median, hf_ratio_stddev,
        hf_ratio_p05, hf_ratio_p50, hf_ratio_p95,
        hf_ratio_min, hf_ratio_max
    )
    VALUES (
		?,
        ?,?,?,?,?,?,?,?,
        ?,?,?,?,?,?,?,?,
        ?,?,?,?,?,?,?,?,
        ?,?,?,?,?,?,?,?,
        ?,?,?,?,?,?,?,?,
        ?,?,?,?,?,?,?,?,
        ?,?,?,?,?,?,?,?
    );
    )sql",
        -1, &stmt, nullptr
    );


    int i = 1;
    sqlite3_bind_int64(stmt, i++, trackId);

#define BIND_STATS(s) \
    sqlite3_bind_double(stmt, i++, (s).mean); \
    sqlite3_bind_double(stmt, i++, (s).median); \
    sqlite3_bind_double(stmt, i++, (s).stddev); \
    sqlite3_bind_double(stmt, i++, (s).p05); \
    sqlite3_bind_double(stmt, i++, (s).p50); \
    sqlite3_bind_double(stmt, i++, (s).p95); \
    sqlite3_bind_double(stmt, i++, (s).min); \
    sqlite3_bind_double(stmt, i++, (s).max);

    BIND_STATS(features.pcmRms)
        BIND_STATS(features.peak)
        BIND_STATS(features.spectralRms)
        BIND_STATS(features.spectralCentroid)
        BIND_STATS(features.spectralRolloff85)
        BIND_STATS(features.spectralFlatness)
        BIND_STATS(features.hfRatio)

#undef BIND_STATS

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            throw std::runtime_error(sqlite3_errmsg(db));
        }
    sqlite3_finalize(stmt);
}

void SqliteDatabase::createSchema() {
    exec(db, R"sql(
        CREATE TABLE IF NOT EXISTS tracks (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            path TEXT NOT NULL UNIQUE,
            title TEXT,
            artist TEXT,
            album TEXT,
            year INTEGER,
            duration_seconds REAL NOT NULL,
            sample_rate INTEGER NOT NULL,
            total_samples INTEGER NOT NULL,
            frame_count INTEGER NOT NULL
        );
    )sql");

    exec(db, R"sql(
        CREATE TABLE IF NOT EXISTS track_features (
            track_id INTEGER PRIMARY KEY,

            pcm_rms_mean REAL, pcm_rms_median REAL, pcm_rms_stddev REAL,
            pcm_rms_p05 REAL, pcm_rms_p50 REAL, pcm_rms_p95 REAL,
            pcm_rms_min REAL, pcm_rms_max REAL,

            peak_mean REAL, peak_median REAL, peak_stddev REAL,
            peak_p05 REAL, peak_p50 REAL, peak_p95 REAL,
            peak_min REAL, peak_max REAL,

            spectral_rms_mean REAL, spectral_rms_median REAL, spectral_rms_stddev REAL,
            spectral_rms_p05 REAL, spectral_rms_p50 REAL, spectral_rms_p95 REAL,
            spectral_rms_min REAL, spectral_rms_max REAL,

            spectral_centroid_mean REAL, spectral_centroid_median REAL,
            spectral_centroid_stddev REAL, spectral_centroid_p05 REAL,
            spectral_centroid_p50 REAL, spectral_centroid_p95 REAL,
            spectral_centroid_min REAL, spectral_centroid_max REAL,

            spectral_rolloff85_mean REAL, spectral_rolloff85_median REAL,
            spectral_rolloff85_stddev REAL, spectral_rolloff85_p05 REAL,
            spectral_rolloff85_p50 REAL, spectral_rolloff85_p95 REAL,
            spectral_rolloff85_min REAL, spectral_rolloff85_max REAL,

            spectral_flatness_mean REAL, spectral_flatness_median REAL,
            spectral_flatness_stddev REAL, spectral_flatness_p05 REAL,
            spectral_flatness_p50 REAL, spectral_flatness_p95 REAL,
            spectral_flatness_min REAL, spectral_flatness_max REAL,

            hf_ratio_mean REAL, hf_ratio_median REAL, hf_ratio_stddev REAL,
            hf_ratio_p05 REAL, hf_ratio_p50 REAL, hf_ratio_p95 REAL,
            hf_ratio_min REAL, hf_ratio_max REAL,

            FOREIGN KEY(track_id) REFERENCES tracks(id)
        );
    )sql");
}

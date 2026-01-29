#define NOMINMAX

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

#include <iostream>

#include "Utilities/BlackMetalSanitizer.h"
#include "Resources/Constants.h"
#include "Persistence/SqliteDatabase.h"
#include "TrackBatchProcessor.h"

static void printDuration(std::chrono::milliseconds ms);

int main() {
    BlackMetalSanitizer::setupConsole();
    using clock = std::chrono::steady_clock;

    const auto t0 = clock::now();
    TrackBatchProcessor batchProcessor(CONSTANTS::INPUT_DIRECTORY);
    std::vector<Track> tracks = batchProcessor.runParallel(13,32);

    const auto t1 = clock::now();

    SqliteDatabase db(CONSTANTS::DB_PATH_V4);
    db.begin();
    for (const Track& track : tracks) {
        db.insertTrack(track);
    }
    db.commit();

    const auto t2 = clock::now();

    std::wcout << L"Analysis time: ";
    printDuration(std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0));
    std::wcout << L'\n';

    std::wcout << L"DB write time: ";
    printDuration(std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1));
    std::wcout << L'\n';

    std::wcout << L"Total time:   ";
    printDuration(std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t0));
    std::wcout << L'\n';

    std::wcout << L"Tracks analyzed: " << tracks.size() << L'\n';
    return 0;
}


static void printDuration(std::chrono::milliseconds ms) {
    using namespace std::chrono;

    const auto h = duration_cast<hours>(ms);
    ms -= h;
    const auto m = duration_cast<minutes>(ms);
    ms -= m;
    const auto s = duration_cast<seconds>(ms);
    ms -= s;

    std::wcout
        << std::setfill(L'0')
        << std::setw(2) << h.count() << L":"
        << std::setw(2) << m.count() << L":"
        << std::setw(2) << s.count() << L"."
        << std::setw(3) << ms.count();
}
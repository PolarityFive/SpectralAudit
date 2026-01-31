#define NOMINMAX

#include <iostream>

#include "Utilities/BlackMetalSanitizer.h"
#include "Resources/Constants.h"
#include "TrackBatchProcessor.h"
#include "Persistence/SqliteTrackSink.h"
#include "Persistence/TrackSink.h"

static void printDuration(std::chrono::milliseconds ms);

int main() {
    BlackMetalSanitizer::setupConsole();

    using clock = std::chrono::steady_clock;
    const auto t0 = clock::now();

    SqliteTrackSink dbSink(CONSTANTS::DB_PATH_V5);
    TrackBatchProcessor batchProcessor(CONSTANTS::INPUT_DIRECTORY, dbSink);

    batchProcessor.runParallel(13, 32);

    const auto t1 = clock::now();

    std::wcout << L"Total time: ";
    printDuration(std::chrono::duration_cast<std::chrono::milliseconds>(t1 - t0));
    std::wcout << L'\n';

    dbSink.close();
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
#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "Model/Track.h"
#include "Queue/BlockingQueue.h"
#include "Utilities/Logger.h"
#include "Persistence/TrackSink.h"

class TrackBatchProcessor {
public:
    explicit TrackBatchProcessor(std::filesystem::path inputDirectory, TrackSink& sink);
    void runParallel(std::size_t workerCount, std::size_t queueCapacity);

private:
    void workerLoop(BlockingQueue<std::filesystem::path>& workQueue, std::atomic<std::size_t>& failedCount, Logger& logger);
    void producerLoop(BlockingQueue<std::filesystem::path>& workQueue, std::atomic<std::size_t>& enqueuedCount, Logger& logger);

    TrackFeatures extractTrackFeatures(const std::vector<double>& samples,int sampleRate,std::size_t& outFrameCount);
    Track buildTrack(const std::filesystem::path& path, const TrackFeatures& features, int sampleRate, std::size_t totalSamples, std::size_t frameCount);
    std::optional<Track>processTrack(const std::filesystem::path& path);

private:
    std::filesystem::path inputDirectory;
	TrackSink& sink;
};

#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "Model/Track.h"

class TrackBatchProcessor {
public:
    explicit TrackBatchProcessor(std::filesystem::path inputDirectory);
    std::vector<Track> run();
    std::vector<Track> runParallel(std::size_t workerCount, std::size_t queueCapacity);

private:
    TrackFeatures extractTrackFeatures(const std::vector<double>& samples,int sampleRate,std::size_t& outFrameCount);
    Track buildTrack(const std::filesystem::path& path, const TrackFeatures& features, int sampleRate, std::size_t totalSamples, std::size_t frameCount);
    std::optional<Track>processTrack(const std::filesystem::path& path);

private:
    std::filesystem::path inputDirectory;
};

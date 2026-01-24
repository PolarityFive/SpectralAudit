#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "Model/Track.h"

class TrackBatchProcessor {
public:
    TrackBatchProcessor(std::filesystem::path inputDirectory);
    std::vector<Track> run();

private:
    std::filesystem::path inputDirectory;
    std::optional<Track> processTrack(const std::filesystem::path& path);
};

#pragma once

#include <filesystem>
#include <optional>
#include <string>

struct AudioTags {
    std::string artist;
    std::string title;
    std::string album;
    int year = 0;
};

class AudioMetadataReader {
public:
    static std::optional<AudioTags>extract(const std::filesystem::path& path);
};

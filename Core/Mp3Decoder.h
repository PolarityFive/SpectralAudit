#pragma once
#include <filesystem>
#include <string>
#include <vector>
#include <optional>

struct DecodedAudio {
    std::vector<double> samples;
    int sampleRate = 0;
};

class Mp3Decoder {
public:
	static bool decodeMp3Mono(const std::string& path, std::vector<double>& samples, int& sampleRate);
    static std::optional<DecodedAudio>decode(const std::filesystem::path& path,std::size_t minSamples);
};

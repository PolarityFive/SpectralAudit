#include "Mp3Decoder.h"

#define MINIMP3_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 4267 4244 6385 6386 6262)
#include <iostream>

#include "../Third Party/minimp3.h"
#include "../Third Party/minimp3_ex.h"
#include "../Utilities/BlackMetalSanitizer.h"
#pragma warning(pop)

bool Mp3Decoder::decodeMp3Mono(const std::string& path, std::vector<double>& samples, int& sampleRate) {
    mp3dec_t decoder{};
    mp3dec_file_info_t info{};

    mp3dec_init(&decoder);

    if (mp3dec_load(&decoder, path.c_str(), &info, nullptr, nullptr) != 0)
        return false;

    sampleRate = info.hz;
    const int channels = info.channels;

    if (channels <= 0 || sampleRate <= 0 || info.samples <= 0 || info.buffer == nullptr) {
        if (info.buffer) std::free(info.buffer);
        return false;
    }

    const size_t total = static_cast<size_t>(info.samples);
    const size_t ch = static_cast<size_t>(channels);
    const size_t frameCount = total / ch;

    samples.resize(frameCount);

    for (size_t frame = 0; frame < frameCount; ++frame) {
        const size_t base = frame * ch;
        double sum = 0.0;
        for (size_t c = 0; c < ch; ++c)
            sum += info.buffer[base + c];
        samples[frame] = (sum / static_cast<double>(channels)) / 32768.0;
    }

    std::free(info.buffer);
    return true;
}


std::optional<DecodedAudio> Mp3Decoder::decode(const std::filesystem::path& path, std::size_t minSamples) {
    DecodedAudio out;

    auto safePath = BlackMetalSanitizer::makeSafeTempCopy(path);
    const bool ok = decodeMp3Mono(safePath.string(), out.samples, out.sampleRate);
    BlackMetalSanitizer::cleanup(safePath);

    if (!ok) {
        std::cerr << "Decode failed: " << path << '\n';
        return std::nullopt;
    }

    if (out.samples.size() < minSamples) {
        std::cerr << "Too short: " << path << '\n';
        return std::nullopt;
    }

    return out;
}

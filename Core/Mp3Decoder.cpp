#include "Mp3Decoder.h"

#define MINIMP3_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 4267 4244 6385 6386 6262)
#include "../Third Party/minimp3.h"
#include "../Third Party/minimp3_ex.h"
#pragma warning(pop)

bool Mp3Decoder::decodeMp3Mono(const std::string& path, std::vector<double>& samples, int& sampleRate) {
    mp3dec_t decoder{};
    mp3dec_file_info_t info{};

    mp3dec_init(&decoder);

    if (mp3dec_load(&decoder, path.c_str(), &info, nullptr, nullptr) != 0) {
        return false;
    }

    sampleRate = info.hz;
    const int channels = info.channels;

    samples.resize(info.samples / channels);

    for (size_t i = 0, j = 0; i < info.samples; i += channels, ++j) {
        double sum = 0.0;
        for (int c = 0; c < channels; ++c) {
            sum += info.buffer[i + c];
        }
        samples[j] = (sum / channels) / 32768.0;
    }

    std::free(info.buffer);
    return true;
}

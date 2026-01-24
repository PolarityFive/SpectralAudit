#include "SpectrogramPngWriter.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#pragma warning(push)
#pragma warning(disable : 6011 26819 4996 6386 6262)
#include "../Third Party/stb_image_write.h"
#pragma warning(pop)

static constexpr double EPS = 1e-12;
bool isDebugEnabled = false;

bool SpectrogramPngWriter::write(const std::string& path, const std::vector<std::vector<double>>& magnitudes, const Options& options) {
    if (!isDebugEnabled) {
        return false;
    }

    if (magnitudes.empty()) {
        return false;
    }

    const int frames = static_cast<int>(magnitudes.size());
    const int bins = static_cast<int>(magnitudes[0].size());

    const int width = frames;
    const int height = bins;

    std::vector<uint8_t> image(width * height * 3);

    for (int y = 0; y < height; ++y) {
        double yf = static_cast<double>(y) / height;
        int srcBin = y;

        if (options.logFrequency) {
            srcBin = static_cast<int>(std::pow(yf, 2.0) * bins);
            srcBin = std::min(srcBin, bins - 1);
        }

        for (int x = 0; x < width; ++x) {
            const double mag = magnitudes[x][srcBin];
            const double db = 20.0 * std::log10(mag + EPS);

            double v = (db - options.dbFloor) /
                (options.dbCeil - options.dbFloor);
            v = std::clamp(v, 0.0, 1.0);

            const int outY = height - 1 - y;
            const int idx = (outY * width + x) * 3;

            image[idx + 0] = static_cast<uint8_t>(255 * v);
            image[idx + 1] = static_cast<uint8_t>(255 * std::sqrt(v));
            image[idx + 2] = static_cast<uint8_t>(255 * (1.0 - v));
        }
    }

    return stbi_write_png(path.c_str(), width, height, 3, image.data(), width * 3) != 0;
}

#pragma once

#include <string>
#include <vector>

class SpectrogramPngWriter {
public:
    struct Options {
        double dbFloor = -80.0;
        double dbCeil = 0.0;
        bool logFrequency = true;
    };

    static bool write(const std::string& path, const std::vector<std::vector<double>>& magnitudes, const Options& options = {});
};

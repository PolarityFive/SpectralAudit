#pragma once

#include <vector>
#include "../Model/TrackData.h"

class FeatureExtractor {
public:
    FeatureExtractor(int sampleRate);
    FrameFeatures extract(const std::vector<double>& magnitudes) const;

private:
    int sampleRate;
};

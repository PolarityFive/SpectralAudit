#pragma once

#include <vector>
#include "../Model/TrackData.h"

class TrackAggregator {
public:
    static TrackFeatures aggregate(const std::vector<FrameFeatures>& frames);
};
#pragma once

#include <vector>
#include "TrackData.h"

class Track {
public:
    explicit Track(TrackMetadata metadata);

    void setFrameFeatures(std::vector<FrameFeatures> frames);
    void setTrackFeatures(TrackFeatures features);

    const TrackMetadata& getMetadata() const;
    const std::vector<FrameFeatures>& getFrameFeatures() const;
    const TrackFeatures& getTrackFeatures() const;

private:
    TrackMetadata metadata;
    std::vector<FrameFeatures> frameFeatures;
    TrackFeatures trackFeatures;
};

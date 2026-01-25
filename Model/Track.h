#pragma once

#include <vector>
#include "TrackData.h"

class Track {
public:
    explicit Track(TrackMetadata metadata);

    void setTrackFeatures(TrackFeatures features);

    const TrackMetadata& getMetadata() const;
    const TrackFeatures& getTrackFeatures() const;

private:
    TrackMetadata metadata;
    TrackFeatures trackFeatures;
};

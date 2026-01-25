#include "Track.h"

Track::Track(TrackMetadata metadata) : metadata(std::move(metadata)) {
}

void Track::setTrackFeatures(TrackFeatures features) {
    this->trackFeatures = features;
}

const TrackMetadata& Track::getMetadata() const {
    return this->metadata;
}

const TrackFeatures& Track::getTrackFeatures() const {
    return this->trackFeatures;
}

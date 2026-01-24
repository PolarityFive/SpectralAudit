#include "Track.h"

Track::Track(TrackMetadata metadata) : metadata(std::move(metadata)) {
}

void Track::setFrameFeatures(std::vector<FrameFeatures> frames) {
    this->frameFeatures = std::move(frames);
}

void Track::setTrackFeatures(TrackFeatures features) {
    this->trackFeatures = features;
}

const TrackMetadata& Track::getMetadata() const {
    return this->metadata;
}

const std::vector<FrameFeatures>& Track::getFrameFeatures() const {
    return this->frameFeatures;
}

const TrackFeatures& Track::getTrackFeatures() const {
    return this->trackFeatures;
}

#pragma once

#include "../Model/Track.h"

class TrackSink {
public:
    virtual ~TrackSink() = default;
    virtual void consume(Track&& track) = 0;
    virtual void close() = 0;
};

#pragma once
#include <filesystem>
#include <string>

struct FrameFeatures {
    double pcmRms; // time-domain RMS
    double peak;

    double spectralRms;
    double spectralCentroid; // Spectral centroid is magnitude-weighted
    double spectralRolloff85;
    double spectralFlatness;
    double hfRatio;
};

struct FeatureStats {
    double mean;
    double median;
    double stddev;
    double p05;
    double p50;
    double p95;
    double min;
    double max;
};

struct TrackFeatures {
    FeatureStats pcmRms;
    FeatureStats peak;
    FeatureStats spectralRms;
    FeatureStats spectralCentroid;
    FeatureStats spectralRolloff85;
    FeatureStats spectralFlatness;
    FeatureStats hfRatio;
};

struct TrackMetadata {
    std::filesystem::path path;
    std::string title;
    std::string artist;
    std::string album;
    int year = 0;

    double durationSeconds;
    int sampleRate;

    size_t totalSamples;
    size_t frameCount;
};
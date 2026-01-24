#include "TrackAggregator.h"

#include <algorithm>
#include <cmath>
#include <numeric>

static FeatureStats computeStats(std::vector<double>& values) {
    FeatureStats featureStats{};

    const size_t size = values.size();
    if (size == 0) {
        return featureStats;
    }

    std::sort(values.begin(), values.end());

    featureStats.min = values.front();
    featureStats.max = values.back();

    const double sum = std::accumulate(values.begin(), values.end(), 0.0);
    featureStats.mean = sum / size;

    featureStats.median = (size % 2 == 0)
        ? (values[size / 2 - 1] + values[size / 2]) * 0.5
        : values[size / 2];

    auto percentile = [&](double p) {
        const double idx = p * (size - 1);
        const size_t i = static_cast<size_t>(idx);
        const double frac = idx - i;
        return (i + 1 < size)
            ? values[i] * (1.0 - frac) + values[i + 1] * frac
            : values[i];
        };

    featureStats.p05 = percentile(0.05);
    featureStats.p50 = percentile(0.50);
    featureStats.p95 = percentile(0.95);

    double variance = 0.0;
    for (double v : values) {
        const double d = v - featureStats.mean;
        variance += d * d;
    }
    featureStats.stddev = std::sqrt(variance / size);

    return featureStats;
}

TrackFeatures TrackAggregator::aggregate(const std::vector<FrameFeatures>& frames) {
    TrackFeatures out{};

    std::vector<double>pcmRms, peak, spectralRms, centroid, rolloff, flatness, hfRatio;

    const size_t size = frames.size();

    pcmRms.reserve(size);
    peak.reserve(size);
    spectralRms.reserve(size);
    centroid.reserve(size);
    rolloff.reserve(size);
    flatness.reserve(size);
    hfRatio.reserve(size);

    for (const auto& f : frames) {
        pcmRms.push_back(f.pcmRms);
        peak.push_back(f.peak);
        spectralRms.push_back(f.spectralRms);
        centroid.push_back(f.spectralCentroid);
        rolloff.push_back(f.spectralRolloff85);
        flatness.push_back(f.spectralFlatness);
        hfRatio.push_back(f.hfRatio);
    }

    out.pcmRms = computeStats(pcmRms);
    out.peak = computeStats(peak);
    out.spectralRms = computeStats(spectralRms);
    out.spectralCentroid = computeStats(centroid);
    out.spectralRolloff85 = computeStats(rolloff);
    out.spectralFlatness = computeStats(flatness);
    out.hfRatio = computeStats(hfRatio);

    return out;
}


#include "FeatureExtractor.h"
#include <cmath>
#include <algorithm>

static constexpr double EPS = 1e-12;

FeatureExtractor::FeatureExtractor(int sampleRate)
    : sampleRate(sampleRate) {
}

FrameFeatures FeatureExtractor::extract(const std::vector<double>& magnitudes) const {
    if (magnitudes.empty()) {
        return FrameFeatures{};
    }

    FrameFeatures features{};

    const int bins = static_cast<int>(magnitudes.size());
    const double nyquist = sampleRate * 0.5;
    const double binHz = nyquist / static_cast<double>(bins);

    double energySum = 0.0;
    double weightedFreqSum = 0.0;
    double peak = 0.0;
    double magSum = 0.0;

    for (int i = 0; i < bins; ++i) {
        const double mag = magnitudes[i];
        const double freq = i * binHz;

        const double mag2 = mag * mag;

        energySum += mag2;
        weightedFreqSum += freq * mag;
        magSum += mag;
        peak = std::max(peak, mag);
    }

    features.spectralRms = std::sqrt(energySum / bins);
    features.peak = peak;
    features.spectralCentroid = weightedFreqSum / (magSum + EPS);

    double cumulativeEnergy = 0.0;
    const double targetEnergy = energySum * 0.85;

    features.spectralRolloff85 = (bins - 1) * binHz;
    for (int i = 0; i < bins; ++i) {
        cumulativeEnergy += magnitudes[i] * magnitudes[i];
        if (cumulativeEnergy >= targetEnergy) {
            features.spectralRolloff85 = i * binHz;
            break;
        }
    }

    double logSum = 0.0;
    double linearSum = 0.0;

    for (double mag : magnitudes) {
        logSum += std::log(mag + EPS);
        linearSum += mag;
    }

    const double geoMean = std::exp(logSum / bins);
    const double arithMean = linearSum / bins;

    features.spectralFlatness = geoMean / (arithMean + EPS);

    double lowEnergy = 0.0;
    double highEnergy = 0.0;

    static constexpr double HF_SPLIT_HZ = 2000.0;

    for (int i = 0; i < bins; ++i) {
        const double freq = i * binHz;
        const double e = magnitudes[i] * magnitudes[i];

        if (freq < HF_SPLIT_HZ) {
            lowEnergy += e;
        }
        else {
            highEnergy += e;
        }
    }

    features.hfRatio = highEnergy / (lowEnergy + EPS);

    return features;
}


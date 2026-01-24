#include "FeatureExtractor.h"
#include <cmath>
#include <algorithm>

static constexpr double EPS = 1e-12;
static constexpr double HF_SPLIT_HZ = 2000.0;

FeatureExtractor::FeatureExtractor(int sampleRate)
    : sampleRate(sampleRate) {
}

FrameFeatures FeatureExtractor::extract(const std::vector<double>& magnitudes) const {
    FrameFeatures features{};
    const int bins = static_cast<int>(magnitudes.size());

    if (bins == 0) 
        return features;

    const double nyquist = sampleRate * 0.5;
    const double binHz = nyquist / bins;
    const int hfSplitBin = std::min(bins,static_cast<int>(HF_SPLIT_HZ / binHz));

    double energySum = 0.0;
    double weightedFreqSum = 0.0;
    double magSum = 0.0;
    double logSum = 0.0;
    double peak = 0.0;
    double lowEnergy = 0.0;
    double highEnergy = 0.0;

    for (int i = 0; i < bins; ++i) {
        const double mag = magnitudes[i];
        const double mag2 = mag * mag;

        energySum += mag2;
        magSum += mag;
        logSum += std::log(mag + EPS);
        peak = std::max(peak, mag);

        weightedFreqSum += (i * binHz) * mag;

        if (i < hfSplitBin)
            lowEnergy += mag2;
        else
            highEnergy += mag2;
    }

    features.spectralRms = std::sqrt(energySum / bins);
    features.peak = peak;
    features.spectralCentroid = weightedFreqSum / (magSum + EPS);

    const double geoMean = std::exp(logSum / bins);
    const double arithMean = magSum / bins;
    features.spectralFlatness = geoMean / (arithMean + EPS);

    features.hfRatio = highEnergy / (lowEnergy + EPS);

    const double targetEnergy = energySum * 0.85;
    double cumulativeEnergy = 0.0;

    features.spectralRolloff85 = (bins - 1) * binHz;

    for (int i = 0; i < bins; ++i) {
        cumulativeEnergy += magnitudes[i] * magnitudes[i];
        if (cumulativeEnergy >= targetEnergy) {
            features.spectralRolloff85 = i * binHz;
            break;
        }
    }

    return features;
}



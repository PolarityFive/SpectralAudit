#include "StftProcessor.h"
#include <cmath>
#include <mutex>
#include <stdexcept>

static constexpr double PI = 3.14159265358979323846;

namespace {
    std::mutex fftwPlannerMutex;
}

StftProcessor::StftProcessor(int windowSize, int hopSize)
    : windowSize(windowSize),
    hopSize(hopSize),
    frequencyBins(windowSize / 2 + 1),
    fftInput(windowSize)
{
    if (windowSize < 2 || hopSize <= 0) {
        throw std::invalid_argument("Invalid windowSize or hopSize");
    }

    buildHannWindow();

    fftOutput = static_cast<fftw_complex*>(fftw_malloc(sizeof(fftw_complex) * frequencyBins));

    if (!fftOutput) {
        throw std::runtime_error("FFTW allocation failed");
    }

    {
        std::lock_guard<std::mutex> lk(fftwPlannerMutex);
        fftPlan = fftw_plan_dft_r2c_1d(windowSize, fftInput.data(), fftOutput, FFTW_ESTIMATE);
    }

    if (!fftPlan) {
        fftw_free(fftOutput);
        throw std::runtime_error("FFTW plan creation failed");
    }
}

StftProcessor::~StftProcessor() {
    if (fftPlan) {
        std::lock_guard<std::mutex> lk(fftwPlannerMutex);
        fftw_destroy_plan(fftPlan);
        fftPlan = nullptr;
    }
    if (fftOutput) {
        fftw_free(fftOutput);
        fftOutput = nullptr;
    }
}


void StftProcessor::buildHannWindow() {
    hannWindow.resize(windowSize);
    for (int n = 0; n < windowSize; ++n) {
        hannWindow[n] = 0.5 * (1.0 - std::cos(2.0 * PI * n / (windowSize - 1)));
    }
}

std::vector<std::vector<double>> StftProcessor::computeMagnitudes(const std::vector<double>& samples) {
    const size_t total = samples.size();
    const size_t ws = static_cast<size_t>(windowSize);
    const size_t hs = static_cast<size_t>(hopSize);

    if (total < ws) 
        return {};

    const size_t frameCount = 1 + (total - ws) / hs;

    std::vector<std::vector<double>> magnitudes(frameCount, std::vector<double>(static_cast<size_t>(frequencyBins)));

    for (size_t frame = 0; frame < frameCount; ++frame) {
        const size_t offset = frame * hs;

        for (size_t n = 0; n < ws; ++n)
            fftInput[n] = samples[offset + n] * hannWindow[n];

        fftw_execute(fftPlan);

        for (int bin = 0; bin < frequencyBins; ++bin) {
            const double re = fftOutput[bin][0];
            const double im = fftOutput[bin][1];
            magnitudes[frame][static_cast<size_t>(bin)] = std::sqrt(re * re + im * im);
        }
    }

    return magnitudes;
}


int StftProcessor::getFrequencyBins() const {
    return frequencyBins;
}

int StftProcessor::getWindowSize() const {
    return windowSize;
}

int StftProcessor::getHopSize() const {
    return hopSize;
}

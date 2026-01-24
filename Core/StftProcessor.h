#pragma once

#include <vector>
#include <fftw3.h>

class StftProcessor {
public:
    StftProcessor(int windowSize, int hopSize);
    ~StftProcessor();

    StftProcessor(const StftProcessor&) = delete;
    StftProcessor& operator=(const StftProcessor&) = delete;

    std::vector<std::vector<double>> computeMagnitudes(const std::vector<double>& samples);

    int getFrequencyBins() const;
    int getWindowSize() const;
    int getHopSize() const;

private:
    void buildHannWindow();

    int windowSize;
    int hopSize;
    int frequencyBins;

    std::vector<double> hannWindow;
    std::vector<double> fftInput;
    fftw_complex* fftOutput;
    fftw_plan fftPlan;
};

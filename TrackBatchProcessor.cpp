#include "TrackBatchProcessor.h"
#include <iostream>
#include "Utilities/AudioMetadataExtractor.h"
#include "Utilities//BlackMetalSanitizer.h"
#include "Resources/Constants.h"
#include "Core/FeatureExtractor.h"
#include "Core/Mp3Decoder.h"
#include "Core/StftProcessor.h"
#include "Core/TrackAggregator.h"


TrackBatchProcessor::TrackBatchProcessor(std::filesystem::path inputDirectory)
    : inputDirectory(std::move(inputDirectory)){
}

std::vector<Track> TrackBatchProcessor::run() {
    namespace fs = std::filesystem;

    std::vector<Track> results;

    int failed = 0;

    for (const auto& entry : fs::recursive_directory_iterator(inputDirectory)) {

        if (!entry.is_regular_file())
            continue;

        const auto& path = entry.path();
        if (path.extension() != ".mp3" && path.extension() != ".MP3")
            continue;

        std::wcout << L"Processing: " << path.wstring() << L'\n';

        std::optional<Track> result = processTrack(path);
        if (!result.has_value()) {
            failed++;
            continue;
        }

        results.push_back(std::move(*result));
    }

    std::wcout << L"Processed: " << results.size() << L", Failed: " << failed << L'\n';
    return results;
}

std::optional<Track> TrackBatchProcessor::processTrack(const std::filesystem::path& path) {
    std::vector<double> samples;
    int sampleRate = 0;
    
    auto safePath = BlackMetalSanitizer::makeSafeTempCopy(path);
    if (!Mp3Decoder::decodeMp3Mono(safePath.string(), samples, sampleRate)) {
        BlackMetalSanitizer::cleanup(safePath);
        std::cerr << "Decode failed: " << path << '\n';
        return std::nullopt;
    }
    BlackMetalSanitizer::cleanup(safePath);

    if (samples.size() < CONSTANTS::WINDOW_SIZE) {
        std::cerr << "Too short: " << path << '\n';
        return std::nullopt;
    }

    const int windowSize = CONSTANTS::WINDOW_SIZE;
    const int hopSize = CONSTANTS::HOP_SIZE;

    StftProcessor stft(windowSize, hopSize);
    auto magnitudes = stft.computeMagnitudes(samples);

    //SpectrogramPngWriter::write((path.parent_path() / (path.stem().string() + "_spectrogram.png")).string(),magnitudes);

    FeatureExtractor extractor(sampleRate);

    std::vector<FrameFeatures> frameFeatures;
    frameFeatures.reserve(magnitudes.size());

    for (size_t frameIdx = 0; frameIdx < magnitudes.size(); ++frameIdx) {
        const size_t offset = frameIdx * hopSize;
        if (offset + windowSize > samples.size()) {
            break;
        }

        double sumSq = 0.0;
        double peak = 0.0;

        for (int i = 0; i < windowSize; ++i) {
            const double s = samples[offset + i];
            sumSq += s * s;
            peak = std::max(peak, std::abs(s));
        }

        FrameFeatures f{};
        f.pcmRms = std::sqrt(sumSq / windowSize);
        f.peak = peak;

        FrameFeatures spectral = extractor.extract(magnitudes[frameIdx]);
        f.spectralRms = spectral.spectralRms;
        f.spectralCentroid = spectral.spectralCentroid;
        f.spectralRolloff85 = spectral.spectralRolloff85;
        f.spectralFlatness = spectral.spectralFlatness;
        f.hfRatio = spectral.hfRatio;
        frameFeatures.push_back(f);
    }

    TrackMetadata metadata{};
    metadata.path = path;
    metadata.sampleRate = sampleRate;
    metadata.totalSamples = samples.size();
    metadata.durationSeconds = static_cast<double>(samples.size()) / sampleRate;
    metadata.frameCount = frameFeatures.size();

    if (auto tags = AudioMetadataReader::extract(path)) {
        metadata.artist = tags->artist;
        metadata.title = tags->title;
        metadata.album = tags->album;
        metadata.year = tags->year;
    }

    Track track(metadata);
    track.setFrameFeatures(std::move(frameFeatures));
    track.setTrackFeatures(TrackAggregator::aggregate(track.getFrameFeatures()));

    return track;
}




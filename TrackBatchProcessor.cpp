#include "TrackBatchProcessor.h"

#include <iostream>

#include "Utilities/AudioMetadataExtractor.h"
#include "Resources/Constants.h"

#include "Core/Mp3Decoder.h"
#include "Core/StftProcessor.h"
#include "Core/FeatureExtractor.h"
#include "Core/TrackAggregator.h"
#include <mutex>
#include "Queue/BlockingQueue.h"

using namespace std;

TrackBatchProcessor::TrackBatchProcessor(std::filesystem::path inputDirectory)
    : inputDirectory(std::move(inputDirectory)) {
}

std::vector<Track> TrackBatchProcessor::runParallel(std::size_t workerCount, std::size_t queueCapacity) {
    namespace fs = std::filesystem;

    if (workerCount == 0)
        workerCount = 1;
    if (queueCapacity == 0)
        queueCapacity = 1;

    BlockingQueue<fs::path> workQueue(queueCapacity);

    std::vector<Track> results;
    results.reserve(256);

    std::mutex resultsMutex;
    std::mutex logMutex;

    std::atomic<std::size_t> failedCount{ 0 };
    std::atomic<std::size_t> enqueuedCount{ 0 };

    std::vector<std::thread> workers;
    workers.reserve(workerCount);

    for (std::size_t i = 0; i < workerCount; ++i) {
        workers.emplace_back([&] {
            fs::path path;

            while (workQueue.pop(path)) {
                {
                    std::lock_guard<std::mutex> lk(logMutex);
                    std::wcout << L"Processing: " << path.wstring() << L'\n';
                } 

                std::optional<Track> track;

                try {
                    track = processTrack(path);
                }
                catch (const std::exception& e) {
                    std::lock_guard<std::mutex> lk(logMutex);
                    std::wcout << L"Exception processing " << path.wstring() << L": " << e.what() << L'\n';
                }
                catch (...) {
                    std::lock_guard<std::mutex> lk(logMutex);
                    std::wcout << L"Unknown exception processing " << path.wstring() << L'\n';
                }

                if (!track) {
                    failedCount.fetch_add(1, std::memory_order_relaxed);
                    continue;
                }

                {
                    std::lock_guard<std::mutex> lk(resultsMutex);
                    results.push_back(std::move(*track));
                }
            }
            });
    }

    std::thread producer([&] {
        try {
            for (const auto& entry :
                fs::recursive_directory_iterator(inputDirectory)) {

                if (!entry.is_regular_file())
                    continue;

                const auto& path = entry.path();
                const auto ext = path.extension();

                if (ext != ".mp3" && ext != ".MP3")
                    continue;

                if (!workQueue.push(path))
                    break;

                enqueuedCount.fetch_add(1, std::memory_order_relaxed);
            }
        }
        catch (const std::exception& e) {
            std::lock_guard<std::mutex> lk(logMutex);
            std::wcout << L"Filesystem error: " << e.what() << L'\n';
        }

        workQueue.close();
        });

    producer.join();
    for (auto& w : workers)
        w.join();

	std::lock_guard<std::mutex> lk(logMutex);
	std::wcout << L"Processed: " << results.size() << L", Failed: " << failedCount.load() << L", Enqueued: " << enqueuedCount.load() << L'\n';
    return results;
}



std::optional<Track> TrackBatchProcessor::processTrack(const std::filesystem::path& path) {
    auto decoded = Mp3Decoder::decode(path,CONSTANTS::WINDOW_SIZE);

    if (!decoded)
        return std::nullopt;

    std::size_t frameCount = 0;
    TrackFeatures features = extractTrackFeatures(decoded->samples, decoded->sampleRate, frameCount);
    Track track = buildTrack(path,features,decoded->sampleRate,decoded->samples.size(),frameCount);
    return track;
}

TrackFeatures TrackBatchProcessor::extractTrackFeatures(const std::vector<double>& samples, int sampleRate, std::size_t& outFrameCount) {
    const int windowSize = CONSTANTS::WINDOW_SIZE;
    const int hopSize = CONSTANTS::HOP_SIZE;

    StftProcessor stft(windowSize, hopSize);
    auto magnitudes = stft.computeMagnitudes(samples);

    FeatureExtractor extractor(sampleRate);

    std::vector<FrameFeatures> frameFeatures;
    frameFeatures.reserve(magnitudes.size());

    for (std::size_t frameIdx = 0; frameIdx < magnitudes.size(); ++frameIdx) {
        const std::size_t offset = frameIdx * hopSize;
        if (offset + windowSize > samples.size())
            break;

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

        const FrameFeatures spectral = extractor.extract(magnitudes[frameIdx]);
        f.spectralRms = spectral.spectralRms;
        f.spectralCentroid = spectral.spectralCentroid;
        f.spectralRolloff85 = spectral.spectralRolloff85;
        f.spectralFlatness = spectral.spectralFlatness;
        f.hfRatio = spectral.hfRatio;

        frameFeatures.push_back(f);
    }

    outFrameCount = frameFeatures.size();
    return TrackAggregator::aggregate(frameFeatures);
}

Track TrackBatchProcessor::buildTrack(const std::filesystem::path& path, const TrackFeatures& features, int sampleRate, std::size_t totalSamples, std::size_t frameCount) {
    TrackMetadata metadata{};
    metadata.path = path;
    metadata.sampleRate = sampleRate;
    metadata.totalSamples = totalSamples;
    metadata.durationSeconds = static_cast<double>(totalSamples) / sampleRate;
    metadata.frameCount = frameCount;

    if (auto tags = AudioMetadataReader::extract(path)) {
        metadata.artist = tags->artist;
        metadata.title = tags->title;
        metadata.album = tags->album;
        metadata.year = tags->year;
    }

    Track track(metadata);
    track.setTrackFeatures(features);
    return track;
}

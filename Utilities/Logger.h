#pragma once

#include <filesystem>
#include <mutex>
#include <iostream>

class Logger {
public:
    explicit Logger(std::wostream& out = std::wcout);

    void logGroupChange(const std::filesystem::path& groupPath);
    void logException(const std::filesystem::path& file, const wchar_t* msg);
    void logException(const std::filesystem::path& file, const std::exception& e);
    void logFilesystemError(const std::exception& e);
    void logSummary(std::size_t processed, std::size_t failed, std::size_t enqueued);

private:
    std::wostream& out;
    std::mutex ioMutex;

    static thread_local std::filesystem::path lastGroup;
};
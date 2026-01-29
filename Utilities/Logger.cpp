#include "Logger.h"

thread_local std::filesystem::path Logger::lastGroup;

Logger::Logger(std::wostream& out)
    : out(out) {
}

void Logger::logGroupChange(const std::filesystem::path& groupPath) {
    if (groupPath == lastGroup)
        return;

    std::lock_guard<std::mutex> lk(ioMutex);
    out << L"\n=== " << groupPath.wstring() << L" ===\n";
    lastGroup = groupPath;
}

void Logger::logException(const std::filesystem::path& file, const wchar_t* msg) {
    std::lock_guard<std::mutex> lk(ioMutex);
    out << L"Exception processing " << file.wstring() << L": " << msg << L'\n';
}

void Logger::logException(const std::filesystem::path& file, const std::exception& e) {
    std::lock_guard<std::mutex> lk(ioMutex);
    out << L"Exception processing " << file.wstring() << L": " << e.what() << L'\n';
}

void Logger::logFilesystemError(const std::exception& e) {
    std::lock_guard<std::mutex> lk(ioMutex);
    out << L"Filesystem error: " << e.what() << L'\n';
}

void Logger::logSummary(std::size_t processed, std::size_t failed, std::size_t enqueued) {
	std::lock_guard<std::mutex> lk(ioMutex);
    out << L"Processed: " << processed << L", Failed: " << failed << L", Enqueued: " << enqueued << L'\n';
}
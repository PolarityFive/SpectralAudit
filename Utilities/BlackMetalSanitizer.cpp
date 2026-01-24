#include "BlackMetalSanitizer.h"

#include <fstream>
#include <cctype>
#include <functional>

#ifdef _WIN32
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#endif

void BlackMetalSanitizer::setupConsole() {
#ifdef _WIN32
    _setmode(_fileno(stdout), _O_U16TEXT);
#endif
}


std::string BlackMetalSanitizer::sanitizeFilename(const std::filesystem::path& path) {
    auto toUtf8 = [](const std::u8string& s) {
        return std::string(reinterpret_cast<const char*>(s.data()), s.size());
    };

    std::string name = toUtf8(path.filename().u8string());

    for (char& c : name) {
        if (!(std::isalnum(static_cast<unsigned char>(c)) ||
            c == '.' || c == '_' || c == '-')) {
            c = '_';
        }
    }

    std::hash<std::string> h;
    return std::to_string(h(toUtf8(path.u8string()))) + "_" + name;
}


std::filesystem::path BlackMetalSanitizer::makeSafeTempCopy(const std::filesystem::path& originalPath) {
    namespace fs = std::filesystem;

    fs::path tempDir = fs::temp_directory_path() / "spectral_audit";
    fs::create_directories(tempDir);
    fs::path safePath = tempDir / sanitizeFilename(originalPath);
    fs::copy_file(originalPath, safePath,fs::copy_options::overwrite_existing);

    return safePath;
}

std::string BlackMetalSanitizer::toUtf8(const std::filesystem::path& p) {
    const auto u8 = p.u8string();
    return std::string(reinterpret_cast<const char*>(u8.data()), u8.size());
}

void BlackMetalSanitizer::cleanup(const std::filesystem::path& tempPath) {
    std::error_code ec;
    std::filesystem::remove(tempPath, ec);
}

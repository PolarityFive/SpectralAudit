#pragma once

#include <filesystem>
#include <string>

/*
 * Black metal and Phonk artists LOVE using weird characters in album and song names causing various issues when trying to pass in paths, print them on console e.t.c.
 * This should take care of that. 
 */
class BlackMetalSanitizer {
public:
    static void setupConsole();

    static std::filesystem::path makeSafeTempCopy(const std::filesystem::path& originalPath);
    static void cleanup(const std::filesystem::path& tempPath);
    static std::string toUtf8(const std::filesystem::path& p);

private:
    static std::string sanitizeFilename(const std::filesystem::path& path);
};

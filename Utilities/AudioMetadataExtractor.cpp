#include "AudioMetadataExtractor.h"

#include <taglib/fileref.h>
#include <taglib/tag.h>

std::optional<AudioTags> AudioMetadataReader::extract(const std::filesystem::path& path) {
    TagLib::FileRef file(path.c_str());

    if (file.isNull() || !file.tag()) {
        return std::nullopt;
    }

    TagLib::Tag* tag = file.tag();
    AudioTags out;

    out.artist = tag->artist().to8Bit(true);
    out.title = tag->title().to8Bit(true);
    out.album = tag->album().to8Bit(true);
    out.year = tag->year();

    return out;
}


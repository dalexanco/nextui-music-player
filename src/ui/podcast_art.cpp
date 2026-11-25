#include "ui/podcast_art.hpp"

#include <SDL2/SDL_image.h>

#include <sys/stat.h>

#include <cstdio>

#include "podcast.h"

// The NextUI headers declare C functions and have no guard of their own
extern "C" {
#include "defines.h"
}

#include "wget_fetch.h"

namespace {
/** The biggest cover that the app takes from the network */
constexpr int kMaxCoverBytes = 1024 * 1024;
}  // namespace

SDL_Surface* loadPodcastArt(const char* feedId) {
    if (!feedId || !feedId[0]) return nullptr;
    char directory[512];
    Podcast_getFeedDataPath(feedId, directory, sizeof(directory));
    char path[768];
    std::snprintf(path, sizeof(path), "%s/artwork.jpg", directory);
    SDL_Surface* loaded = IMG_Load(path);
    if (!loaded) return nullptr;
    SDL_Surface* converted = SDL_ConvertSurfaceFormat(loaded, SDL_PIXELFORMAT_ARGB8888, 0);
    SDL_FreeSurface(loaded);
    return converted;
}

bool isPodcastImageComplete(const uint8_t* data, int size) {
    if (size < 4) return false;
    // JPEG: starts with FF D8, ends with FF D9
    if (data[0] == 0xFF && data[1] == 0xD8) return data[size - 2] == 0xFF && data[size - 1] == 0xD9;
    // PNG: starts with 89 50 4E 47, ends with the IEND chunk (AE 42 60 82)
    if (data[0] == 0x89 && data[1] == 0x50 && data[2] == 0x4E && data[3] == 0x47) {
        return size >= 8 && data[size - 4] == 0xAE && data[size - 3] == 0x42 && data[size - 2] == 0x60 && data[size - 1] == 0x82;
    }
    // Another format: assumed complete
    return true;
}

namespace {

constexpr const char* kCacheParent = SDCARD_PATH "/.cache";
constexpr const char* kCacheDir    = SDCARD_PATH "/.cache/podcast";

}  // namespace

std::string getPodcastCoverPath(const char* itunesId) {
    return std::string(kCacheDir) + "/" + (itunesId ? itunesId : "") + ".jpg";
}

bool fetchPodcastCover(const char* itunesId, const char* url) {
    if (!itunesId || !itunesId[0] || !url || !url[0]) return false;
    const std::string path = getPodcastCoverPath(itunesId);
    if (std::FILE* known = std::fopen(path.c_str(), "rb")) {
        std::fclose(known);
        return true;
    }
    static uint8_t buffer[kMaxCoverBytes];
    const int size = wget_fetch_bytes(url, buffer, kMaxCoverBytes);
    if (size <= 0 || !isPodcastImageComplete(buffer, size)) return false;
    mkdir(kCacheParent, 0755);
    mkdir(kCacheDir, 0755);
    std::FILE* file = std::fopen(path.c_str(), "wb");
    if (!file) return false;
    const bool written = std::fwrite(buffer, 1, static_cast<size_t>(size), file) == static_cast<size_t>(size);
    std::fclose(file);
    if (!written) std::remove(path.c_str());
    return written;
}

#include "ui/podcast_art.hpp"

#include <SDL2/SDL_image.h>

#include <cstdio>

#include "podcast.h"

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

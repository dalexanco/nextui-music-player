#ifndef __UI_PODCAST_ART_HPP__
#define __UI_PODCAST_ART_HPP__

#include <SDL2/SDL.h>

#include <cstdint>
#include <string>

/**
 * Loads the art of the feed `feedId` from the folder of the feed, or returns nullptr when the
 * folder has none (or a file that does not decode). The surface is ARGB8888, and the caller frees it.
 */
SDL_Surface* loadPodcastArt(const char* feedId);

/** True where the image data is complete: a JPEG ends with its end marker, a PNG with its IEND chunk. */
bool isPodcastImageComplete(const uint8_t* data, int size);

/** Returns the path of the cover of a show of the search or of the charts in the cache of the app. */
std::string getPodcastCoverPath(const char* itunesId);

/**
 * Downloads the cover at `url` into the cache, under `itunesId`, unless it is there. It blocks
 * while the network answers. Returns true when the cover is in the cache afterwards.
 */
bool fetchPodcastCover(const char* itunesId, const char* url);

#endif

#ifndef __UI_PODCAST_ART_HPP__
#define __UI_PODCAST_ART_HPP__

#include <SDL2/SDL.h>

/**
 * Loads the art of the feed `feedId` from the folder of the feed, or returns nullptr when the
 * folder has none (or a file that does not decode). The surface is ARGB8888, and the caller frees it.
 */
SDL_Surface* loadPodcastArt(const char* feedId);

#endif

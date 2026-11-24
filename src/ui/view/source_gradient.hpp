#ifndef __UI_VIEW_SOURCE_GRADIENT_HPP__
#define __UI_VIEW_SOURCE_GRADIENT_HPP__

#include <cstdint>

#include <SDL2/SDL.h>

/** The two tones that give the background of a page its look: see Theme_getAccent() and Theme_getDeep(). */
struct SourcePalette {
    SDL_Color accent;
    SDL_Color deep;

    bool operator==(const SourcePalette& other) const {
        return accent.r == other.accent.r && accent.g == other.accent.g && accent.b == other.accent.b &&
               deep.r == other.deep.r && deep.g == other.deep.g && deep.b == other.deep.b;
    }
};

/**
 * Returns the opaque ARGB8888 pixel of the soft background of a page, at (x, y) of a `width` by
 * `height` screen. The background is a linear gradient at 165 degrees from the deep tone to a
 * near black, under two diffuse halos of the accent: a strong one at the top right, and a weak
 * one at the bottom left.
 */
uint32_t getSourceGradientPixel(int x, int y, int width, int height, const SourcePalette& palette);

/** Renders the whole background in `pixels`, which hold `height` rows of `pitchPixels` pixels. */
void renderSourceGradient(uint32_t* pixels, int width, int height, int pitchPixels, const SourcePalette& palette);

/**
 * Renders a tile for an art that has no image: a linear gradient at 145 degrees from `from` at the
 * top left to `to` at the bottom right, with rounded corners of `radius` pixels. The pixels are
 * ARGB8888 with premultiplied alpha, and the corners are transparent.
 */
void renderArtTile(uint32_t* pixels, int width, int height, int pitchPixels, SDL_Color from, SDL_Color to, int radius);

#endif

#ifndef __UI_VIEW_ROUNDED_RECT_HPP__
#define __UI_VIEW_ROUNDED_RECT_HPP__

#include <cstdint>

#include <SDL2/SDL.h>

/**
 * The look of a rounded rectangle: the fill, and an edge of `edgeWidth` pixels along its inside.
 * The alphas of the colors count: a fill with alpha 0 leaves the inside open, as a chip that is
 * only an outline. `radius` is the radius of the corners in pixels, clamped to half the smaller
 * side.
 */
struct RoundedRectStyle {
    SDL_Color fill;
    SDL_Color edge;
    int edgeWidth;
    int radius;
};

/**
 * Renders an anti-aliased rounded rectangle that fills a `width` by `height` bitmap: the pixels
 * are ARGB8888 with premultiplied alpha, `pitchPixels` pixels apart per row, and what lies
 * outside the corners is transparent. The NextUI helpers (`GFX_blitPill`, `GFX_ApplyRoundedCorners`,
 * `render_rounded_rect_bg`) cut the corners with no anti-aliasing and no edge, hence this one.
 */
void renderRoundedRect(uint32_t* pixels, int width, int height, int pitchPixels, const RoundedRectStyle& style);

#endif

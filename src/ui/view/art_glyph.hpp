#ifndef __UI_VIEW_ART_GLYPH_HPP__
#define __UI_VIEW_ART_GLYPH_HPP__

#include <cstdint>

/** The pictogram that stands on a generated art tile, as a placeholder for the image that is missing. */
enum class ArtGlyph {
    Music,     ///< two beamed notes
    Radio,     ///< a dot with radio waves on each side
    Podcast,   ///< a microphone
    Book,      ///< an open book
    Playlist,  ///< a bulleted list
    Folder,    ///< a folder with its tab
    Neutral,   ///< a gear
};

/**
 * Draws `glyph` over a tile that is already rendered, centered, in translucent white. The glyph
 * is anti-aliased and spans about 40% of the smaller side, so it scales with the tile. The pixels
 * are ARGB8888 with premultiplied alpha, `pitchPixels` pixels apart per row, and the tile under
 * them should be opaque where the glyph falls. `alpha` is the opacity of the white at the full
 * coverage of the glyph. Never draws a text.
 */
void renderArtGlyph(uint32_t* pixels, int width, int height, int pitchPixels, ArtGlyph glyph, uint8_t alpha);

/** The coverage, from 0 to 1, of the pixel centered at (x + 0.5, y + 0.5) by `glyph` on a tile of `width` by `height`. */
float getArtGlyphCoverage(ArtGlyph glyph, int x, int y, int width, int height);

#endif

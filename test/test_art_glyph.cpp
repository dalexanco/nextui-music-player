// Host tests of the pictograms of the generated art tiles: they draw something, centered, scale
// with the tile, and the symmetric ones are.

#include <cmath>
#include <vector>

#include "test.h"
#include "ui/view/art_glyph.hpp"

namespace {

constexpr ArtGlyph kGlyphs[] = {ArtGlyph::Music, ArtGlyph::Radio,    ArtGlyph::Podcast, ArtGlyph::Book,
                                ArtGlyph::Playlist, ArtGlyph::Folder, ArtGlyph::Neutral};

/** The sum of the coverage over the tile, and the centroid of it */
struct Mass {
    double sum = 0, cx = 0, cy = 0;
    int minX = 1 << 30, maxX = -1, minY = 1 << 30, maxY = -1;
};

Mass measure(ArtGlyph glyph, int size) {
    Mass mass;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            const float c = getArtGlyphCoverage(glyph, x, y, size, size);
            if (c <= 0.0f) continue;
            mass.sum += c;
            mass.cx += c * (x + 0.5);
            mass.cy += c * (y + 0.5);
            if (c >= 0.5f) {
                mass.minX = std::min(mass.minX, x);
                mass.maxX = std::max(mass.maxX, x);
                mass.minY = std::min(mass.minY, y);
                mass.maxY = std::max(mass.maxY, y);
            }
        }
    }
    return mass;
}

}  // namespace

TEST(each_glyph_draws_something_around_the_center) {
    const int size = 340;
    for (ArtGlyph glyph : kGlyphs) {
        const Mass m = measure(glyph, size);
        CHECK(m.sum > 500.0);
        // The glyph stays within the middle 50% of the tile, and its mass is near the center
        CHECK(m.minX > size / 4);
        CHECK(m.maxX < size * 3 / 4);
        CHECK(m.minY > size / 4);
        CHECK(m.maxY < size * 3 / 4);
        CHECK(std::fabs(m.cx / m.sum - size / 2.0) < size * 0.05);
        CHECK(std::fabs(m.cy / m.sum - size / 2.0) < size * 0.07);
    }
}

TEST(the_glyph_is_about_40_percent_of_the_tile) {
    const int size = 400;
    for (ArtGlyph glyph : kGlyphs) {
        const Mass m = measure(glyph, size);
        const int width  = m.maxX - m.minX + 1;
        const int height = m.maxY - m.minY + 1;
        const int larger = std::max(width, height);
        CHECK(larger > size * 0.28);
        CHECK(larger <= size * 0.41);
    }
}

TEST(the_glyph_scales_with_the_tile) {
    for (ArtGlyph glyph : kGlyphs) {
        const double small = measure(glyph, 120).sum;
        const double large = measure(glyph, 240).sum;
        // Twice the side, four times the area
        CHECK(std::fabs(large / small - 4.0) < 0.5);
    }
}

TEST(the_symmetric_glyphs_are_mirrored) {
    const int size = 300;
    for (ArtGlyph glyph : {ArtGlyph::Radio, ArtGlyph::Podcast, ArtGlyph::Book, ArtGlyph::Neutral}) {
        for (int y = 0; y < size; y += 7) {
            for (int x = 0; x < size; x += 7) {
                const float a = getArtGlyphCoverage(glyph, x, y, size, size);
                const float b = getArtGlyphCoverage(glyph, size - 1 - x, y, size, size);
                CHECK(std::fabs(a - b) < 0.01f);
            }
        }
    }
}

TEST(the_edge_is_anti_aliased) {
    const int size = 300;
    int partial    = 0;
    for (int y = 0; y < size; y++) {
        for (int x = 0; x < size; x++) {
            const float c = getArtGlyphCoverage(ArtGlyph::Radio, x, y, size, size);
            if (c > 0.05f && c < 0.95f) partial++;
        }
    }
    CHECK(partial > 100);
}

TEST(render_blends_translucent_white_over_the_tile_and_leaves_the_rest) {
    const int size = 200;
    // An opaque tile of a dark red, premultiplied (alpha 255)
    const uint32_t tile = 0xff300808;
    std::vector<uint32_t> pixels(static_cast<size_t>(size) * size, tile);
    renderArtGlyph(pixels.data(), size, size, size, ArtGlyph::Neutral, 200);
    // A corner is untouched
    CHECK_EQ_INT(pixels[0], tile);
    // Some pixel is lighter, and still opaque
    bool lighter = false;
    for (uint32_t pixel : pixels) {
        CHECK_EQ_INT(pixel >> 24, 0xff);
        if (((pixel >> 16) & 0xff) > 0xc0) lighter = true;
    }
    CHECK(lighter);
    // The glyph is translucent: a full coverage pixel does not reach pure white on a red tile
    for (uint32_t pixel : pixels) CHECK(((pixel >> 8) & 0xff) < 0xff);
}

int main() {
    RUN(each_glyph_draws_something_around_the_center);
    RUN(the_glyph_is_about_40_percent_of_the_tile);
    RUN(the_glyph_scales_with_the_tile);
    RUN(the_symmetric_glyphs_are_mirrored);
    RUN(the_edge_is_anti_aliased);
    RUN(render_blends_translucent_white_over_the_tile_and_leaves_the_rest);
    return test_summary();
}

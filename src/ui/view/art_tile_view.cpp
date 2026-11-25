#include "ui/view/art_tile_view.hpp"

#include <algorithm>

#include "ui/view/source_gradient.hpp"

// The NextUI headers declare C functions and have no guard of their own
extern "C" {
#include "defines.h"
#include "api.h"
}

namespace {

/** How much of white the light end of the tile holds, and how much of the deep tone the dark end */
constexpr int kLightMixPercent = 20;
constexpr int kDarkMixPercent  = 55;

/** The opacity of the white of the glyph: 80% */
constexpr uint8_t kGlyphAlpha = 204;

}  // namespace

ArtGlyph getArtGlyphOfSource(ThemeSource source) {
    switch (source) {
        case THEME_SOURCE_MUSIC: return ArtGlyph::Music;
        case THEME_SOURCE_RADIO: return ArtGlyph::Radio;
        case THEME_SOURCE_PODCAST: return ArtGlyph::Podcast;
        case THEME_SOURCE_AUDIOBOOK: return ArtGlyph::Book;
        default: return ArtGlyph::Neutral;
    }
}

void ArtTileFallbackView::setSource(ThemeSource newSource) {
    if (newSource == source) return;
    source = newSource;
    if (!glyphChosen) glyph = getArtGlyphOfSource(source);
    dirty = true;
}

void ArtTileFallbackView::setGlyph(ArtGlyph newGlyph) {
    if (glyphChosen && newGlyph == glyph) return;
    glyphChosen = true;
    glyph       = newGlyph;
    dirty       = true;
}

void ArtTileFallbackView::clearGlyph() {
    if (!glyphChosen) return;
    glyphChosen = false;
    glyph       = getArtGlyphOfSource(source);
    dirty       = true;
}

void ArtTileFallbackView::draw(UiDrawContext& context) {
    dirty                = false;
    SDL_Surface* surface = context.getSurface();
    if (!surface) return;
    const SDL_Color accent = Theme_getAccent(source);
    // The canvas draws the tile from a lighter accent to a darker one
    //noinspection HardcodedColor
    const SDL_Color white = {255, 255, 255, 255};
    renderArtTile(static_cast<uint32_t*>(surface->pixels), surface->w, surface->h, surface->pitch / 4,
                  Theme_mix(accent, white, kLightMixPercent), Theme_mix(accent, Theme_getDeep(source), kDarkMixPercent),
                  SCALE1(8));
    renderArtGlyph(static_cast<uint32_t*>(surface->pixels), surface->w, surface->h, surface->pitch / 4, glyph, kGlyphAlpha);
}

void ArtTileView::layout(UiLayoutContext& context) {
    const UiSize space = context.getMaxSize();
    if (bitmapView.hasBitmap()) {
        context.layoutChild(bitmapView, SDL_Rect{0, 0, space.w, space.h});
        return;
    }
    context.layoutChild(tileView, SDL_Rect{0, 0, space.w, space.h});
}

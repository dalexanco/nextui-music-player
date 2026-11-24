#include "ui/view/art_tile_view.hpp"

#include <algorithm>

#include "ui/view/source_gradient.hpp"
#include "ui_fonts.h"

// The NextUI headers declare C functions and have no guard of their own
extern "C" {
#include "defines.h"
#include "api.h"
}

namespace {

/** How much of white the light end of the tile holds, and how much of the deep tone the dark end */
constexpr int kLightMixPercent = 30;
constexpr int kDarkMixPercent  = 55;

}  // namespace

void ArtTileFallbackView::setSource(ThemeSource newSource) {
    if (newSource == source) return;
    source = newSource;
    dirty  = true;
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
}

ArtTileView::ArtTileView() : titleView(Fonts_getMedium(), true) {
    titleView.setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
}

void ArtTileView::setFallback(const std::string& title, ThemeSource source) {
    titleView.setText(title);
    tileView.setSource(source);
}

void ArtTileView::layout(UiLayoutContext& context) {
    const UiSize space = context.getMaxSize();
    if (bitmapView.hasBitmap()) {
        context.layoutChild(bitmapView, SDL_Rect{0, 0, space.w, space.h});
        return;
    }
    context.layoutChild(tileView, SDL_Rect{0, 0, space.w, space.h});
    const int padding         = SCALE1(PADDING) * 2;
    const int textWidth       = std::max(0, space.w - padding * 2);
    UiLayoutContext measuring = context.createMeasuringContext();
    const int textHeight      = measuring.layoutChild(titleView, SDL_Rect{0, 0, textWidth, space.h}).getSize().h;
    context.layoutChild(titleView, SDL_Rect{padding, std::max(0, space.h - padding - textHeight), textWidth, textHeight});
}

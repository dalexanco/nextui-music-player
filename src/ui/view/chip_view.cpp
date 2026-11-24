#include "ui/view/chip_view.hpp"

#include <algorithm>

#include "ui_fonts.h"
#include "ui_theme.h"

// The NextUI headers declare C functions and have no guard of their own
extern "C" {
#include "defines.h"
#include "api.h"
}

void PillView::draw(UiDrawContext& context) {
    dirty                = false;
    SDL_Surface* surface = context.getSurface();
    if (!surface) return;
    SDL_Rect pill{0, 0, surface->w, surface->h};
    context.setContentRect(pill);
    // The fill argument marks a pill with no separate center color
    //noinspection HardcodedColor
    GFX_blitPillColor(ASSET_WHITE_PILL, surface, &pill, Theme_getPackedColor(THEME_ROLE_BAND_BACKGROUND, false), RGB_WHITE);
}

ChipView::ChipView() : text(Fonts_getTiny(), false) {
    text.setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
}

void ChipView::layout(UiLayoutContext& context) {
    const int height          = SCALE1(PILL_SIZE) / 2;
    const int padding         = SCALE1(9);
    UiLayoutContext measuring = context.createMeasuringContext();
    const UiSize textSize     = measuring.layoutChild(text, SDL_Rect{0, 0, context.getMaxSize().w, height}).getSize();
    const int width           = std::min(context.getMaxSize().w, textSize.w + 2 * padding);
    context.setSize(UiSize{width, height});
    context.layoutChild(pill, SDL_Rect{0, 0, width, height});
    context.layoutChild(text, SDL_Rect{padding, (height - textSize.h) / 2, textSize.w, textSize.h});
}

#include "ui/view/chip_view.hpp"

#include <algorithm>

#include "ui_fonts.h"
#include "ui_theme.h"

// The NextUI headers declare C functions and have no guard of their own
extern "C" {
#include "defines.h"
#include "api.h"
}

ChipView::ChipView() : text(Fonts_getTiny(), false) {
    text.setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
}

void ChipView::setOutline() {
    outline  = true;
    hasColor = false;
}

void ChipView::setOutline(SDL_Color color) {
    outline      = true;
    hasColor     = true;
    outlineColor = color;
    text.setColor(color);
}

void ChipView::setPill() {
    outline  = false;
    hasColor = false;
}

void ChipView::layout(UiLayoutContext& context) {
    const int height          = SCALE1(PILL_SIZE) / 2;
    const int padding         = SCALE1(9);
    UiLayoutContext measuring = context.createMeasuringContext();
    const UiSize textSize     = measuring.layoutChild(text, SDL_Rect{0, 0, context.getMaxSize().w, height}).getSize();
    const int width           = std::min(context.getMaxSize().w, textSize.w + 2 * padding);
    context.setSize(UiSize{width, height});
    const SDL_Color none = {0, 0, 0, 0};
    if (outline) {
        const SDL_Color edge = hasColor ? outlineColor : Theme_getColor(THEME_ROLE_PRIMARY, false);
        if (!hasColor) text.setColor(edge);
        shape.setStyle(RoundedRectStyle{none, edge, 2, SCALE1(2)});
    } else {
        // The board fills the pill with white at 14 %
        SDL_Color fill = Theme_getColor(THEME_ROLE_PRIMARY, false);
        fill.a         = 36;
        shape.setStyle(RoundedRectStyle{fill, none, 0, 0});
    }
    context.layoutChild(shape, SDL_Rect{0, 0, width, height});
    context.layoutChild(text, SDL_Rect{padding, (height - textSize.h) / 2, textSize.w, textSize.h});
}

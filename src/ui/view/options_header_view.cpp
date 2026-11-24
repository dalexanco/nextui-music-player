#include "ui/view/options_header_view.hpp"

#include <algorithm>

#include "ui/view/options_panel_layout.hpp"
#include "ui_fonts.h"
#include "ui_theme.h"

// The NextUI headers declare C functions and have no guard of their own
extern "C" {
#include "defines.h"
#include "api.h"
}

OptionsHeaderView::OptionsHeaderView() : meta(Fonts_getMedium(), true), title(Fonts_getHeading(), false) {
    meta.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
    title.setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
    title.startScrolling();
}

void OptionsHeaderView::layout(UiLayoutContext& context) {
    const UiSize screen       = context.getMaxSize();
    const int x               = SCALE1(PADDING) + SCALE1(BUTTON_PADDING);
    const int y               = screen.h * 132 / 768;
    const int width           = std::max(0, getOptionsPanelLeft(screen.w, SCALE1(PADDING)) - SCALE1(PADDING) - x);
    UiLayoutContext measuring = context.createMeasuringContext();
    const int metaHeight      = measuring.layoutChild(meta, SDL_Rect{0, 0, width, screen.h}).getSize().h;
    context.layoutChild(meta, SDL_Rect{x, y, width, metaHeight});
    const int titleHeight = measuring.layoutChild(title, SDL_Rect{0, 0, width, screen.h}).getSize().h;
    context.layoutChild(title, SDL_Rect{x, y + metaHeight + SCALE1(6), width, titleHeight});
}

#include "ui/view/info_block_view.hpp"

#include <algorithm>

#include "ui/view/detail_panel_layout.hpp"
#include "ui_fonts.h"
#include "ui_theme.h"

// The NextUI headers declare C functions and have no guard of their own
extern "C" {
#include "defines.h"
#include "api.h"
}

InfoBlockView::InfoBlockView()
    : lineView(Fonts_getMedium(), false), positionView(Fonts_getSmall(), false), timesView(Fonts_getSmall(), false) {
    lineView.setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
    positionView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
    timesView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
}

void InfoBlockView::onAttach(UiContext&) {
    // A view that left the layout stopped: each text starts again, and moves only if it is clipped
    lineView.startScrolling();
    positionView.startScrolling();
    timesView.startScrolling();
}

void InfoBlockView::setProgress(int percent) {
    barShown = percent >= 0;
    if (barShown) barView.setPercent(percent);
}

void InfoBlockView::layout(UiLayoutContext& context) {
    const int width           = context.getMaxSize().w;
    UiLayoutContext measuring = context.createMeasuringContext();
    const SDL_Rect room{0, 0, width, context.getMaxSize().h};
    // The rows keep the height of their font, empty or not
    const int lineHeight = measuring.layoutChild(lineView, room).getSize().h;
    const int rowHeight  = measuring.layoutChild(positionView, room).getSize().h;
    const int timesWidth = measuring.layoutChild(timesView, room).getSize().w;
    // The measure of the position is its whole text, which is cut to the width of the block
    const int positionWidth = measuring.layoutChild(positionView, room).getSize().w;

    const int gap               = SCALE1(5);
    const InfoBlockRects rects  = layoutInfoBlock(width, lineHeight, SCALE1(3), rowHeight, gap, timesWidth, positionWidth);
    context.setSize(UiSize{width, rects.height});
    context.layoutChild(lineView, rects.line);
    if (barShown) context.layoutChild(barView, rects.bar);
    context.layoutChild(positionView, rects.position);
    context.layoutChild(timesView, rects.times);
}

#include "ui/view/detail_panel_view.hpp"

#include "ui/view/detail_panel_layout.hpp"

// The NextUI headers declare C functions and have no guard of their own
extern "C" {
#include "defines.h"
#include "api.h"
}

void DetailPanelView::layout(UiLayoutContext& context) {
    const UiSize space        = context.getMaxSize();
    const int gap             = SCALE1(PADDING);
    UiLayoutContext measuring = context.createMeasuringContext();
    const int infoHeight      = measuring.layoutChild(info, SDL_Rect{0, 0, space.w, space.h}).getSize().h;
    const DetailPanelRects rects = layoutDetailPanel(space.w, space.h, infoHeight, gap);
    context.layoutChild(art, rects.art);
    context.layoutChild(info, rects.info);
}

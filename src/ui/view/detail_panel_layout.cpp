#include "ui/view/detail_panel_layout.hpp"

#include <algorithm>

SidePanelRects layoutSidePanel(const SDL_Rect& body, int screenWidth, int margin) {
    const int panelWidth = std::min(screenWidth / 3, body.w);
    const int panelX     = body.x + body.w - panelWidth;
    const int listWidth  = std::max(0, panelX - margin - body.x);
    const int panelTop   = std::min(margin, body.h);
    return SidePanelRects{
        .list  = {body.x, body.y, listWidth, body.h},
        .panel = {panelX, body.y + panelTop, panelWidth, body.h - panelTop},
    };
}

DetailPanelRects layoutDetailPanel(int width, int height, int infoHeight, int gap) {
    const int side = std::clamp(std::min(width, height - infoHeight - gap), 0, std::max(0, width));
    return DetailPanelRects{
        .art  = {(width - side) / 2, 0, side, side},
        .info = {0, side + gap, width, infoHeight},
    };
}

InfoBlockRects layoutInfoBlock(int width, int lineHeight, int barHeight, int rowHeight, int gap, int timesWidth,
                               int positionWidth) {
    InfoBlockRects rects{};
    int y         = 0;
    rects.line    = {0, y, width, lineHeight};
    y += lineHeight + gap;
    rects.bar     = {0, y, width, barHeight};
    y += barHeight + gap;
    const int times = std::min(timesWidth, width);
    if (positionWidth > 0 && times > 0 && positionWidth + gap + times > width) {
        // The times do not fit beside the position: a row of their own, at the left
        rects.position = {0, y, width, rowHeight};
        y += rowHeight;
        rects.times = {0, y, times, rowHeight};
        y += rowHeight;
        rects.height = y;
        return rects;
    }
    rects.times     = {width - times, y, times, rowHeight};
    rects.position  = {0, y, std::max(0, width - times - gap), rowHeight};
    y += rowHeight;
    rects.height = y;
    return rects;
}

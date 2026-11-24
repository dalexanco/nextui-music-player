#include "ui/view/options_panel_layout.hpp"

#include <algorithm>

int getOptionsPanelLeft(int screenWidth, int padding) { return screenWidth - padding - screenWidth * 600 / 1024; }

OptionsPanelRects layoutOptionsPanel(int screenWidth, int screenHeight, int padding, int headerHeight, int rowHeight,
                                     int rowCount, int bottomLimit) {
    OptionsPanelRects rects{};
    const int width   = screenWidth * 600 / 1024;
    const int top     = screenHeight * 120 / 768;
    const int margin  = padding;
    // The panel keeps a margin above and under the rows, as the board does
    const int wanted  = margin + headerHeight + rowCount * rowHeight + margin;
    const int maxH    = std::max(0, bottomLimit - top);
    const int height  = std::min(wanted, maxH);
    rects.panel       = SDL_Rect{getOptionsPanelLeft(screenWidth, padding), top, width, height};
    rects.header      = SDL_Rect{rects.panel.x + padding * 2, rects.panel.y + margin, width - padding * 4, headerHeight};
    const int rowsTop = rects.header.y + headerHeight;
    rects.rows        = SDL_Rect{rects.panel.x + padding, rowsTop, width - padding * 2,
                                 std::max(0, rects.panel.y + height - margin - rowsTop)};
    return rects;
}

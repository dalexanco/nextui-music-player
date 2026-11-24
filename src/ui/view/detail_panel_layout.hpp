#ifndef __UI_VIEW_DETAIL_PANEL_LAYOUT_HPP__
#define __UI_VIEW_DETAIL_PANEL_LAYOUT_HPP__

#include <SDL2/SDL.h>

/*
 * The geometry of a screen with a detail panel, as pure functions of integers. The views call them
 * at their layout, and a host test checks them with no display.
 */

/** The rectangles of a list and of its side panel */
struct SidePanelRects {
    SDL_Rect list;
    SDL_Rect panel;
};

/**
 * Splits `body` into a list at the left and a side panel at the right. The panel is one third of
 * `screenWidth` wide and ends at the right edge of `body`. `margin` is the gap between the list
 * and the panel, and it is also the space above the panel. The list keeps the height of `body`.
 */
SidePanelRects layoutSidePanel(const SDL_Rect& body, int screenWidth, int margin);

/** The rectangles of the art and of the info block, in the coordinates of the panel */
struct DetailPanelRects {
    SDL_Rect art;
    SDL_Rect info;
};

/**
 * Puts a square art of the width of the panel at the top, and the info block `gap` under it. An
 * info block of `infoHeight` that does not fit under the art in `height` makes the art smaller,
 * and the art then stays in the middle of the width.
 */
DetailPanelRects layoutDetailPanel(int width, int height, int infoHeight, int gap);

/** The rectangles of the rows of the info block, in the coordinates of the block */
struct InfoBlockRects {
    SDL_Rect line;
    SDL_Rect bar;
    SDL_Rect position;
    SDL_Rect times;
    int height;
};

/**
 * Stacks the line, the progress bar, and the row of the position and of the times, `gap` apart. The
 * position is at the left of the last row, and the times are at its right, `timesWidth` wide. The
 * position takes the room that the times leave.
 *
 * With `positionWidth`, the width that the position needs, the times go to a row of their own under
 * it when the two do not fit side by side, thus the position keeps the whole width and does not
 * shrink to a few letters. Pass 0 to keep both on one row.
 */
InfoBlockRects layoutInfoBlock(int width, int lineHeight, int barHeight, int rowHeight, int gap, int timesWidth,
                               int positionWidth = 0);

#endif

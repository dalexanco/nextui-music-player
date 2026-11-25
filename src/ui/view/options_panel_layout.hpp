#ifndef __UI_VIEW_OPTIONS_PANEL_LAYOUT_HPP__
#define __UI_VIEW_OPTIONS_PANEL_LAYOUT_HPP__

#include <SDL2/SDL.h>

/** The rectangles of the options panel, in the coordinates of the screen */
struct OptionsPanelRects {
    SDL_Rect panel;
    /** The row of the title and the scope at the top of the panel */
    SDL_Rect header;
    /** The room of the rows, inside the side margins of the panel */
    SDL_Rect rows;
};

/** Returns the left edge of the panel, which the text at its left must not pass. */
int getOptionsPanelLeft(int screenWidth, int padding);

/**
 * Puts the panel of the board `tpl-options` at the right of the screen: 600 of 1024 wide, from 120
 * of 768 down, as high as its header and `rowCount` rows of `rowHeight` need, but never lower than
 * `bottomLimit` (the top of the row of button hints). `padding` is the margin of the screen, and
 * `headerHeight` the height of the title row.
 */
OptionsPanelRects layoutOptionsPanel(int screenWidth, int screenHeight, int padding, int headerHeight, int rowHeight,
                                     int rowCount, int bottomLimit);

#endif

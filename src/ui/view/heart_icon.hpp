#ifndef __UI_VIEW_HEART_ICON_HPP__
#define __UI_VIEW_HEART_ICON_HPP__

#include <SDL2/SDL.h>

/**
 * The heart of a favourite, drawn once for each color of the primary text of a row that is selected
 * or not. The surface belongs to the program, and the icon view of a row only reads it.
 */
SDL_Surface* getHeartIcon(bool selected);

#endif

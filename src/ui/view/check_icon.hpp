#ifndef __UI_VIEW_CHECK_ICON_HPP__
#define __UI_VIEW_CHECK_ICON_HPP__

#include <SDL2/SDL.h>

/**
 * The check mark of a row that is done (an episode that was listened to), drawn once for each
 * color of the primary text of a row that is selected or not. The surface belongs to the program,
 * and the icon view of a row only reads it.
 */
SDL_Surface* getCheckIcon(bool selected);

#endif

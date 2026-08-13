#ifndef __MODULE_MENU_H__
#define __MODULE_MENU_H__

#include <SDL2/SDL.h>

// Menu selection results
#define MENU_NOW_PLAYING    0  // Mutable first slot; only shown while audio plays
#define MENU_LIBRARY        1
#define MENU_AUDIOBOOK      2
#define MENU_RADIO          3
#define MENU_PODCAST        4
#define MENU_SETTINGS       5
#define MENU_QUIT          -1

// Number of menu items when the mutable first slot is shown.
// The first slot occupies index 0, so this is MENU_SETTINGS + 1.
#define MENU_ITEM_COUNT     (MENU_SETTINGS + 1)

// First-item mode for the menu.
// Per-domain resume lives inside each domain's own menu (Music, Audiobook),
// so the main menu's first slot only ever reflects live background playback.
#define MENU_FIRST_NONE         0
#define MENU_FIRST_NOW_PLAYING  1

// Run the main menu
// Returns: menu item index (MENU_*) or MENU_QUIT (-1) if user wants to exit
int MenuModule_run(SDL_Surface* screen);

// Set toast message (called by modules returning to menu with a message)
void MenuModule_setToast(const char* message);

#endif

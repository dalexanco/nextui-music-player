#ifndef __UI_THEME_H__
#define __UI_THEME_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <SDL2/SDL.h>
#include <stdbool.h>

// The meaning that an element has in the interface.
// Get the color of an element from the Theme_* functions. Do not write one.
//
// There are two sets of colors, for the regular case and for the selected
// case. See the Theme_get*Color functions.

typedef enum {
    // The background of a screen, behind everything that the screen draws.
    THEME_ROLE_PAGE_BACKGROUND,
    // The band behind a whole row.
    THEME_ROLE_BAND_BACKGROUND,
    // The background of a panel such as a dialog or a toast.
    THEME_ROLE_SURFACE_BACKGROUND,

    // Content that is the primary focus of the user's attention: a title, the
    // text of a list item, the border of a dialog.
    THEME_ROLE_PRIMARY,
    // Content that supplements the primary one: a description, a hint, a date,
    // a duration.
    THEME_ROLE_SECONDARY,
    // The left part of a progress bar (completed portion).
    THEME_ROLE_PROGRESS_FILL,
    // The right part of a progress bar (not completed portion).
    THEME_ROLE_PROGRESS_TRACK,
    // Green-hued color that signifies success of an operation.
    THEME_ROLE_STATUS_SUCCESS,
    // Red-hued color that signifies failure of an operation.
    THEME_ROLE_STATUS_ERROR,

    THEME_ROLE_COUNT,
} ThemeRole;

// The source of the content that a page shows. Each source has its own accent
// and its own deep tone, which are on the status
// pill, the hint pills and the page background.
typedef enum {
    THEME_SOURCE_MUSIC,
    THEME_SOURCE_RADIO,
    THEME_SOURCE_PODCAST,
    THEME_SOURCE_AUDIOBOOK,
    // Settings and each page that has no source of its own.
    THEME_SOURCE_NEUTRAL,

    THEME_SOURCE_COUNT,
} ThemeSource;

// The accent of a source. Pills and highlights use it. It does not depend on
// the NextUI theme, thus it needs no Theme_init().
SDL_Color Theme_getAccent(ThemeSource source);

// The deep tone of a source: the dark base of its page background. An
// unknown source gives the neutral one.
SDL_Color Theme_getDeep(ThemeSource source);

// The platform draws the pills of the status group and of the button hints with
// the accent colour of the NextUI theme, and reads it from its configuration.
// These functions replace it by the accent of a source around that drawing.
//
// The stack of pages sets the active source before each layout: it is the source
// of the top page.
void Theme_setSource(ThemeSource source);

// The accent of the active source, packed as 0xRRGGBBAA. A view that draws a pill renders again when it changes.
uint32_t Theme_getPillAccent(void);

// Put the accent of the active source in the pill colour of the platform, for
// the draw calls between the two functions. The first always returns true; call
// the second after it.
// See the AccentPills class for the C++ form.
bool Theme_beginAccentPills(void);
void Theme_endAccentPills(void);

// Resolve each role from the active NextUI theme.
// Must be called after GFX_init().
void Theme_init(SDL_Surface* screen);

// A theme role is the meaning of an element on the screen. The color of that
// element also depends on whether the element belongs to selected content.
//
// The row that the cursor is on is the usual selected content. The selected
// color of the page background is the pill behind that row.

// The color of a role, packed for the format of the screen. Give this to
// SDL_FillRect and to the pill blitters of the platform. Give true for
// "selected" where the element belongs to the row that the cursor is on.
uint32_t Theme_getPackedColor(ThemeRole role, bool selected);

// The color of a role as separate channels. Give this to the font functions and
// to SDL_SetSurfaceColorMod for an icon.
SDL_Color Theme_getColor(ThemeRole role, bool selected);

// --- ui_theme_utils.c ---

// Contrast ratio of two colors, as WCAG gives it. The value is 1.0 or more. A
// value of 1.0 means that the two colors are the same to the eye.
double Theme_contrast(SDL_Color a, SDL_Color b);

// Mix a percent of "to" into "from". A percent of 0 gives "from".
SDL_Color Theme_mix(SDL_Color from, SDL_Color to, int percent);

// Give a color of this hue that a user can read on the background. The hue
// carries the meaning, thus it does not change, and the saturation is fixed at
// THEME_STATUS_SATURATION. Only the lightness moves, and only until the color
// reaches min_contrast against the background. Where the start lightness
// already reaches it, the color keeps that lightness and a theme that works
// today does not change.
SDL_Color Theme_findLegibleHue(int hue, double min_contrast,
                              SDL_Color background);

#ifdef __cplusplus
}
#endif

#endif

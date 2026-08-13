#include <stdio.h>
#include <string.h>

#include "defines.h"
#include "api.h"
#include "module_common.h"
#include "module_library.h"
#include "module_player.h"
#include "module_playlist.h"
#include "module_downloader.h"
#include "ui_utils.h"
#include "ui_fonts.h"
#include "resume.h"

// Library submenu items, as indices into the list *without* the Continue row.
// A pinned "Continue" row occupies index 0 when a track can be resumed, which
// shifts everything below it down by one (see continue_offset()).
#define LIBRARY_FILES       0
#define LIBRARY_PLAYLISTS   1
#define LIBRARY_DOWNLOADER  2
#define LIBRARY_ITEM_COUNT  3

// Help state for controls dialog
#define LIBRARY_MENU_HELP_STATE 55

static const char* library_items[] = {"Files", "Playlists", "Downloader"};
static const char* library_items_with_continue[] = {"Continue", "Files", "Playlists", "Downloader"};

// Toast state
static char library_toast_message[128] = "";
static uint32_t library_toast_time = 0;

// Cached for the render callbacks, which take no user data
static bool library_has_continue = false;

// Scroll state for the resumable track name
static ScrollTextState library_continue_scroll = {0};

// 1 while the pinned Continue row is present, else 0
static int continue_offset(void) {
    return Resume_isAvailable() ? 1 : 0;
}

// Name of the resumable track, falling back to its filename
static const char* continue_track_name(void) {
    const ResumeState* rs = Resume_getState();
    if (!rs) return NULL;
    if (rs->track_name[0]) return rs->track_name;
    const char* slash = strrchr(rs->track_path, '/');
    return slash ? slash + 1 : rs->track_path;
}

// Full label for the Continue row, so the pill is sized for the whole string
static const char* library_get_label(int index, const char* default_label,
                                      char* buffer, int buffer_size) {
    if (!library_has_continue || index != 0) return NULL;
    const char* name = continue_track_name();
    if (!name) return NULL;
    snprintf(buffer, buffer_size, "Continue: %s", name);
    return buffer;
}

// Fixed "Continue: " prefix plus the track name scrolling in the space left
static bool library_render_text(SDL_Surface* screen, int index, bool selected,
                                 int text_x, int text_y, int max_text_width) {
    if (!library_has_continue || index != 0) return false;
    // Only the selected row scrolls; default rendering covers the rest
    if (!selected) return false;

    const char* name = continue_track_name();
    if (!name) return false;

    const char* prefix = "Continue: ";
    SDL_Color text_color = Fonts_getListTextColor(true);
    TTF_Font* font = Fonts_getLarge();

    int prefix_width = 0;
    TTF_SizeUTF8(font, prefix, &prefix_width, NULL);

    SDL_Surface* prefix_surf = TTF_RenderUTF8_Blended(font, prefix, text_color);
    if (prefix_surf) {
        SDL_BlitSurface(prefix_surf, NULL, screen, &(SDL_Rect){text_x, text_y});
        SDL_FreeSurface(prefix_surf);
    }

    int remaining_width = max_text_width - prefix_width;
    if (remaining_width > 0) {
        int track_x = text_x + prefix_width;

        // Clip the track name to the pill so long names can't overflow it
        SDL_Rect old_clip;
        SDL_GetClipRect(screen, &old_clip);
        SDL_Rect clip = {track_x, text_y, remaining_width, TTF_FontHeight(font)};
        SDL_SetClipRect(screen, &clip);

        // Software scroll (use_gpu=false) to respect the SDL clip rect
        ScrollText_update(&library_continue_scroll, name, font, remaining_width,
                          text_color, screen, track_x, text_y, false);

        if (old_clip.w > 0 && old_clip.h > 0)
            SDL_SetClipRect(screen, &old_clip);
        else
            SDL_SetClipRect(screen, NULL);
    }

    return true;
}

// True while the Continue row's name is scrolling (software scroll needs redraws)
static bool library_needs_scroll_redraw(void) {
    return ScrollText_isScrolling(&library_continue_scroll) ||
           ScrollText_needsRender(&library_continue_scroll);
}

static void render_library_menu(SDL_Surface* screen, int show_setting, int menu_selected) {
    library_has_continue = (continue_offset() == 1);

    SimpleMenuConfig config = {
        .title = "Music",
        .items = library_has_continue ? library_items_with_continue : library_items,
        .item_count = LIBRARY_ITEM_COUNT + (library_has_continue ? 1 : 0),
        .btn_b_label = "BACK",
        .get_label = library_get_label,
        .render_badge = NULL,
        .get_icon = NULL,
        .render_text = library_render_text
    };
    render_simple_menu(screen, show_setting, menu_selected, &config);
    render_toast(screen, library_toast_message, library_toast_time);
}

void LibraryModule_setToast(const char* message) {
    snprintf(library_toast_message, sizeof(library_toast_message), "%s", message);
    library_toast_time = SDL_GetTicks();
}

ModuleExitReason LibraryModule_run(SDL_Surface* screen) {
    int menu_selected = 0;
    int dirty = 1;
    int show_setting = 0;

    while (1) {
        GFX_startFrame();
        PAD_poll();

        // Handle global input
        GlobalInputResult global = ModuleCommon_handleGlobalInput(screen, &show_setting, LIBRARY_MENU_HELP_STATE);
        if (global.should_quit) {
            return MODULE_EXIT_QUIT;
        }
        if (global.input_consumed) {
            if (global.dirty) dirty = 1;
            GFX_sync();
            continue;
        }

        // The Continue row can appear or vanish while this menu is open
        int has_continue = continue_offset();
        int item_count = LIBRARY_ITEM_COUNT + has_continue;
        if (menu_selected >= item_count) menu_selected = item_count - 1;

        // Keep the track name scrolling while it is the selected row
        if (library_needs_scroll_redraw()) dirty = 1;

        // Menu navigation
        int items_per_page = calc_list_layout(screen).items_per_page;
        if (PAD_justRepeated(BTN_UP)) {
            menu_selected = (menu_selected > 0) ? menu_selected - 1 : item_count - 1;
            GFX_clearLayers(LAYER_SCROLLTEXT);
            dirty = 1;
        }
        else if (PAD_justRepeated(BTN_DOWN)) {
            menu_selected = (menu_selected < item_count - 1) ? menu_selected + 1 : 0;
            GFX_clearLayers(LAYER_SCROLLTEXT);
            dirty = 1;
        }
        else if (PAD_justPressed(BTN_LEFT)) {
            int scroll = 0;
            list_page_up(&menu_selected, &scroll, item_count, items_per_page);
            GFX_clearLayers(LAYER_SCROLLTEXT);
            dirty = 1;
        }
        else if (PAD_justPressed(BTN_RIGHT)) {
            int scroll = 0;
            list_page_down(&menu_selected, &scroll, item_count, items_per_page);
            GFX_clearLayers(LAYER_SCROLLTEXT);
            dirty = 1;
        }
        else if (PAD_justPressed(BTN_X)) {
            // Forget the resumable track, dropping the Continue row
            if (has_continue && menu_selected == 0) {
                Resume_clear();
                GFX_clearLayers(LAYER_SCROLLTEXT);
                menu_selected = 0;
                dirty = 1;
            }
        }
        else if (PAD_justPressed(BTN_A)) {
            ModuleExitReason reason = MODULE_EXIT_TO_MENU;
            GFX_clearLayers(LAYER_SCROLLTEXT);

            if (has_continue && menu_selected == 0) {
                const ResumeState* rs = Resume_getState();
                if (rs) reason = PlayerModule_runResume(screen, rs);
            } else {
                switch (menu_selected - has_continue) {
                    case LIBRARY_FILES:
                        reason = PlayerModule_run(screen, false);  // File browser entry
                        break;
                    case LIBRARY_PLAYLISTS:
                        reason = PlaylistModule_run(screen);
                        break;
                    case LIBRARY_DOWNLOADER:
                        reason = DownloaderModule_run(screen);
                        break;
                }
            }

            if (reason == MODULE_EXIT_QUIT) {
                return MODULE_EXIT_QUIT;
            }

            // Sub-module returned to library menu
            dirty = 1;
        }
        else if (PAD_justPressed(BTN_B)) {
            GFX_clearLayers(LAYER_SCROLLTEXT);
            return MODULE_EXIT_TO_MENU;
        }

        // Handle power management
        ModuleCommon_PWR_update(&dirty, &show_setting);

        // Render
        if (dirty) {
            render_library_menu(screen, show_setting, menu_selected);

            if (show_setting) {
                GFX_blitHardwareHints(screen, show_setting);
            }

            GFX_flip(screen);
            dirty = 0;

            ModuleCommon_tickToast(library_toast_message, library_toast_time, &dirty);
        } else {
            GFX_sync();
        }
    }
}

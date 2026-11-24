#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defines.h"
#include "api.h"
#include "config.h"
#include "help_screen.h"
#include "module_common.h"
#include "toast.h"
#include "module_podcast.h"
#include "podcast.h"
#include "podcast_sleep.h"
#include "player.h"
#include "keyboard.h"
#include "display_helper.h"
#include "ui_podcast.h"
#include "ui_layers.h"
#include "ui_main.h"
#include "ui_utils.h"
#include "wifi.h"
#include "list_nav.h"
#include "background.h"

// Internal states
typedef enum {
    PODCAST_INTERNAL_MENU,
    PODCAST_INTERNAL_MANAGE,
    PODCAST_INTERNAL_TOP_SHOWS,
    PODCAST_INTERNAL_SEARCH_RESULTS
} PodcastInternalState;

// Module state
static ListNav podcast_manage_nav = {
    .selected       = 0,
    .scroll         = 0,
    .count          = PODCAST_MANAGE_COUNT,
    .items_per_page = 1,
};
static ListNav podcast_top_shows_nav = {
    .selected       = 0,
    .scroll         = 0,
    .count          = 0,
    .items_per_page = 1,
};
static ListNav podcast_search_nav = {
    .selected       = 0,
    .scroll         = 0,
    .count          = 0,
    .items_per_page = 1,
};
static char podcast_search_query[256] = "";
// Periodic progress saving
#define PROGRESS_SAVE_INTERVAL_MS 30000  // 30 seconds

// Confirmation dialog state
static bool show_confirm = false;
static int confirm_target_index = -1;
static char confirm_podcast_name[256] = "";
static int confirm_return_state = 0;  // 0 = menu, 1 = top_shows, 2 = search_results

void PodcastModule_autoRefresh(void) {
    // Auto-check for new episodes once per app session
    static bool auto_refreshed = false;
    if (!auto_refreshed && Wifi_isConnected() && Podcast_getSubscriptionCount() > 0) {
        Podcast_startRefreshAll();
        auto_refreshed = true;
    }
}

ModuleExitReason PodcastModule_run(DisplayContext* display) {
    Podcast_init();
    PodcastModule_autoRefresh();

    PodcastInternalState state = PODCAST_INTERNAL_MANAGE;
    ModuleExitReason reason = MODULE_EXIT_TO_MENU;
    int dirty = 1;
    int show_setting = 0;

    ScreenTitle_start(false);
    ModuleCommon_resetScreenOffHint();
    ModuleCommon_recordInputTime();
    show_confirm = false;

    ListNav_scrollToTop(&podcast_manage_nav);

    while (1) {
        ModuleCommon_frameBegin(true);
        SDL_Surface* const screen = DisplayHelper_getSurface(display);

        // Handle confirmation dialog
        // A pending stop skips the dialog, thus the global input below reports the
        // quit and the module leaves through the path that it already has.
        if (show_confirm && !ModuleCommon_stopSignalled()) {
            if (PAD_justPressed(BTN_A)) {
                // Confirm unsubscribe
                Podcast_unsubscribe(confirm_target_index);
                Toast_show("Unsubscribed", TOAST_DURATION);
                show_confirm = false;
                Podcast_clearTitleScroll();
                dirty = 1;
                ModuleCommon_frameEnd(screen);
                continue;
            } else if (PAD_justPressed(BTN_B)) {
                show_confirm = false;
                Podcast_clearTitleScroll();
                dirty = 1;
                ModuleCommon_frameEnd(screen);
                continue;
            }
            // Render confirmation dialog (covers entire screen)
            render_confirmation_dialog(screen, confirm_podcast_name, "Unsubscribe?");
            ModuleCommon_markSurfaceDrawn();
            ModuleCommon_frameEnd(screen);
            continue;
        }

        // Handle global input (skip if screen off or hint active)
        // A signal must reach the global input, thus a pending stop passes this
        // guard. ModuleCommon_handleGlobalInput() reports the quit and returns
        // before it reads any input, thus the screen-off states keep their
        // behavior.
        if (ModuleCommon_stopSignalled()
            || !ModuleCommon_isScreenOffHintActive()) {
            HelpId help_id;
            switch (state) {
                case PODCAST_INTERNAL_MENU:           help_id = HELP_PODCAST_MENU;           break;
                case PODCAST_INTERNAL_MANAGE:         help_id = HELP_PODCAST_MANAGE;         break;
                case PODCAST_INTERNAL_TOP_SHOWS:      help_id = HELP_PODCAST_TOP_SHOWS;      break;
                case PODCAST_INTERNAL_SEARCH_RESULTS: help_id = HELP_PODCAST_SEARCH_RESULTS; break;
                default:                              help_id = HELP_PODCAST_MENU;           break;
            }

            GlobalInputResult global = ModuleCommon_handleGlobalInput(screen, &show_setting, help_id);
            if (global.should_quit) {
                Podcast_cleanup();
                reason = MODULE_EXIT_QUIT;
                break;
            }
            if (global.input_consumed) {
                if (global.dirty) dirty = 1;
                ModuleCommon_frameEnd(screen);
                continue;
            }
        }

        // =========================================
        // MANAGE STATE
        // =========================================
        if (state == PODCAST_INTERNAL_MANAGE) {
            Podcast_update();
            podcast_manage_nav.items_per_page = calc_list_layout(screen).items_per_page;

            if (ListNav_step(&podcast_manage_nav, ListNavPad_read()).moved) {
                dirty = 1;
            }
            else if (PAD_justPressed(BTN_A)) {
                switch (podcast_manage_nav.selected) {
                    case PODCAST_MANAGE_SEARCH: {
                        if (!Wifi_ensureConnected(screen, show_setting)) {
                            Toast_show("Internet connection required", TOAST_DURATION);
                            dirty = 1;
                            break;
                        }
                        char* query = Keyboard_open("Search podcasts", sizeof(podcast_search_query) - 1);
                        if (query && query[0]) {
                            strncpy(podcast_search_query, query, sizeof(podcast_search_query) - 1);
                            Podcast_startSearch(podcast_search_query);
                            ListNav_scrollToTop(&podcast_search_nav);
                            state = PODCAST_INTERNAL_SEARCH_RESULTS;
                        }
                        if (query) free(query);
                        // The keyboard may have recreated the display,
                        // use continue to restart the loop to begin a fresh frame.
                        dirty = 1;
                        continue;
                    }
                    case PODCAST_MANAGE_TOP_SHOWS:
                        if (!Wifi_ensureConnected(screen, show_setting)) {
                            Toast_show("Internet connection required", TOAST_DURATION);
                            dirty = 1;
                            break;
                        }
                        Podcast_loadCharts(NULL);
                        ListNav_scrollToTop(&podcast_top_shows_nav);
                        state = PODCAST_INTERNAL_TOP_SHOWS;
                        dirty = 1;
                        break;
                }
            }
            else if (PAD_justPressed(BTN_B)) {
                state = PODCAST_INTERNAL_MENU;
                dirty = 1;
            }
        }
        // =========================================
        // TOP SHOWS STATE
        // =========================================
        else if (state == PODCAST_INTERNAL_TOP_SHOWS) {
            Podcast_update();
            const PodcastChartsStatus* chart_status = Podcast_getChartsStatus();
            podcast_top_shows_nav.items_per_page = calc_list_layout(screen).rich_items_per_page;

            if (chart_status->loading || chart_status->completed) dirty = 1;
            if (Podcast_isTitleScrolling()) Podcast_animateTitleScroll();
            if (Podcast_titleScrollNeedsRender()) dirty = 1;

            if (!chart_status->loading) {
                int count = 0;
                Podcast_getTopShows(&count);
                if (ListNav_reconcile(&podcast_top_shows_nav, count).moved) dirty = 1;

                if (ListNav_step(&podcast_top_shows_nav, ListNavPad_read()).moved) {
                    Podcast_clearTitleScroll();
                    dirty = 1;
                }
                else if (PAD_justPressed(BTN_A) && count > 0) {
                    PodcastChartItem* items = Podcast_getTopShows(&count);
                    if (podcast_top_shows_nav.selected < count) {
                        bool already_subscribed = Podcast_isSubscribedByItunesId(items[podcast_top_shows_nav.selected].itunes_id);
                        if (already_subscribed) {
                            // Find subscription index and show confirm dialog
                            int sub_count = 0;
                            PodcastFeed* feeds = Podcast_getSubscriptions(&sub_count);
                            for (int si = 0; si < sub_count; si++) {
                                if (feeds[si].itunes_id[0] && strcmp(feeds[si].itunes_id, items[podcast_top_shows_nav.selected].itunes_id) == 0) {
                                    strncpy(confirm_podcast_name, items[podcast_top_shows_nav.selected].title, sizeof(confirm_podcast_name) - 1);
                                    confirm_podcast_name[sizeof(confirm_podcast_name) - 1] = '\0';
                                    confirm_target_index = si;
                                    confirm_return_state = 1;
                                    show_confirm = true;
                                    break;
                                }
                            }
                        } else {
                            Podcast_clearTitleScroll();
                            render_podcast_loading(screen, "Subscribing...");
                            ModuleCommon_markSurfaceDrawn();
                            ModuleCommon_frameEnd(screen);
                            int sub_result = Podcast_subscribeFromItunes(items[podcast_top_shows_nav.selected].itunes_id);
                            if (sub_result == 0) {
                                Toast_show("Subscribed!", TOAST_DURATION);
                            } else {
                                const char* err = Podcast_getError();
                                Toast_show(err && err[0] ? err : "Subscribe failed", TOAST_DURATION);
                            }
                        }
                    }
                    dirty = 1;
                }
                else if (PAD_justPressed(BTN_X)) {
                    // Refresh charts - clear cache and reload
                    if (!Wifi_ensureConnected(screen, show_setting)) {
                        Toast_show("Internet connection required", TOAST_DURATION);
                    } else {
                        Podcast_clearChartsCache();
                        Podcast_loadCharts(NULL);
                        ListNav_scrollToTop(&podcast_top_shows_nav);
                        Toast_show("Refreshing...", TOAST_DURATION);
                    }
                    dirty = 1;
                }
            }

            if (PAD_justPressed(BTN_B)) {
                Podcast_clearTitleScroll();
                state = PODCAST_INTERNAL_MANAGE;
                dirty = 1;
            }
        }
        // =========================================
        // SEARCH RESULTS STATE
        // =========================================
        else if (state == PODCAST_INTERNAL_SEARCH_RESULTS) {
            Podcast_update();
            const PodcastSearchStatus* search_status = Podcast_getSearchStatus();
            podcast_search_nav.items_per_page = calc_list_layout(screen).rich_items_per_page;

            if (search_status->searching || search_status->completed) dirty = 1;
            if (Podcast_isTitleScrolling()) Podcast_animateTitleScroll();
            if (Podcast_titleScrollNeedsRender()) dirty = 1;

            if (!search_status->searching) {
                int count = 0;
                Podcast_getSearchResults(&count);
                if (ListNav_reconcile(&podcast_search_nav, count).moved) dirty = 1;

                if (ListNav_step(&podcast_search_nav, ListNavPad_read()).moved) {
                    Podcast_clearTitleScroll();
                    dirty = 1;
                }
                else if (PAD_justPressed(BTN_A) && count > 0) {
                    PodcastSearchResult* results = Podcast_getSearchResults(&count);
                    if (podcast_search_nav.selected < count) {
                        bool already_subscribed = results[podcast_search_nav.selected].feed_url[0] &&
                                                   Podcast_isSubscribed(results[podcast_search_nav.selected].feed_url);
                        if (already_subscribed) {
                            // Find subscription index and show confirm dialog
                            int sub_count = 0;
                            PodcastFeed* feeds = Podcast_getSubscriptions(&sub_count);
                            for (int si = 0; si < sub_count; si++) {
                                if (strcmp(feeds[si].feed_url, results[podcast_search_nav.selected].feed_url) == 0) {
                                    strncpy(confirm_podcast_name, results[podcast_search_nav.selected].title, sizeof(confirm_podcast_name) - 1);
                                    confirm_podcast_name[sizeof(confirm_podcast_name) - 1] = '\0';
                                    confirm_target_index = si;
                                    confirm_return_state = 2;
                                    show_confirm = true;
                                    break;
                                }
                            }
                        } else {
                            Podcast_clearTitleScroll();
                            render_podcast_loading(screen, "Subscribing...");
                            ModuleCommon_markSurfaceDrawn();
                            ModuleCommon_frameEnd(screen);
                            int sub_result;
                            if (results[podcast_search_nav.selected].feed_url[0]) {
                                sub_result = Podcast_subscribe(results[podcast_search_nav.selected].feed_url);
                            } else {
                                sub_result = Podcast_subscribeFromItunes(results[podcast_search_nav.selected].itunes_id);
                            }
                            if (sub_result == 0) {
                                Toast_show("Subscribed!", TOAST_DURATION);
                            } else {
                                const char* err = Podcast_getError();
                                Toast_show(err && err[0] ? err : "Subscribe failed", TOAST_DURATION);
                            }
                        }
                    }
                    dirty = 1;
                }
            }

            if (PAD_justPressed(BTN_B)) {
                Podcast_clearTitleScroll();
                Podcast_cancelSearch();
                state = PODCAST_INTERNAL_MANAGE;
                dirty = 1;
            }
        }
        // The module gives the screen back to the page that started it
        if (state == PODCAST_INTERNAL_MENU) {
            Podcast_clearTitleScroll();
            Podcast_saveSubscriptions();
            break;
        }

        // Handle power management
        if (!ModuleCommon_isScreenOffHintActive()) {
            ModuleCommon_PWR_update(&dirty, &show_setting);
        }

        // Render
        if (dirty) {
            if (ModuleCommon_isScreenOffHintActive()) {
                GFX_clear(screen);
                render_screen_off_hint(screen);
            } else {
                switch (state) {
                    case PODCAST_INTERNAL_MENU:
                        break;
                    case PODCAST_INTERNAL_MANAGE:
                        render_podcast_manage(screen, show_setting, podcast_manage_nav.selected,
                                              podcast_manage_nav.scroll, Podcast_getSubscriptionCount());
                        break;
                    case PODCAST_INTERNAL_TOP_SHOWS:
                        render_podcast_top_shows(screen, show_setting, podcast_top_shows_nav.selected, &podcast_top_shows_nav.scroll);
                        break;
                    case PODCAST_INTERNAL_SEARCH_RESULTS:
                        render_podcast_search_results(screen, show_setting, podcast_search_nav.selected, &podcast_search_nav.scroll);
                        break;
                }
            }

            if (show_setting) {
                GFX_blitHardwareHints(screen, show_setting);
            }

            ModuleCommon_markSurfaceDrawn();
            dirty = 0;
        }
        ScreenTitle_frameEnd(&dirty, true);
        ModuleCommon_frameEnd(screen);
    }

    return reason;
}

// Check if podcast module is active (playing)
bool PodcastModule_isActive(void) {
    return Podcast_isActive();
}

bool PodcastModule_tickPlayback(void) {
    static uint32_t last_progress_save_time = 0;
    static bool tracking = false;

    Podcast_update();
    PodcastSleep_tick();

    const uint32_t now = SDL_GetTicks();
    if (!tracking) {
        last_progress_save_time = now;
        tracking = true;
    }

    const int feed_index = Podcast_getCurrentFeedIndex();
    const int episode_index = Podcast_getCurrentEpisodeIndex();

    // Periodic progress saving
    if (Podcast_isActive() && now - last_progress_save_time >= PROGRESS_SAVE_INTERVAL_MS) {
        PodcastFeed* feed = Podcast_getSubscription(feed_index);
        PodcastEpisode* ep = feed ? Podcast_getEpisode(feed_index, episode_index) : NULL;
        if (ep) {
            int position = Player_getPosition();
            if (position > 0) {
                ep->progress_sec = position / 1000;
                Podcast_saveProgress(feed->feed_url, ep->guid, ep->progress_sec);
                Podcast_flushProgress();
            }
        }
        last_progress_save_time = now;
    }

    // Detect episode end (player stopped naturally)
    if (Player_getState() == PLAYER_STATE_STOPPED) {
        tracking = false;
        PodcastFeed* feed = Podcast_getSubscription(feed_index);
        PodcastEpisode* ep = feed ? Podcast_getEpisode(feed_index, episode_index) : NULL;
        char saved_feed_url[PODCAST_MAX_URL] = "";
        char saved_guid[PODCAST_MAX_GUID] = "";
        if (feed && ep) {
            strncpy(saved_feed_url, feed->feed_url, PODCAST_MAX_URL - 1);
            strncpy(saved_guid, ep->guid, PODCAST_MAX_GUID - 1);
        }

        Podcast_stop();

        if (saved_feed_url[0] && saved_guid[0]) {
            Podcast_markAsPlayed(saved_feed_url, saved_guid);
            Podcast_removeContinueListening(saved_feed_url, saved_guid);
            // A sleep timer that stopped the playback has no episode to mark: this one ended
        }
        if (ep) ep->progress_sec = -1;
        Podcast_flushProgress();
        PodcastSleep_clear();
        return true;
    }
    return false;
}

// Background tick: detect episode end and save progress while in menu
void PodcastModule_backgroundTick(void) {
    if (PodcastModule_tickPlayback()) {
        Background_setActive(BG_NONE);
        ModuleCommon_setAutosleepDisabled(false);
    }
}

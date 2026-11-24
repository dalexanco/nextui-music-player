#include <stdio.h>
#include <string.h>
#include "module_common.h"
#include "module_podcast.h"
#include "podcast.h"
#include "podcast_sleep.h"
#include "player.h"
#include "background.h"
#include "wifi.h"

// The playback of the podcasts that the player page and the background share. The screens are the
// native pages of ui/podcast_*.

// Periodic progress saving
#define PROGRESS_SAVE_INTERVAL_MS 30000  // 30 seconds

void PodcastModule_autoRefresh(void) {
    // Auto-check for new episodes once per app session
    static bool auto_refreshed = false;
    if (!auto_refreshed && Wifi_isConnected() && Podcast_getSubscriptionCount() > 0) {
        Podcast_startRefreshAll();
        auto_refreshed = true;
    }
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

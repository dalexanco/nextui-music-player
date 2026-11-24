#include "podcast_sleep.h"

#include <SDL2/SDL.h>

#include "podcast.h"
#include "sleep_timer.h"

static SleepTimer sleep_timer = {0};

void PodcastSleep_set(int minutes) {
    SleepTimer_set(&sleep_timer, minutes, SDL_GetTicks());
}

int PodcastSleep_getMinutes(void) {
    return sleep_timer.minutes;
}

void PodcastSleep_tick(void) {
    if (sleep_timer.minutes <= 0) return;
    if (SleepTimer_takeExpired(&sleep_timer, SDL_GetTicks())) {
        // Podcast_stop() keeps the position of the episode
        Podcast_stop();
        Podcast_flushProgress();
    }
}

void PodcastSleep_clear(void) {
    SleepTimer_set(&sleep_timer, 0, 0);
}

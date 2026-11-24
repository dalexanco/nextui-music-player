#ifndef __PODCAST_SLEEP_H__
#define __PODCAST_SLEEP_H__

#ifdef __cplusplus
extern "C" {
#endif

// The sleep timer of the podcasts. It stops the episode when its countdown ends, as the sleep timer
// of the radio stops the station. The countdown is the one of sleep_timer.h.

// Starts a countdown of `minutes`, or turns the timer off with 0.
void PodcastSleep_set(int minutes);

// Returns the length that the user chose in minutes, or 0 for a timer that is off.
int PodcastSleep_getMinutes(void);

// Stops the episode when the countdown ends, and saves its position. Call it on each frame while a
// podcast plays, also while the screen is dark.
void PodcastSleep_tick(void);

// Turns the timer off.
void PodcastSleep_clear(void);

#ifdef __cplusplus
}
#endif

#endif

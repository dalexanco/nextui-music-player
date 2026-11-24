#ifndef __MODULE_PODCAST_H__
#define __MODULE_PODCAST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <SDL2/SDL.h>
#include <stdbool.h>

#include "display_helper.h"
#include "module_common.h"

// Starts the refresh of every subscription, once for each session of the app, with a network
void PodcastModule_autoRefresh(void);

// Run the podcast module
// Handles: search, top shows, the download queue
ModuleExitReason PodcastModule_run(DisplayContext* display);

// Check if podcast module is active (playing)
bool PodcastModule_isActive(void);

// One frame of the playback of an episode, for the player page and for the background: it feeds the
// player, ticks the sleep timer, saves the position every 30 seconds, and ends the episode that
// stopped (marked as listened where it ended, not where the sleep timer stopped it). Returns true
// when the playback ended in this frame.
bool PodcastModule_tickPlayback(void);

// Background tick: detect episode end and save progress while in menu
void PodcastModule_backgroundTick(void);

#ifdef __cplusplus
}
#endif

#endif

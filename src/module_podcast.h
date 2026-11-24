#ifndef __MODULE_PODCAST_H__
#define __MODULE_PODCAST_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <SDL2/SDL.h>
#include <stdbool.h>

#include "display_helper.h"
#include "module_common.h"

// Where the module starts. The native pages that open it start it on one screen, and the module
// returns to them where it would go back to its menu.
typedef enum {
    PODCAST_ENTRY_MENU = 0,   // The menu of the module (and the player of a podcast in the background)
    PODCAST_ENTRY_MANAGE,
    PODCAST_ENTRY_QUEUE,
    PODCAST_ENTRY_PLAYER,     // The player of a podcast that plays in the background
    PODCAST_ENTRY_PLAY,       // arg: the index of the subscription, arg2: the index of its episode, which plays
    PODCAST_ENTRY_CONTINUE    // arg: the index of the entry of Continue Listening, which plays
} PodcastEntry;

// Sets the entry of the next run of the module (it is back to the menu after that run)
void PodcastModule_setEntry(PodcastEntry entry, int arg, int arg2);

// Starts the refresh of every subscription, once for each session of the app, with a network
void PodcastModule_autoRefresh(void);

// Run the podcast module
// Handles: Subscriptions, search, top shows, episodes, playback
ModuleExitReason PodcastModule_run(DisplayContext* display);

// Check if podcast module is active (playing)
bool PodcastModule_isActive(void);

// Background tick: detect episode end and save progress while in menu
void PodcastModule_backgroundTick(void);

#ifdef __cplusplus
}
#endif

#endif

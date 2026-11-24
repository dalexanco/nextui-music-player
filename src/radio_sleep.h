#ifndef __RADIO_SLEEP_H__
#define __RADIO_SLEEP_H__

#ifdef __cplusplus
extern "C" {
#endif

// The sleep timer of the radio. It stops the station when its countdown ends, as the sleep timer of
// the music stops the music. The countdown is the one of sleep_timer.h.

// Starts a countdown of `minutes`, or turns the timer off with 0.
void RadioSleep_set(int minutes);

// Returns the length that the user chose in minutes, or 0 for a timer that is off.
int RadioSleep_getMinutes(void);

// Stops the radio when the countdown ends. Call it on each frame, also while the screen is dark.
void RadioSleep_tick(void);

// Turns the timer off.
void RadioSleep_clear(void);

#ifdef __cplusplus
}
#endif

#endif

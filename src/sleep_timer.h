#ifndef __SLEEP_TIMER_H__
#define __SLEEP_TIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

// A countdown that stops the playback when it ends. It reads no clock: the caller gives the time.

typedef struct {
    int      minutes;      // the length that the user chose, 0 for no timer
    uint32_t deadline_ms;  // the time where the timer ends
} SleepTimer;

// The lengths that the options offer, in minutes. The first one is 0, which is off.
#define SLEEP_TIMER_STEP_COUNT 5
extern const int SLEEP_TIMER_STEPS[SLEEP_TIMER_STEP_COUNT];

// Starts a countdown of `minutes` from `now_ms`. A length of 0 or less turns the timer off.
void SleepTimer_set(SleepTimer* timer, int minutes, uint32_t now_ms);

// Returns true one time, when the countdown reaches its end, and then turns the timer off.
// The clock of the platform can wrap, and the comparison follows it.
bool SleepTimer_takeExpired(SleepTimer* timer, uint32_t now_ms);

// Returns the time left in milliseconds, or 0 for a timer that is off or ended.
uint32_t SleepTimer_remainingMs(const SleepTimer* timer, uint32_t now_ms);

// Returns the length of the step after (`direction` 1) or before (-1) `minutes`, wrapping around the
// steps. A length that is not a step counts as the nearest step below it.
int SleepTimer_stepMinutes(int minutes, int direction);

#ifdef __cplusplus
}
#endif

#endif

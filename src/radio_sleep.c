#include "radio_sleep.h"

#include <SDL2/SDL.h>

#include "radio.h"
#include "sleep_timer.h"

static SleepTimer sleep_timer = {0};

void RadioSleep_set(int minutes) {
    SleepTimer_set(&sleep_timer, minutes, SDL_GetTicks());
}

int RadioSleep_getMinutes(void) {
    return sleep_timer.minutes;
}

void RadioSleep_tick(void) {
    if (sleep_timer.minutes <= 0) return;
    if (SleepTimer_takeExpired(&sleep_timer, SDL_GetTicks())) Radio_stop();
}

void RadioSleep_clear(void) {
    SleepTimer_set(&sleep_timer, 0, 0);
}

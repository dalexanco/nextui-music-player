#include "sleep_timer.h"

const int SLEEP_TIMER_STEPS[SLEEP_TIMER_STEP_COUNT] = {0, 15, 30, 45, 60};

void SleepTimer_set(SleepTimer* timer, int minutes, uint32_t now_ms) {
    if (minutes <= 0) {
        timer->minutes     = 0;
        timer->deadline_ms = 0;
        return;
    }
    timer->minutes     = minutes;
    timer->deadline_ms = now_ms + (uint32_t)minutes * 60000u;
}

// The signed difference follows the wrap of the clock
static int32_t ms_until(uint32_t deadline_ms, uint32_t now_ms) {
    return (int32_t)(deadline_ms - now_ms);
}

bool SleepTimer_takeExpired(SleepTimer* timer, uint32_t now_ms) {
    if (timer->minutes <= 0) return false;
    if (ms_until(timer->deadline_ms, now_ms) > 0) return false;
    timer->minutes     = 0;
    timer->deadline_ms = 0;
    return true;
}

uint32_t SleepTimer_remainingMs(const SleepTimer* timer, uint32_t now_ms) {
    if (timer->minutes <= 0) return 0;
    int32_t left = ms_until(timer->deadline_ms, now_ms);
    return left > 0 ? (uint32_t)left : 0;
}

int SleepTimer_stepMinutes(int minutes, int direction) {
    int index = 0;
    for (int step = 0; step < SLEEP_TIMER_STEP_COUNT; step++) {
        if (SLEEP_TIMER_STEPS[step] <= minutes) index = step;
    }
    index = (index + (direction >= 0 ? 1 : SLEEP_TIMER_STEP_COUNT - 1)) % SLEEP_TIMER_STEP_COUNT;
    return SLEEP_TIMER_STEPS[index];
}

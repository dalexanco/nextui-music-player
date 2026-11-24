// The countdown of the sleep timer, with the clock given by the test.

#include "sleep_timer.h"
#include "test.h"

TEST(a_new_timer_is_off) {
    SleepTimer timer = {0};
    CHECK(!SleepTimer_takeExpired(&timer, 1000000));
    CHECK(SleepTimer_remainingMs(&timer, 5) == 0);
}

TEST(a_timer_ends_after_its_length) {
    SleepTimer timer = {0};
    SleepTimer_set(&timer, 15, 1000);
    CHECK(SleepTimer_remainingMs(&timer, 1000) == 15u * 60000u);
    CHECK(!SleepTimer_takeExpired(&timer, 1000 + 15 * 60000 - 1));
    CHECK(SleepTimer_takeExpired(&timer, 1000 + 15 * 60000));
}

TEST(the_end_reports_once) {
    SleepTimer timer = {0};
    SleepTimer_set(&timer, 15, 0);
    CHECK(SleepTimer_takeExpired(&timer, 15 * 60000 + 5));
    CHECK(!SleepTimer_takeExpired(&timer, 15 * 60000 + 6));
    CHECK(timer.minutes == 0);
}

TEST(a_length_of_zero_turns_the_timer_off) {
    SleepTimer timer = {0};
    SleepTimer_set(&timer, 30, 0);
    SleepTimer_set(&timer, 0, 100);
    CHECK(!SleepTimer_takeExpired(&timer, 99999999));
    CHECK(SleepTimer_remainingMs(&timer, 100) == 0);
}

TEST(the_countdown_follows_the_wrap_of_the_clock) {
    SleepTimer timer = {0};
    uint32_t start   = UINT32_MAX - 60000u;
    SleepTimer_set(&timer, 15, start);
    CHECK(!SleepTimer_takeExpired(&timer, start + 14u * 60000u));
    CHECK(SleepTimer_remainingMs(&timer, start + 14u * 60000u) == 60000u);
    CHECK(SleepTimer_takeExpired(&timer, start + 15u * 60000u));
}

TEST(the_remaining_time_counts_down) {
    SleepTimer timer = {0};
    SleepTimer_set(&timer, 15, 0);
    CHECK(SleepTimer_remainingMs(&timer, 60000) == 14u * 60000u);
    CHECK(SleepTimer_remainingMs(&timer, 20 * 60000) == 0);
}

TEST(the_steps_cycle_forward_and_back) {
    CHECK(SleepTimer_stepMinutes(0, 1) == 15);
    CHECK(SleepTimer_stepMinutes(15, 1) == 30);
    CHECK(SleepTimer_stepMinutes(60, 1) == 0);
    CHECK(SleepTimer_stepMinutes(0, -1) == 60);
    CHECK(SleepTimer_stepMinutes(30, -1) == 15);
}

TEST(a_length_that_is_not_a_step_counts_as_the_step_below) {
    CHECK(SleepTimer_stepMinutes(20, 1) == 30);
    CHECK(SleepTimer_stepMinutes(20, -1) == 0);
}

int main(void) {
    RUN(a_new_timer_is_off);
    RUN(a_timer_ends_after_its_length);
    RUN(the_end_reports_once);
    RUN(a_length_of_zero_turns_the_timer_off);
    RUN(the_countdown_follows_the_wrap_of_the_clock);
    RUN(the_remaining_time_counts_down);
    RUN(the_steps_cycle_forward_and_back);
    RUN(a_length_that_is_not_a_step_counts_as_the_step_below);
    return test_summary();
}

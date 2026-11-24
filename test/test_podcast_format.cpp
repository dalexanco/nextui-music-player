// The texts of the podcast lists.

#include "podcast_format.hpp"
#include "test.h"

static const std::time_t kNow = 1700000000;
static const uint32_t kDay    = 24 * 3600;

TEST(a_date_reads_as_a_time_ago) {
    CHECK(formatRelativeDate(kNow, static_cast<uint32_t>(kNow - 100)) == "Today");
    CHECK(formatRelativeDate(kNow, static_cast<uint32_t>(kNow) - kDay) == "Yesterday");
    CHECK(formatRelativeDate(kNow, static_cast<uint32_t>(kNow) - 3 * kDay) == "3 days ago");
    CHECK(formatRelativeDate(kNow, static_cast<uint32_t>(kNow) - 7 * kDay) == "1 week ago");
    CHECK(formatRelativeDate(kNow, static_cast<uint32_t>(kNow) - 15 * kDay) == "2 weeks ago");
}

TEST(an_old_date_reads_as_a_month_and_a_day) {
    const std::string text = formatRelativeDate(kNow, static_cast<uint32_t>(kNow) - 90 * kDay);
    CHECK(text.size() == 6);
    CHECK(text.find("ago") == std::string::npos);
}

TEST(an_unknown_date_is_empty_and_a_future_one_is_today) {
    CHECK(formatRelativeDate(kNow, 0).empty());
    CHECK(formatRelativeDate(kNow, static_cast<uint32_t>(kNow) + 5 * kDay) == "Today");
}

TEST(the_episode_counts_say_the_new_ones_only_when_there_are) {
    CHECK(formatEpisodeCounts(0, 0) == "No episodes");
    CHECK(formatEpisodeCounts(1, 0) == "1 episode");
    CHECK(formatEpisodeCounts(42, 0) == "42 episodes");
    CHECK(formatEpisodeCounts(42, 3) == "42 episodes · 3 new");
}

TEST(the_speed_reads_with_the_digits_that_count) {
    CHECK(formatPlaybackSpeed(1.0f) == "1x");
    CHECK(formatPlaybackSpeed(1.25f) == "1.25x");
    CHECK(formatPlaybackSpeed(1.5f) == "1.5x");
    CHECK(formatPlaybackSpeed(0.5f) == "0.5x");
    CHECK(formatPlaybackSpeed(2.0f) == "2x");
}

TEST(the_speed_steps_by_a_quarter_between_half_and_double) {
    CHECK(stepPlaybackSpeed(1.0f, 1) == 1.25f);
    CHECK(stepPlaybackSpeed(1.0f, -1) == 0.75f);
    CHECK(stepPlaybackSpeed(2.0f, 1) == 2.0f);
    CHECK(stepPlaybackSpeed(0.5f, -1) == 0.5f);
    // A speed that the player set between two steps snaps to the grid
    CHECK(stepPlaybackSpeed(1.1f, 1) == 1.25f);
}

TEST(a_publication_date_names_the_day) {
    CHECK(formatPublishedDate(0).empty());
    const std::string text = formatPublishedDate(1700000000);
    CHECK(text.rfind("Published ", 0) == 0);
    CHECK(text.size() == 16);
}

int main() {
    RUN(the_speed_reads_with_the_digits_that_count);
    RUN(the_speed_steps_by_a_quarter_between_half_and_double);
    RUN(a_publication_date_names_the_day);
    RUN(a_date_reads_as_a_time_ago);
    RUN(an_old_date_reads_as_a_month_and_a_day);
    RUN(an_unknown_date_is_empty_and_a_future_one_is_today);
    RUN(the_episode_counts_say_the_new_ones_only_when_there_are);
    return test_summary();
}

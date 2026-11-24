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

int main() {
    RUN(a_date_reads_as_a_time_ago);
    RUN(an_old_date_reads_as_a_month_and_a_day);
    RUN(an_unknown_date_is_empty_and_a_future_one_is_today);
    RUN(the_episode_counts_say_the_new_ones_only_when_there_are);
    return test_summary();
}

// Host tests of the geometry and of the texts of the music player.

#include "test.h"
#include "ui/music_player_layout.hpp"

namespace {

constexpr int kW = 1024;
constexpr int kH = 768;

PlayerRects makeRects() { return layoutPlayer(kW, kH, 15, 22, 88, 44); }

}  // namespace

TEST(the_art_takes_the_right_of_the_screen_over_its_whole_height) {
    const PlayerRects rects = makeRects();
    CHECK_EQ_INT(rects.art.w, 620);
    CHECK_EQ_INT(rects.art.x + rects.art.w, kW);
    CHECK_EQ_INT(rects.art.y, 0);
    CHECK_EQ_INT(rects.art.h, kH);
}

TEST(the_text_column_and_the_progress_row_match_the_board) {
    const PlayerRects rects = makeRects();
    CHECK_EQ_INT(rects.textColumn.x, 37);
    CHECK_EQ_INT(rects.textColumn.y, 132);
    CHECK_EQ_INT(rects.textColumn.w, 560);
    CHECK_EQ_INT(rects.progressRow.y, 560);
    CHECK_EQ_INT(rects.progressRow.x, 37);
    CHECK_EQ_INT(rects.progressRow.w, kW - 2 * 37);
}

TEST(the_text_column_ends_where_the_progress_row_starts) {
    const PlayerRects rects = makeRects();
    CHECK_EQ_INT(rects.textColumn.y + rects.textColumn.h, rects.progressRow.y);
}

TEST(the_top_row_sits_in_the_top_pill_row) {
    const PlayerRects rects = makeRects();
    CHECK_EQ_INT(rects.topRow.y, 15);
    CHECK_EQ_INT(rects.topRow.h, 88);
    CHECK_EQ_INT(rects.topRow.x, 37);
}

TEST(the_lyrics_room_is_between_the_content_and_the_progress_row) {
    const PlayerRects rects = makeRects();
    const SDL_Rect room     = layoutLyricsRect(rects, 400, 10);
    CHECK_EQ_INT(room.y, 410);
    CHECK_EQ_INT(room.y + room.h, 550);
    CHECK_EQ_INT(room.x, rects.textColumn.x);
}

TEST(the_lyrics_room_is_empty_when_the_content_reaches_the_progress_row) {
    const PlayerRects rects = makeRects();
    CHECK_EQ_INT(layoutLyricsRect(rects, 700, 10).h, 0);
}

TEST(the_fade_starts_transparent_and_ends_opaque) {
    const int fade = getArtFadeWidth(620);
    CHECK_EQ_INT(getArtFadeAlpha(0, 620, fade), 0);
    CHECK_EQ_INT(getArtFadeAlpha(fade, 620, fade), 255);
    CHECK_EQ_INT(getArtFadeAlpha(619, 620, fade), 255);
}

TEST(the_fade_never_falls) {
    const int fade = getArtFadeWidth(620);
    int previous   = 0;
    for (int x = 0; x < 620; x++) {
        const int alpha = getArtFadeAlpha(x, 620, fade);
        CHECK(alpha >= previous);
        previous = alpha;
    }
}

TEST(the_fade_is_half_at_the_middle_of_its_width) {
    const int fade  = getArtFadeWidth(620);
    const int alpha = getArtFadeAlpha(fade / 2, 620, fade);
    CHECK(alpha >= 120 && alpha <= 135);
}

TEST(an_art_with_no_width_has_no_fade) {
    CHECK_EQ_INT(getArtFadeAlpha(0, 0, 0), 255);
}

TEST(times_show_minutes_and_seconds_then_hours) {
    CHECK(formatPlayerTime(0) == "0:00");
    CHECK(formatPlayerTime(8999) == "0:08");
    CHECK(formatPlayerTime(215000) == "3:35");
    CHECK(formatPlayerTime(3600000) == "1:00:00");
    CHECK(formatPlayerTime(11351000) == "3:09:11");
    CHECK(formatPlayerTime(-5) == "0:00");
}

TEST(the_remaining_time_counts_down_to_zero) {
    CHECK(formatRemainingTime(8000, 215000) == "-3:27");
    CHECK(formatRemainingTime(215000, 215000) == "-0:00");
    CHECK(formatRemainingTime(300000, 215000) == "-0:00");
}

TEST(the_counter_has_two_digits) {
    CHECK(formatTrackCounter(2, 15) == "02 / 15");
    CHECK(formatTrackCounter(120, 300) == "120 / 300");
}

int main() {
    RUN(the_art_takes_the_right_of_the_screen_over_its_whole_height);
    RUN(the_text_column_and_the_progress_row_match_the_board);
    RUN(the_text_column_ends_where_the_progress_row_starts);
    RUN(the_top_row_sits_in_the_top_pill_row);
    RUN(the_lyrics_room_is_between_the_content_and_the_progress_row);
    RUN(the_lyrics_room_is_empty_when_the_content_reaches_the_progress_row);
    RUN(the_fade_starts_transparent_and_ends_opaque);
    RUN(the_fade_never_falls);
    RUN(the_fade_is_half_at_the_middle_of_its_width);
    RUN(an_art_with_no_width_has_no_fade);
    RUN(times_show_minutes_and_seconds_then_hours);
    RUN(the_remaining_time_counts_down_to_zero);
    RUN(the_counter_has_two_digits);
    return test_summary();
}

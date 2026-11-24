// What the radio player says of the stream.

#include "radio_player_state.hpp"
#include "test.h"

static RadioView view(RadioState state, float buffer, bool waiting = false, bool healthy = false) {
    return getRadioView(RadioViewInput{state, waiting, buffer, healthy});
}

TEST(a_station_that_waits_for_its_screen_connects) {
    CHECK(view(RADIO_STATE_STOPPED, 0.0f, true) == RadioView::Connecting);
    // The stream of the station that went is still there
    CHECK(view(RADIO_STATE_PLAYING, 1.0f, true) == RadioView::Connecting);
}

TEST(the_radio_states_map_to_a_look) {
    CHECK(view(RADIO_STATE_CONNECTING, 0.0f) == RadioView::Connecting);
    CHECK(view(RADIO_STATE_PLAYING, 0.9f) == RadioView::Live);
    CHECK(view(RADIO_STATE_ERROR, 0.0f) == RadioView::Offline);
    CHECK(view(RADIO_STATE_STOPPED, 0.0f) == RadioView::Paused);
}

TEST(a_first_buffer_connects_and_a_healthy_one_is_live) {
    CHECK(view(RADIO_STATE_BUFFERING, 0.2f) == RadioView::Connecting);
    CHECK(view(RADIO_STATE_BUFFERING, 0.5f) == RadioView::Live);
}

TEST(a_stream_that_played_and_runs_low_reconnects) {
    CHECK(view(RADIO_STATE_BUFFERING, 0.2f, false, true) == RadioView::Rebuffering);
    CHECK(view(RADIO_STATE_BUFFERING, 0.8f, false, true) == RadioView::Live);
}

TEST(the_healthy_stream_follows_the_buffer) {
    CHECK(isRadioStreamHealthy(RADIO_STATE_PLAYING, 0.0f));
    CHECK(isRadioStreamHealthy(RADIO_STATE_BUFFERING, 0.6f));
    CHECK(!isRadioStreamHealthy(RADIO_STATE_BUFFERING, 0.1f));
    CHECK(!isRadioStreamHealthy(RADIO_STATE_ERROR, 1.0f));
}

TEST(each_look_has_its_chip_and_its_verb) {
    CHECK(std::string(getRadioChipText(RadioView::Live)) == "LIVE");
    CHECK(std::string(getRadioChipText(RadioView::Offline)) == "OFFLINE");
    CHECK(std::string(getRadioActionLabel(RadioView::Live)) == "STOP");
    CHECK(std::string(getRadioActionLabel(RadioView::Connecting)) == "CANCEL");
    CHECK(std::string(getRadioActionLabel(RadioView::Paused)) == "LISTEN");
    CHECK(std::string(getRadioActionLabel(RadioView::Offline)) == "RETRY");
}

TEST(the_status_line_shows_the_error_of_the_stream) {
    CHECK(getRadioStatusLine(RadioView::Live, nullptr).empty());
    CHECK(getRadioStatusLine(RadioView::Offline, "HTTP 404") == "HTTP 404");
    CHECK(!getRadioStatusLine(RadioView::Offline, "").empty());
    CHECK(!getRadioStatusLine(RadioView::Connecting, nullptr).empty());
}

TEST(the_buffer_is_a_percent) {
    CHECK_EQ_INT(getRadioBufferPercent(0.0f), 0);
    CHECK_EQ_INT(getRadioBufferPercent(0.4f), 40);
    CHECK_EQ_INT(getRadioBufferPercent(1.7f), 100);
    CHECK_EQ_INT(getRadioBufferPercent(-1.0f), 0);
}

TEST(the_listening_time_counts_minutes) {
    CHECK(formatListening(59999) == "Just started");
    CHECK(formatListening(12 * 60000) == "Listening for 12 min");
    CHECK(formatListening(65 * 60000) == "Listening for 1 h 05");
}

int main() {
    RUN(a_station_that_waits_for_its_screen_connects);
    RUN(the_radio_states_map_to_a_look);
    RUN(a_first_buffer_connects_and_a_healthy_one_is_live);
    RUN(a_stream_that_played_and_runs_low_reconnects);
    RUN(the_healthy_stream_follows_the_buffer);
    RUN(each_look_has_its_chip_and_its_verb);
    RUN(the_status_line_shows_the_error_of_the_stream);
    RUN(the_buffer_is_a_percent);
    RUN(the_listening_time_counts_minutes);
    return test_summary();
}

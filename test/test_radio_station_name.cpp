// The name that the user types for a station.

#include "radio_station_name.hpp"
#include "test.h"

TEST(a_plain_name_stays) { CHECK(cleanStationName("FIP", 127) == "FIP"); }

TEST(the_ends_lose_their_spaces) { CHECK(cleanStationName("  Radio Nova  ", 127) == "Radio Nova"); }

TEST(nothing_but_spaces_is_empty) {
    CHECK(cleanStationName("", 127).empty());
    CHECK(cleanStationName("   ", 127).empty());
    CHECK(cleanStationName("|\n|", 127).empty());
}

TEST(the_separator_of_the_station_file_becomes_a_space) {
    CHECK(cleanStationName("A|B", 127) == "A B");
    CHECK(cleanStationName("A\nB", 127) == "A B");
}

TEST(a_long_name_is_cut_to_the_limit) {
    CHECK(cleanStationName("abcdefghij", 5) == "abcde");
    CHECK(cleanStationName("abc defghij", 4) == "abc");
}

TEST(the_cut_does_not_split_a_character) {
    // "é" takes two bytes
    CHECK(cleanStationName("aé", 2) == "a");
    CHECK(cleanStationName("aéb", 3) == "aé");
}

int main() {
    RUN(a_plain_name_stays);
    RUN(the_ends_lose_their_spaces);
    RUN(nothing_but_spaces_is_empty);
    RUN(the_separator_of_the_station_file_becomes_a_space);
    RUN(a_long_name_is_cut_to_the_limit);
    RUN(the_cut_does_not_split_a_character);
    return test_summary();
}

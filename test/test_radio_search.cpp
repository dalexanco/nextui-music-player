// The search of the directory of stations.

#include "radio_search.hpp"
#include "test.h"

static std::vector<SearchEntry> directory() {
    return {
        {"TSF Jazz", "Jazz", "Le jazz sans frontiere", "France", "FR"},
        {"Radio Swiss Jazz", "Jazz", "", "Switzerland", "CH"},
        {"FIP", "Eclectic", "Radio France", "France", "FR"},
        {"abc Classic", "Classical", "", "Australia", "AU"},
    };
}

TEST(an_empty_query_keeps_every_station_sorted_by_name) {
    const auto found = searchStations(directory(), "", "");
    CHECK(found.size() == 4);
    CHECK(found[0] == 3);  // abc Classic
    CHECK(found[1] == 2);  // FIP
    CHECK(found[2] == 1);  // Radio Swiss Jazz
    CHECK(found[3] == 0);  // TSF Jazz
}

TEST(the_case_of_the_letters_does_not_count) {
    CHECK(searchStations(directory(), "JAZZ", "").size() == 2);
    CHECK(searchStations(directory(), "tsf", "").size() == 1);
}

TEST(each_word_must_be_found) {
    CHECK(searchStations(directory(), "jazz swiss", "").size() == 1);
    CHECK(searchStations(directory(), "jazz  nothing", "").empty());
}

TEST(the_slogan_and_the_country_answer_too) {
    CHECK(searchStations(directory(), "frontiere", "").size() == 1);
    CHECK(searchStations(directory(), "australia", "").size() == 1);
}

TEST(a_country_keeps_only_its_stations) {
    const auto found = searchStations(directory(), "", "FR");
    CHECK(found.size() == 2);
    CHECK(searchStations(directory(), "jazz", "FR").size() == 1);
    CHECK(searchStations(directory(), "classic", "FR").empty());
}

TEST(spaces_alone_are_an_empty_query) { CHECK(searchStations(directory(), "   ", "").size() == 4); }

int main() {
    RUN(an_empty_query_keeps_every_station_sorted_by_name);
    RUN(the_case_of_the_letters_does_not_count);
    RUN(each_word_must_be_found);
    RUN(the_slogan_and_the_country_answer_too);
    RUN(a_country_keeps_only_its_stations);
    RUN(spaces_alone_are_an_empty_query);
    return test_summary();
}

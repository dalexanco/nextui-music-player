// The set of favourite stations, in memory and through a file.

#include <string.h>

#include "radio_favourites.h"
#include "test.h"

static RadioFavourites set;

TEST(a_new_set_has_no_favourite) {
    memset(&set, 0, sizeof(set));
    CHECK(!RadioFavourites_has(&set, "http://a"));
    CHECK(!RadioFavourites_has(&set, NULL));
    CHECK(!RadioFavourites_has(&set, ""));
}

TEST(a_mark_is_set_and_cleared) {
    memset(&set, 0, sizeof(set));
    CHECK(RadioFavourites_set(&set, "http://a", true));
    CHECK(RadioFavourites_set(&set, "http://b", true));
    CHECK(RadioFavourites_has(&set, "http://a"));
    CHECK(RadioFavourites_set(&set, "http://a", false));
    CHECK(!RadioFavourites_has(&set, "http://a"));
    CHECK(RadioFavourites_has(&set, "http://b"));
    CHECK(set.count == 1);
}

TEST(the_same_mark_twice_counts_once) {
    memset(&set, 0, sizeof(set));
    RadioFavourites_set(&set, "http://a", true);
    RadioFavourites_set(&set, "http://a", true);
    CHECK(set.count == 1);
    CHECK(RadioFavourites_set(&set, "http://never", false));
}

TEST(toggle_returns_the_new_state) {
    memset(&set, 0, sizeof(set));
    CHECK(RadioFavourites_toggle(&set, "http://a"));
    CHECK(!RadioFavourites_toggle(&set, "http://a"));
    CHECK(set.count == 0);
}

TEST(a_full_set_refuses_one_more) {
    memset(&set, 0, sizeof(set));
    char url[32];
    for (int i = 0; i < RADIO_MAX_STATIONS; i++) {
        snprintf(url, sizeof(url), "http://s%d", i);
        CHECK(RadioFavourites_set(&set, url, true));
    }
    CHECK(!RadioFavourites_set(&set, "http://extra", true));
    CHECK(!RadioFavourites_toggle(&set, "http://extra"));
    CHECK(!RadioFavourites_has(&set, "http://extra"));
}

TEST(the_set_survives_a_file) {
    memset(&set, 0, sizeof(set));
    RadioFavourites_set(&set, "http://a/x?y=1", true);
    RadioFavourites_set(&set, "http://b", true);
    FILE* file = tmpfile();
    CHECK(file != NULL);
    RadioFavourites_write(&set, file);
    rewind(file);
    static RadioFavourites back;
    memset(&back, 0, sizeof(back));
    CHECK(RadioFavourites_read(&back, file) == 2);
    CHECK(RadioFavourites_has(&back, "http://a/x?y=1"));
    CHECK(RadioFavourites_has(&back, "http://b"));
    fclose(file);
}

int main(void) {
    RUN(a_new_set_has_no_favourite);
    RUN(a_mark_is_set_and_cleared);
    RUN(the_same_mark_twice_counts_once);
    RUN(toggle_returns_the_new_state);
    RUN(a_full_set_refuses_one_more);
    RUN(the_set_survives_a_file);
    return test_summary();
}

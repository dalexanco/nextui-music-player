// The history of the file browser: where the cursor goes after a way up.

#include <vector>

#include "browser_nav.hpp"
#include "test.h"

TEST(a_way_up_puts_the_cursor_back_on_the_row_that_it_left) {
    BrowserNav nav;
    nav.enter(1, 7);
    CHECK(nav.leave(1) == 7);
}

TEST(the_history_is_a_stack) {
    BrowserNav nav;
    nav.enter(1, 3);
    nav.enter(2, 9);
    CHECK(nav.leave(2) == 9);
    CHECK(nav.leave(1) == 3);
    CHECK(nav.leave(1) == -1);
}

TEST(a_history_of_another_folder_is_stale_and_dropped) {
    BrowserNav nav;
    nav.enter(1, 3);
    nav.enter(2, 9);
    // The browser went up to a folder that the history does not expect
    CHECK(nav.leave(5) == -1);
    // The whole history went, thus the old entries do not answer later
    CHECK(nav.leave(2) == -1);
    CHECK(nav.leave(1) == -1);
}

TEST(clear_forgets_the_history) {
    BrowserNav nav;
    nav.enter(1, 3);
    nav.clear();
    CHECK(nav.leave(1) == -1);
}

TEST(a_history_that_is_full_does_not_record_more_and_keeps_the_old_entries) {
    BrowserNav nav;
    for (int dir = 0; dir < BrowserNav::kDepth; dir++) nav.enter(dir, dir * 2);
    nav.enter(999, 1);
    // The extra entry is not there, thus the top is the last recorded one
    CHECK(nav.leave(BrowserNav::kDepth - 1) == (BrowserNav::kDepth - 1) * 2);
}

TEST(without_a_remembered_row_the_cursor_goes_to_the_folder_that_was_left) {
    const std::vector<BrowserNavRow> rows = {{true, 1}, {true, 10}, {true, 11}, {false, 11}};
    CHECK(BrowserNav::chooseSelection(-1, 11, rows) == 2);
}

TEST(a_file_with_the_same_id_as_the_folder_is_not_the_folder) {
    const std::vector<BrowserNavRow> rows = {{false, 11}, {true, 11}};
    CHECK(BrowserNav::chooseSelection(-1, 11, rows) == 1);
}

TEST(a_folder_that_is_not_there_puts_the_cursor_on_the_first_row) {
    const std::vector<BrowserNavRow> rows = {{true, 1}, {true, 2}};
    CHECK(BrowserNav::chooseSelection(-1, 42, rows) == 0);
}

TEST(a_remembered_row_wins_over_the_folder) {
    const std::vector<BrowserNavRow> rows = {{true, 1}, {true, 2}, {true, 3}};
    CHECK(BrowserNav::chooseSelection(0, 3, rows) == 0);
}

TEST(a_remembered_row_past_the_end_goes_to_the_last_row) {
    const std::vector<BrowserNavRow> rows = {{true, 1}, {true, 2}};
    CHECK(BrowserNav::chooseSelection(9, 1, rows) == 1);
}

TEST(an_empty_list_has_the_row_zero) {
    CHECK(BrowserNav::chooseSelection(-1, 1, {}) == 0);
    CHECK(BrowserNav::chooseSelection(4, 1, {}) == 0);
}

int main() {
    RUN(a_way_up_puts_the_cursor_back_on_the_row_that_it_left);
    RUN(the_history_is_a_stack);
    RUN(a_history_of_another_folder_is_stale_and_dropped);
    RUN(clear_forgets_the_history);
    RUN(a_history_that_is_full_does_not_record_more_and_keeps_the_old_entries);
    RUN(without_a_remembered_row_the_cursor_goes_to_the_folder_that_was_left);
    RUN(a_file_with_the_same_id_as_the_folder_is_not_the_folder);
    RUN(a_folder_that_is_not_there_puts_the_cursor_on_the_first_row);
    RUN(a_remembered_row_wins_over_the_folder);
    RUN(a_remembered_row_past_the_end_goes_to_the_last_row);
    RUN(an_empty_list_has_the_row_zero);
    return test_summary();
}

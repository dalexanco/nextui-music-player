// Host tests of the geometry of the options panel.

#include "test.h"
#include "ui/view/options_panel_layout.hpp"

namespace {
constexpr int kW = 1024;
constexpr int kH = 768;
// The top of the row of button hints: 768 - 15 - 88
constexpr int kBottomLimit = 665;
}  // namespace

TEST(the_panel_is_a_bit_more_than_half_the_width_and_ends_one_margin_from_the_right) {
    const OptionsPanelRects rects = layoutOptionsPanel(kW, kH, 15, 60, 84, 5, kBottomLimit);
    CHECK_EQ_INT(rects.panel.w, 600);
    CHECK_EQ_INT(rects.panel.x + rects.panel.w, kW - 15);
}

TEST(the_panel_starts_at_120_of_768) {
    const OptionsPanelRects rects = layoutOptionsPanel(kW, kH, 15, 60, 84, 5, kBottomLimit);
    CHECK_EQ_INT(rects.panel.y, 120);
}

TEST(the_panel_is_as_high_as_its_rows_need) {
    const OptionsPanelRects rects = layoutOptionsPanel(kW, kH, 15, 60, 84, 3, kBottomLimit);
    CHECK_EQ_INT(rects.panel.h, 15 + 60 + 3 * 84 + 15);
}

TEST(the_panel_never_goes_under_the_button_hints) {
    const OptionsPanelRects rects = layoutOptionsPanel(kW, kH, 15, 60, 84, 12, kBottomLimit);
    CHECK(rects.panel.y + rects.panel.h <= kBottomLimit);
}

TEST(the_rows_sit_between_the_header_and_the_foot_of_the_panel) {
    const OptionsPanelRects rects = layoutOptionsPanel(kW, kH, 15, 60, 84, 5, kBottomLimit);
    CHECK_EQ_INT(rects.rows.y, rects.header.y + rects.header.h);
    CHECK(rects.rows.y + rects.rows.h <= rects.panel.y + rects.panel.h);
    CHECK(rects.rows.x > rects.panel.x);
    CHECK(rects.rows.x + rects.rows.w < rects.panel.x + rects.panel.w);
}

int main() {
    RUN(the_panel_is_a_bit_more_than_half_the_width_and_ends_one_margin_from_the_right);
    RUN(the_panel_starts_at_120_of_768);
    RUN(the_panel_is_as_high_as_its_rows_need);
    RUN(the_panel_never_goes_under_the_button_hints);
    RUN(the_rows_sit_between_the_header_and_the_foot_of_the_panel);
    return test_summary();
}

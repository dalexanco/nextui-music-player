// Host tests of the geometry of the detail panel and of the side panel.

#include "test.h"
#include "ui/view/detail_panel_layout.hpp"
#include "ui/view/source_gradient.hpp"

namespace {

constexpr int kScreenWidth  = 1024;
constexpr int kPadding      = 15;

// The body of a page between the top pill row and the button hints, with the side margins
const SDL_Rect kBody = {kPadding, 103, kScreenWidth - 2 * kPadding, 562};

}  // namespace

TEST(panel_is_a_third_of_the_screen_and_ends_at_the_right_edge_of_the_body) {
    const SidePanelRects rects = layoutSidePanel(kBody, kScreenWidth, kPadding);
    CHECK_EQ_INT(rects.panel.w, kScreenWidth / 3);
    CHECK_EQ_INT(rects.panel.x + rects.panel.w, kBody.x + kBody.w);
}

TEST(list_takes_the_room_left_of_the_panel_with_a_gap) {
    const SidePanelRects rects = layoutSidePanel(kBody, kScreenWidth, kPadding);
    CHECK_EQ_INT(rects.list.x, kBody.x);
    CHECK_EQ_INT(rects.list.x + rects.list.w + kPadding, rects.panel.x);
    CHECK_EQ_INT(rects.list.y, kBody.y);
    CHECK_EQ_INT(rects.list.h, kBody.h);
}

TEST(panel_starts_one_margin_under_the_top_of_the_body) {
    const SidePanelRects rects = layoutSidePanel(kBody, kScreenWidth, kPadding);
    CHECK_EQ_INT(rects.panel.y, kBody.y + kPadding);
    CHECK_EQ_INT(rects.panel.h, kBody.h - kPadding);
}

TEST(narrow_body_gives_the_panel_all_and_the_list_nothing) {
    const SDL_Rect narrow      = {0, 0, 200, 100};
    const SidePanelRects rects = layoutSidePanel(narrow, kScreenWidth, kPadding);
    CHECK_EQ_INT(rects.panel.w, 200);
    CHECK_EQ_INT(rects.list.w, 0);
}

TEST(art_is_a_square_of_the_panel_width_and_the_info_follows_it) {
    const DetailPanelRects rects = layoutDetailPanel(341, 547, 120, 15);
    CHECK_EQ_INT(rects.art.x, 0);
    CHECK_EQ_INT(rects.art.y, 0);
    CHECK_EQ_INT(rects.art.w, 341);
    CHECK_EQ_INT(rects.art.h, 341);
    CHECK_EQ_INT(rects.info.y, 341 + 15);
    CHECK_EQ_INT(rects.info.w, 341);
    CHECK_EQ_INT(rects.info.h, 120);
}

TEST(art_shrinks_and_stays_centered_where_the_info_does_not_fit) {
    const DetailPanelRects rects = layoutDetailPanel(341, 300, 120, 15);
    CHECK_EQ_INT(rects.art.w, 165);
    CHECK_EQ_INT(rects.art.h, 165);
    CHECK_EQ_INT(rects.art.x, (341 - 165) / 2);
    CHECK_EQ_INT(rects.info.y + rects.info.h, 300);
}

TEST(art_has_no_negative_size_in_a_tiny_panel) {
    const DetailPanelRects rects = layoutDetailPanel(341, 50, 120, 15);
    CHECK_EQ_INT(rects.art.w, 0);
    CHECK_EQ_INT(rects.art.h, 0);
}

TEST(info_rows_stack_in_order_with_the_gap) {
    const InfoBlockRects rects = layoutInfoBlock(341, 42, 9, 36, 15, 120);
    CHECK_EQ_INT(rects.line.y, 0);
    CHECK_EQ_INT(rects.line.h, 42);
    CHECK_EQ_INT(rects.bar.y, 42 + 15);
    CHECK_EQ_INT(rects.bar.h, 9);
    CHECK_EQ_INT(rects.position.y, 42 + 15 + 9 + 15);
    CHECK_EQ_INT(rects.times.y, rects.position.y);
    CHECK_EQ_INT(rects.height, 42 + 15 + 9 + 15 + 36);
    CHECK_EQ_INT(rects.line.w, 341);
    CHECK_EQ_INT(rects.bar.w, 341);
}

TEST(times_sit_at_the_right_and_the_position_takes_the_rest) {
    const InfoBlockRects rects = layoutInfoBlock(341, 42, 9, 36, 15, 120);
    CHECK_EQ_INT(rects.times.x + rects.times.w, 341);
    CHECK_EQ_INT(rects.times.w, 120);
    CHECK_EQ_INT(rects.position.x, 0);
    CHECK_EQ_INT(rects.position.x + rects.position.w + 15, rects.times.x);
}

TEST(times_wider_than_the_block_are_cut_to_it) {
    const InfoBlockRects rects = layoutInfoBlock(100, 42, 9, 36, 15, 500);
    CHECK_EQ_INT(rects.times.w, 100);
    CHECK_EQ_INT(rects.times.x, 0);
    CHECK_EQ_INT(rects.position.w, 0);
}

TEST(tile_has_transparent_corners_and_an_opaque_middle) {
    constexpr int kSide = 64;
    uint32_t pixels[kSide * kSide];
    const SDL_Color from = {200, 100, 150, 255};
    const SDL_Color to   = {90, 16, 49, 255};
    renderArtTile(pixels, kSide, kSide, kSide, from, to, 12);
    CHECK_EQ_INT(pixels[0] >> 24, 0);
    CHECK_EQ_INT(pixels[kSide - 1] >> 24, 0);
    CHECK_EQ_INT(pixels[(kSide - 1) * kSide] >> 24, 0);
    CHECK_EQ_INT(pixels[kSide * kSide - 1] >> 24, 0);
    CHECK_EQ_INT(pixels[(kSide / 2) * kSide + kSide / 2] >> 24, 255);
    // The edge between the corners is opaque
    CHECK_EQ_INT(pixels[kSide / 2] >> 24, 255);
}

TEST(tile_goes_from_the_first_color_to_the_second) {
    constexpr int kSide = 64;
    uint32_t pixels[kSide * kSide];
    const SDL_Color from = {200, 100, 150, 255};
    const SDL_Color to   = {90, 16, 49, 255};
    renderArtTile(pixels, kSide, kSide, kSide, from, to, 4);
    const uint32_t nearStart = pixels[6 * kSide + 6];
    const uint32_t nearEnd   = pixels[(kSide - 7) * kSide + (kSide - 7)];
    CHECK(((nearStart >> 16) & 0xff) > ((nearEnd >> 16) & 0xff));
    CHECK(((nearStart >> 16) & 0xff) > 150);
    CHECK(((nearEnd >> 16) & 0xff) < 130);
}

int main() {
    RUN(panel_is_a_third_of_the_screen_and_ends_at_the_right_edge_of_the_body);
    RUN(list_takes_the_room_left_of_the_panel_with_a_gap);
    RUN(panel_starts_one_margin_under_the_top_of_the_body);
    RUN(narrow_body_gives_the_panel_all_and_the_list_nothing);
    RUN(art_is_a_square_of_the_panel_width_and_the_info_follows_it);
    RUN(art_shrinks_and_stays_centered_where_the_info_does_not_fit);
    RUN(art_has_no_negative_size_in_a_tiny_panel);
    RUN(info_rows_stack_in_order_with_the_gap);
    RUN(times_sit_at_the_right_and_the_position_takes_the_rest);
    RUN(times_wider_than_the_block_are_cut_to_it);
    RUN(tile_has_transparent_corners_and_an_opaque_middle);
    RUN(tile_goes_from_the_first_color_to_the_second);
    return test_summary();
}

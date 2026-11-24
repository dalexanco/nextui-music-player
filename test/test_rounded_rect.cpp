// Host tests of the rounded rectangle that dialogs, the options panel and chips are made of.

#include <vector>

#include "test.h"
#include "ui/view/rounded_rect.hpp"

namespace {

constexpr int kW = 40;
constexpr int kH = 24;

const SDL_Color kWhite = {255, 255, 255, 255};
const SDL_Color kDark  = {24, 20, 26, 255};
const SDL_Color kNone  = {0, 0, 0, 0};

std::vector<uint32_t> render(const RoundedRectStyle& style, int pitch = kW) {
    std::vector<uint32_t> pixels(static_cast<size_t>(pitch) * kH, 0x01010101u);
    renderRoundedRect(pixels.data(), kW, kH, pitch, style);
    return pixels;
}

int alpha(uint32_t pixel) { return static_cast<int>(pixel >> 24); }
int red(uint32_t pixel) { return static_cast<int>((pixel >> 16) & 0xff); }

}  // namespace

TEST(corner_pixel_is_transparent_and_the_middle_is_the_fill) {
    const auto pixels = render({kDark, kWhite, 0, 8});
    CHECK_EQ_INT(alpha(pixels[0]), 0);
    CHECK_EQ_INT(alpha(pixels[kW - 1]), 0);
    CHECK_EQ_INT(alpha(pixels[static_cast<size_t>(kH - 1) * kW]), 0);
    const uint32_t middle = pixels[static_cast<size_t>(kH / 2) * kW + kW / 2];
    CHECK_EQ_INT(alpha(middle), 255);
    CHECK_EQ_INT(red(middle), 24);
}

TEST(straight_sides_are_opaque_up_to_the_last_pixel) {
    const auto pixels = render({kDark, kWhite, 0, 8});
    CHECK_EQ_INT(alpha(pixels[kW / 2]), 255);
    CHECK_EQ_INT(alpha(pixels[static_cast<size_t>(kH / 2) * kW]), 255);
}

TEST(corner_is_anti_aliased) {
    const auto pixels = render({kDark, kWhite, 0, 8});
    bool partial = false;
    for (int y = 0; y < 8; y++) {
        for (int x = 0; x < 8; x++) {
            const int a = alpha(pixels[static_cast<size_t>(y) * kW + x]);
            if (a > 0 && a < 255) partial = true;
        }
    }
    CHECK(partial);
}

TEST(edge_draws_a_ring_and_keeps_the_inside_to_the_fill) {
    const auto pixels = render({kDark, kWhite, 2, 8});
    // The middle of the top side is the edge color, and the pixel inside the ring is the fill
    CHECK_EQ_INT(red(pixels[kW / 2]), 255);
    CHECK_EQ_INT(red(pixels[static_cast<size_t>(1) * kW + kW / 2]), 255);
    CHECK_EQ_INT(red(pixels[static_cast<size_t>(4) * kW + kW / 2]), 24);
}

TEST(outline_with_a_transparent_fill_leaves_the_inside_open) {
    const auto pixels = render({kNone, kWhite, 2, 6});
    CHECK_EQ_INT(alpha(pixels[kW / 2]), 255);
    CHECK_EQ_INT(alpha(pixels[static_cast<size_t>(kH / 2) * kW + kW / 2]), 0);
}

TEST(translucent_fill_is_premultiplied) {
    const auto pixels = render({SDL_Color{255, 255, 255, 51}, kNone, 0, 6});
    const uint32_t middle = pixels[static_cast<size_t>(kH / 2) * kW + kW / 2];
    CHECK_EQ_INT(alpha(middle), 51);
    CHECK_EQ_INT(red(middle), 51);
}

TEST(padding_of_the_rows_is_not_touched) {
    const auto pixels = render({kDark, kWhite, 0, 8}, kW + 5);
    CHECK_EQ_INT(pixels[kW], 0x01010101u);
}

TEST(radius_is_clamped_to_half_the_smaller_side) {
    const auto pixels = render({kDark, kWhite, 0, 500});
    // A pill: the middle of the left side is opaque, the corner is not
    CHECK_EQ_INT(alpha(pixels[static_cast<size_t>(kH / 2) * kW + 1]), 255);
    CHECK_EQ_INT(alpha(pixels[0]), 0);
}

int main() {
    RUN(corner_pixel_is_transparent_and_the_middle_is_the_fill);
    RUN(straight_sides_are_opaque_up_to_the_last_pixel);
    RUN(corner_is_anti_aliased);
    RUN(edge_draws_a_ring_and_keeps_the_inside_to_the_fill);
    RUN(outline_with_a_transparent_fill_leaves_the_inside_open);
    RUN(translucent_fill_is_premultiplied);
    RUN(padding_of_the_rows_is_not_touched);
    RUN(radius_is_clamped_to_half_the_smaller_side);
    return test_summary();
}

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

TEST(rounding_a_bitmap_fades_the_corners_and_keeps_the_rest) {
    const int w = 40, h = 30, pitch = 44;
    std::vector<uint32_t> pixels(static_cast<size_t>(pitch) * h, 0xff204060);
    for (int y = 0; y < h; y++) pixels[static_cast<size_t>(y) * pitch + w] = 0x12345678;  // padding
    applyRoundedCorners(pixels.data(), w, h, pitch, 8);
    // The corner pixel is fully transparent, with its color channels premultiplied to 0
    CHECK_EQ_INT(pixels[0], 0);
    CHECK_EQ_INT(pixels[static_cast<size_t>(w - 1)], 0);
    CHECK_EQ_INT(pixels[static_cast<size_t>(h - 1) * pitch], 0);
    CHECK_EQ_INT(pixels[static_cast<size_t>(h - 1) * pitch + w - 1], 0);
    // The four corners are the same by symmetry
    // A pixel on the arc is partly transparent, and its colors fade with its alpha
    bool partial = false;
    for (int x = 0; x < 8; x++) {
        const uint32_t pixel = pixels[static_cast<size_t>(3) * pitch + x];
        const uint32_t alpha = pixel >> 24;
        if (alpha > 0 && alpha < 0xff) {
            partial = true;
            CHECK(((pixel >> 16) & 0xff) < 0x20);
            CHECK(((pixel >> 16) & 0xff) > 0);
        }
    }
    CHECK(partial);
    // The middle, the straight sides and the padding are untouched
    CHECK_EQ_INT(pixels[static_cast<size_t>(15) * pitch + 20], 0xff204060);
    CHECK_EQ_INT(pixels[static_cast<size_t>(15) * pitch], 0xff204060);
    CHECK_EQ_INT(pixels[static_cast<size_t>(0) * pitch + 20], 0xff204060);
    CHECK_EQ_INT(pixels[static_cast<size_t>(10) * pitch + w], 0x12345678);
}

TEST(rounding_with_no_radius_changes_nothing) {
    std::vector<uint32_t> pixels(20 * 20, 0xff102030);
    applyRoundedCorners(pixels.data(), 20, 20, 20, 0);
    for (uint32_t pixel : pixels) CHECK_EQ_INT(pixel, 0xff102030);
}

TEST(rounding_matches_the_corner_of_the_rounded_rect) {
    // The same radius gives the same alpha as the rounded rectangle, so an image and a tile agree
    const int s = 60;
    std::vector<uint32_t> image(static_cast<size_t>(s) * s, 0xffffffff);
    applyRoundedCorners(image.data(), s, s, s, 10);
    std::vector<uint32_t> rect(static_cast<size_t>(s) * s);
    renderRoundedRect(rect.data(), s, s, s, RoundedRectStyle{SDL_Color{255, 255, 255, 255}, SDL_Color{0, 0, 0, 0}, 0, 10});
    for (int i = 0; i < s * s; i++) {
        const int a = static_cast<int>(image[static_cast<size_t>(i)] >> 24);
        const int b = static_cast<int>(rect[static_cast<size_t>(i)] >> 24);
        CHECK(std::abs(a - b) <= 1);
    }
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
    RUN(rounding_a_bitmap_fades_the_corners_and_keeps_the_rest);
    RUN(rounding_with_no_radius_changes_nothing);
    RUN(rounding_matches_the_corner_of_the_rounded_rect);
    return test_summary();
}

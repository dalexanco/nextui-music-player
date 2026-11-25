// Host tests of the soft background of a page: the colors at the corners and the middle, and the
// render of a whole image.

#include <vector>

#include "test.h"
#include "ui/view/source_gradient.hpp"

namespace {

constexpr int kWidth  = 1024;
constexpr int kHeight = 768;

// The palette of the music source
const SourcePalette kMusic = {SDL_Color{0xa3, 0x24, 0x5c, 0xff}, SDL_Color{0x3b, 0x0c, 0x22, 0xff}};

int red(uint32_t pixel) { return static_cast<int>((pixel >> 16) & 0xff); }
int green(uint32_t pixel) { return static_cast<int>((pixel >> 8) & 0xff); }
int blue(uint32_t pixel) { return static_cast<int>(pixel & 0xff); }
int luma(uint32_t pixel) { return red(pixel) + green(pixel) + blue(pixel); }

}  // namespace

TEST(each_pixel_is_opaque) {
    for (int x = 0; x < kWidth; x += 97) {
        for (int y = 0; y < kHeight; y += 89) {
            CHECK_EQ_INT(getSourceGradientPixel(x, y, kWidth, kHeight, kMusic) >> 24, 0xff);
        }
    }
}

TEST(top_right_corner_is_the_accent_halo_over_the_deep_tone) {
    const uint32_t pixel = getSourceGradientPixel(kWidth - 1, 0, kWidth, kHeight, kMusic);
    // About 0.55 of the accent over a base that is near the deep tone
    CHECK(red(pixel) > 0x3b + 40);
    CHECK(red(pixel) < 0xa3);
    CHECK(red(pixel) > green(pixel));
}

TEST(middle_left_is_the_plain_gradient) {
    // Out of reach of both halos: (0, 384) is 0.9 of the width from the top center
    // and 0.55 of the height from the bottom center
    const uint32_t pixel = getSourceGradientPixel(0, 384, kWidth, kHeight, kMusic);
    // On the line between the deep tone and the near black, thus darker than the deep tone
    CHECK(luma(pixel) < 0x3b + 0x0c + 0x22);
    CHECK(luma(pixel) > 0x0a + 0x08 + 0x0b);
}

TEST(bottom_right_corner_is_close_to_the_near_black) {
    const uint32_t pixel = getSourceGradientPixel(kWidth - 1, kHeight - 1, kWidth, kHeight, kMusic);
    CHECK(red(pixel) < 0x16);
    CHECK(green(pixel) < 0x10);
    CHECK(blue(pixel) < 0x16);
}

TEST(top_left_corner_keeps_the_deep_tone) {
    const uint32_t pixel = getSourceGradientPixel(0, 0, kWidth, kHeight, kMusic);
    CHECK(red(pixel) >= 0x30 && red(pixel) <= 0x4c);
}

TEST(bottom_left_halo_is_weaker_than_the_top_right_one) {
    const uint32_t top    = getSourceGradientPixel(kWidth * 9 / 10, 0, kWidth, kHeight, kMusic);
    const uint32_t bottom = getSourceGradientPixel(kWidth / 10, kHeight - 1, kWidth, kHeight, kMusic);
    CHECK(red(top) > red(bottom));
    CHECK(red(bottom) > red(getSourceGradientPixel(kWidth / 2, kHeight - 1, kWidth, kHeight, kMusic)) - 1);
}

TEST(render_matches_the_pixel_function_and_skips_the_padding) {
    constexpr int kW = 64, kH = 48, kPitch = 70;
    std::vector<uint32_t> pixels(static_cast<size_t>(kPitch * kH), 0xdeadbeefu);
    renderSourceGradient(pixels.data(), kW, kH, kPitch, kMusic);
    for (int y = 0; y < kH; y += 7) {
        for (int x = 0; x < kW; x += 5) {
            CHECK(pixels[static_cast<size_t>(y * kPitch + x)] == getSourceGradientPixel(x, y, kW, kH, kMusic));
        }
        CHECK(pixels[static_cast<size_t>(y * kPitch + kW)] == 0xdeadbeefu);
    }
}

TEST(palettes_compare_by_color) {
    SourcePalette other = kMusic;
    CHECK(other == kMusic);
    other.deep.r = 0;
    CHECK(!(other == kMusic));
}

int main() {
    RUN(each_pixel_is_opaque);
    RUN(top_right_corner_is_the_accent_halo_over_the_deep_tone);
    RUN(middle_left_is_the_plain_gradient);
    RUN(bottom_right_corner_is_close_to_the_near_black);
    RUN(top_left_corner_keeps_the_deep_tone);
    RUN(bottom_left_halo_is_weaker_than_the_top_right_one);
    RUN(render_matches_the_pixel_function_and_skips_the_padding);
    RUN(palettes_compare_by_color);
    return test_summary();
}

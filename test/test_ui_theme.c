#include "test.h"
#include "ui_theme.h"

static void check_color(SDL_Color color, int red, int green, int blue) {
    CHECK_EQ_INT(color.r, red);
    CHECK_EQ_INT(color.g, green);
    CHECK_EQ_INT(color.b, blue);
}

static void check_not_greyscale(SDL_Color color) {
    CHECK(color.r != color.g || color.g != color.b);
}

TEST(default_palette_keeps_start_lightness) {
    SDL_Color page = {
        .r = 0,
        .g = 0,
        .b = 0,
    };
    check_color(Theme_findLegibleHue(0, 4.5, page), 200, 80, 80);
    check_color(Theme_findLegibleHue(120, 4.5, page), 80, 200, 80);
}

TEST(teal_powder_moves_away_from_page) {
    SDL_Color page = {
        .r = 0xe9,
        .g = 0xf2,
        .b = 0xf5,
    };
    check_color(Theme_findLegibleHue(0, 4.5, page), 195, 63, 63);
    check_color(Theme_findLegibleHue(120, 4.5, page), 40, 126, 40);
}

TEST(mid_grey_keeps_green_hue) {
    SDL_Color page = {
        .r = 0x80,
        .g = 0x80,
        .b = 0x80,
    };
    check_not_greyscale(Theme_findLegibleHue(120, 4.5, page));
}

TEST(tan_keeps_green_hue) {
    SDL_Color page = {
        .r = 0xc8,
        .g = 0xa1,
        .b = 0x65,
    };
    check_not_greyscale(Theme_findLegibleHue(120, 4.5, page));
}

TEST(one_color_against_itself_gives_no_contrast) {
    SDL_Color grey = {
        .r = 0x99,
        .g = 0x99,
        .b = 0x99,
    };
    // The MinUI palette gives this grey for its surface, and the secondary text
    // role resolves to the same value. Theme_init() reads the ratio to find that.
    CHECK(Theme_contrast(grey, grey) < 1.001);
}

TEST(black_against_white_gives_the_full_range) {
    SDL_Color black = {.r = 0, .g = 0, .b = 0};
    SDL_Color white = {.r = 0xff, .g = 0xff, .b = 0xff};
    CHECK(Theme_contrast(black, white) > 20.9);
    CHECK(Theme_contrast(white, black) > 20.9);
}

TEST(each_source_has_its_accent_and_deep_tone) {
    check_color(Theme_getAccent(THEME_SOURCE_MUSIC), 0xa3, 0x24, 0x5c);
    check_color(Theme_getAccent(THEME_SOURCE_RADIO), 0x2a, 0x5f, 0xb8);
    check_color(Theme_getAccent(THEME_SOURCE_PODCAST), 0x1d, 0x7d, 0x53);
    check_color(Theme_getAccent(THEME_SOURCE_AUDIOBOOK), 0xa8, 0x68, 0x12);
    check_color(Theme_getAccent(THEME_SOURCE_NEUTRAL), 0x5b, 0x5b, 0x66);
    check_color(Theme_getDeep(THEME_SOURCE_MUSIC), 0x3b, 0x0c, 0x22);
    check_color(Theme_getDeep(THEME_SOURCE_RADIO), 0x0f, 0x22, 0x49);
    check_color(Theme_getDeep(THEME_SOURCE_PODCAST), 0x0b, 0x30, 0x21);
    check_color(Theme_getDeep(THEME_SOURCE_AUDIOBOOK), 0x3a, 0x23, 0x06);
    check_color(Theme_getDeep(THEME_SOURCE_NEUTRAL), 0x1c, 0x1c, 0x22);
}

TEST(unknown_source_gives_the_neutral_colors) {
    check_color(Theme_getAccent((ThemeSource)99), 0x5b, 0x5b, 0x66);
    check_color(Theme_getDeep((ThemeSource)99), 0x1c, 0x1c, 0x22);
}

TEST(each_accent_is_opaque_and_lighter_than_its_deep_tone) {
    for (int source = 0; source < THEME_SOURCE_COUNT; source++) {
        SDL_Color accent = Theme_getAccent((ThemeSource)source);
        SDL_Color deep   = Theme_getDeep((ThemeSource)source);
        CHECK_EQ_INT(accent.a, 255);
        CHECK_EQ_INT(deep.a, 255);
        CHECK(Theme_contrast(accent, deep) > 1.5);
    }
}

int main(void) {
    RUN(default_palette_keeps_start_lightness);
    RUN(teal_powder_moves_away_from_page);
    RUN(mid_grey_keeps_green_hue);
    RUN(tan_keeps_green_hue);
    RUN(one_color_against_itself_gives_no_contrast);
    RUN(black_against_white_gives_the_full_range);
    RUN(each_source_has_its_accent_and_deep_tone);
    RUN(unknown_source_gives_the_neutral_colors);
    RUN(each_accent_is_opaque_and_lighter_than_its_deep_tone);
    return test_summary();
}

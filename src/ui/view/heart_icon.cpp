#include "ui/view/heart_icon.hpp"

#include <array>
#include <cmath>

#include "ui_theme.h"

namespace {

/** The side of the surface, in pixels. See star_icon.cpp: the row scales it into 24 units. */
constexpr int kSide = 96;
/** A pixel counts kSamples x kSamples samples, for the smooth edge */
constexpr int kSamples = 4;
/** The width of the heart, as a part of the side. The same weight as the star next to the text. */
constexpr float kWidth = 0.64f;
constexpr size_t kPoints = 72;

struct Point {
    float x;
    float y;
};

/** Tells whether the point is inside the polygon, with the even-odd rule. */
bool isInside(const std::array<Point, kPoints>& polygon, float x, float y) {
    bool inside = false;
    for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const Point& a = polygon[i];
        const Point& b = polygon[j];
        if ((a.y > y) != (b.y > y) && x < (b.x - a.x) * (y - a.y) / (b.y - a.y) + a.x) inside = !inside;
    }
    return inside;
}

/** The outline of the classic parametric heart, fitted in kWidth of the side and centred. */
std::array<Point, kPoints> outline() {
    std::array<Point, kPoints> heart{};
    constexpr float pi = 3.14159265f;
    // The curve spans 32 units across and from -17 to 12 up: 29 units high
    const float scale = kSide * kWidth / 32.0f;
    for (size_t i = 0; i < heart.size(); i++) {
        const float t  = 2 * pi * static_cast<float>(i) / static_cast<float>(heart.size());
        const float x  = 16 * std::pow(std::sin(t), 3.0f);
        const float y  = 13 * std::cos(t) - 5 * std::cos(2 * t) - 2 * std::cos(3 * t) - std::cos(4 * t);
        heart[i] = Point{kSide * 0.5f + x * scale, kSide * 0.5f - (y + 2.5f) * scale};
    }
    return heart;
}

SDL_Surface* drawHeart(SDL_Color color) {
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, kSide, kSide, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!surface) return nullptr;
    const auto heart = outline();
    auto* pixels     = static_cast<Uint32*>(surface->pixels);
    const int pitch  = surface->pitch / static_cast<int>(sizeof(Uint32));
    for (int y = 0; y < kSide; y++) {
        for (int x = 0; x < kSide; x++) {
            int covered = 0;
            for (int sy = 0; sy < kSamples; sy++) {
                for (int sx = 0; sx < kSamples; sx++) {
                    const float px = static_cast<float>(x) + (static_cast<float>(sx) + 0.5f) / kSamples;
                    const float py = static_cast<float>(y) + (static_cast<float>(sy) + 0.5f) / kSamples;
                    if (isInside(heart, px, py)) covered++;
                }
            }
            const Uint8 alpha = static_cast<Uint8>(covered * color.a / (kSamples * kSamples));
            pixels[y * pitch + x] = SDL_MapRGBA(surface->format, color.r, color.g, color.b, alpha);
        }
    }
    return surface;
}

}  // namespace

SDL_Surface* getHeartIcon(bool selected) {
    struct Slot {
        SDL_Surface* surface = nullptr;
        SDL_Color color{};
    };
    static Slot slots[2];
    const SDL_Color wanted = Theme_getColor(THEME_ROLE_PRIMARY, selected);
    Slot& slot             = slots[selected ? 1 : 0];
    if (slot.surface && slot.color.r == wanted.r && slot.color.g == wanted.g && slot.color.b == wanted.b) {
        return slot.surface;
    }
    // An older surface can sit in a row view, thus it stays alive: the program frees it at the end
    slot.color   = wanted;
    slot.surface = drawHeart(wanted);
    return slot.surface;
}

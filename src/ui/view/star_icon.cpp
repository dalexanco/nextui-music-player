#include "ui/view/star_icon.hpp"

#include <array>
#include <cmath>

#include "ui_theme.h"

namespace {

/**
 * The side of the surface, in pixels. The row blits it scaled in a square of 24 units of the screen,
 * thus a fine surface keeps the edges smooth where a surface of 24 pixels showed its steps.
 */
constexpr int kSide = 96;
/** A pixel counts kSamples x kSamples samples, for the smooth edge */
constexpr int kSamples = 4;
/**
 * The outer radius of the star, as a part of the side. The star leaves a margin in its square, so it
 * reads as a mark next to the text instead of filling the whole height of the row.
 */
constexpr float kOuterRadius = 0.32f;
/** The inner radius, as a part of the outer one */
constexpr float kInnerRatio = 0.45f;

struct Point {
    float x;
    float y;
};

/** Tells whether the point is inside the polygon, with the even-odd rule. */
bool isInside(const std::array<Point, 10>& polygon, float x, float y) {
    bool inside = false;
    for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
        const Point& a = polygon[i];
        const Point& b = polygon[j];
        if ((a.y > y) != (b.y > y) && x < (b.x - a.x) * (y - a.y) / (b.y - a.y) + a.x) inside = !inside;
    }
    return inside;
}

SDL_Surface* drawStar(SDL_Color color) {
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, kSide, kSide, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!surface) return nullptr;
    constexpr float pi = 3.14159265f;
    std::array<Point, 10> star{};
    for (size_t i = 0; i < star.size(); i++) {
        const float outer  = kSide * kOuterRadius;
        const float radius = i % 2 == 0 ? outer : outer * kInnerRatio;
        const float angle  = -pi / 2 + static_cast<float>(i) * pi / 5;
        // The centre sits a little low: the visual weight of a star is in its lower half
        star[i] = Point{kSide * 0.5f + radius * std::cos(angle), kSide * 0.53f + radius * std::sin(angle)};
    }
    auto* pixels = static_cast<Uint32*>(surface->pixels);
    const int pitch = surface->pitch / static_cast<int>(sizeof(Uint32));
    for (int y = 0; y < kSide; y++) {
        for (int x = 0; x < kSide; x++) {
            int covered = 0;
            for (int sy = 0; sy < kSamples; sy++) {
                for (int sx = 0; sx < kSamples; sx++) {
                    const float px = static_cast<float>(x) + (static_cast<float>(sx) + 0.5f) / kSamples;
                    const float py = static_cast<float>(y) + (static_cast<float>(sy) + 0.5f) / kSamples;
                    if (isInside(star, px, py)) covered++;
                }
            }
            const Uint8 alpha = static_cast<Uint8>(covered * color.a / (kSamples * kSamples));
            pixels[y * pitch + x] = SDL_MapRGBA(surface->format, color.r, color.g, color.b, alpha);
        }
    }
    return surface;
}

}  // namespace

SDL_Surface* getStarIcon(bool selected) {
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
    slot.surface = drawStar(wanted);
    return slot.surface;
}

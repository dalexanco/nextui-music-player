#include "ui/view/star_icon.hpp"

#include <array>
#include <cmath>

#include "ui_theme.h"

namespace {

/** The side of the surface, in pixels before the scale of the screen */
constexpr int kSide = 24;

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
    SDL_FillRect(surface, nullptr, SDL_MapRGBA(surface->format, color.r, color.g, color.b, 0));
    constexpr float pi = 3.14159265f;
    std::array<Point, 10> star{};
    for (size_t i = 0; i < star.size(); i++) {
        const float radius = i % 2 == 0 ? kSide * 0.5f : kSide * 0.22f;
        const float angle  = -pi / 2 + static_cast<float>(i) * pi / 5;
        star[i]            = Point{kSide * 0.5f + radius * std::cos(angle), kSide * 0.54f + radius * std::sin(angle)};
    }
    const Uint32 ink = SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a);
    for (int y = 0; y < kSide; y++) {
        for (int x = 0; x < kSide; x++) {
            if (!isInside(star, static_cast<float>(x) + 0.5f, static_cast<float>(y) + 0.5f)) continue;
            SDL_Rect pixel{x, y, 1, 1};
            SDL_FillRect(surface, &pixel, ink);
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

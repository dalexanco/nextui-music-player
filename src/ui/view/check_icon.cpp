#include "ui/view/check_icon.hpp"

#include <algorithm>
#include <cmath>

#include "ui_theme.h"

namespace {

/** The side of the surface, in pixels before the scale of the screen */
constexpr int kSide = 24;

/** The distance of the point (x, y) to the segment from (ax, ay) to (bx, by) */
float distanceToSegment(float x, float y, float ax, float ay, float bx, float by) {
    const float dx     = bx - ax;
    const float dy     = by - ay;
    const float length = dx * dx + dy * dy;
    const float along  = length > 0 ? std::clamp(((x - ax) * dx + (y - ay) * dy) / length, 0.0f, 1.0f) : 0.0f;
    return std::hypot(x - (ax + along * dx), y - (ay + along * dy));
}

SDL_Surface* drawCheck(SDL_Color color) {
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, kSide, kSide, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!surface) return nullptr;
    SDL_FillRect(surface, nullptr, SDL_MapRGBA(surface->format, color.r, color.g, color.b, 0));
    const Uint32 ink = SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a);
    // A short stroke down to the foot, then a long stroke up
    constexpr float half = 1.8f;
    for (int y = 0; y < kSide; y++) {
        for (int x = 0; x < kSide; x++) {
            const float px = static_cast<float>(x) + 0.5f;
            const float py = static_cast<float>(y) + 0.5f;
            const float d  = std::min(distanceToSegment(px, py, 4.0f, 13.0f, 9.5f, 18.5f),
                                      distanceToSegment(px, py, 9.5f, 18.5f, 20.0f, 6.0f));
            if (d > half) continue;
            SDL_Rect pixel{x, y, 1, 1};
            SDL_FillRect(surface, &pixel, ink);
        }
    }
    return surface;
}

}  // namespace

SDL_Surface* getCheckIcon(bool selected) {
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
    slot.surface = drawCheck(wanted);
    return slot.surface;
}

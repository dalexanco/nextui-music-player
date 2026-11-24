#include "ui/view/source_gradient.hpp"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kPi = 3.14159265358979f;

// The angle of the base gradient, as CSS counts it: 0 is to the top, and 180 is to the bottom
constexpr float kAngleDegrees = 165.0f;

// The halo at the top right: radii in parts of the width and of the height, the center in parts
// of both, the opacity at the center, and the distance at which it reaches zero
constexpr float kTopRadiusX = 0.90f, kTopRadiusY = 0.80f;
constexpr float kTopCenterX = 0.90f, kTopCenterY = 0.0f;
constexpr float kTopAlpha = 140.0f / 255.0f, kTopReach = 0.72f;

// The halo at the bottom left
constexpr float kBottomRadiusX = 0.75f, kBottomRadiusY = 0.70f;
constexpr float kBottomCenterX = 0.10f, kBottomCenterY = 1.0f;
constexpr float kBottomAlpha = 64.0f / 255.0f, kBottomReach = 0.75f;

// The color that the base gradient ends on
//noinspection HardcodedColor
constexpr SDL_Color kFloor = {0x0a, 0x08, 0x0b, 0xff};

struct Rgb {
    float r, g, b;
};

Rgb mix(Rgb from, Rgb to, float amount) {
    return {from.r + (to.r - from.r) * amount, from.g + (to.g - from.g) * amount,
            from.b + (to.b - from.b) * amount};
}

Rgb toRgb(SDL_Color color) { return {static_cast<float>(color.r), static_cast<float>(color.g), static_cast<float>(color.b)}; }

// The opacity of a halo at (x, y): it falls in a straight line from `alpha` at the center to zero
// at `reach` times the radii
float haloAlpha(float x, float y, float centerX, float centerY, float radiusX, float radiusY, float alpha,
                float reach) {
    const float dx       = (x - centerX) / radiusX;
    const float dy       = (y - centerY) / radiusY;
    const float distance = std::sqrt(dx * dx + dy * dy);
    if (distance >= reach) return 0.0f;
    return alpha * (1.0f - distance / reach);
}

uint32_t pack(Rgb color) {
    const auto channel = [](float value) { return static_cast<uint32_t>(std::clamp(value + 0.5f, 0.0f, 255.0f)); };
    return 0xff000000u | (channel(color.r) << 16) | (channel(color.g) << 8) | channel(color.b);
}

struct Geometry {
    float width, height;
    // The direction of the base gradient, and the length of its line
    float directionX, directionY, length;
};

Geometry makeGeometry(int width, int height) {
    const float angle = kAngleDegrees * kPi / 180.0f;
    const float dx    = std::sin(angle);
    const float dy    = -std::cos(angle);
    const float w     = static_cast<float>(width);
    const float h     = static_cast<float>(height);
    return {w, h, dx, dy, std::fabs(w * dx) + std::fabs(h * dy)};
}

uint32_t pixelOf(const Geometry& geometry, int x, int y, const Rgb& deep, const Rgb& floor, const Rgb& accent) {
    const float px = static_cast<float>(x) + 0.5f;
    const float py = static_cast<float>(y) + 0.5f;
    const float t  = std::clamp(((px - geometry.width / 2) * geometry.directionX +
                                 (py - geometry.height / 2) * geometry.directionY) / geometry.length + 0.5f,
                                0.0f, 1.0f);
    Rgb color = mix(deep, floor, t);
    // The bottom halo is under the top one
    color = mix(color, accent,
                haloAlpha(px, py, kBottomCenterX * geometry.width, kBottomCenterY * geometry.height,
                          kBottomRadiusX * geometry.width, kBottomRadiusY * geometry.height, kBottomAlpha,
                          kBottomReach));
    color = mix(color, accent,
                haloAlpha(px, py, kTopCenterX * geometry.width, kTopCenterY * geometry.height,
                          kTopRadiusX * geometry.width, kTopRadiusY * geometry.height, kTopAlpha, kTopReach));
    return pack(color);
}

}  // namespace

uint32_t getSourceGradientPixel(int x, int y, int width, int height, const SourcePalette& palette) {
    return pixelOf(makeGeometry(width, height), x, y, toRgb(palette.deep), toRgb(kFloor), toRgb(palette.accent));
}

void renderSourceGradient(uint32_t* pixels, int width, int height, int pitchPixels, const SourcePalette& palette) {
    const Geometry geometry = makeGeometry(width, height);
    const Rgb deep          = toRgb(palette.deep);
    const Rgb floor         = toRgb(kFloor);
    const Rgb accent        = toRgb(palette.accent);
    for (int y = 0; y < height; y++) {
        uint32_t* row = pixels + static_cast<size_t>(y) * static_cast<size_t>(pitchPixels);
        for (int x = 0; x < width; x++) row[x] = pixelOf(geometry, x, y, deep, floor, accent);
    }
}

#include "ui/view/art_glyph.hpp"

#include <algorithm>
#include <cmath>

namespace {

constexpr float kPi = 3.14159265358979f;

/** The share of the smaller side of the tile that the glyph spans, from edge to edge */
constexpr float kSpan = 0.40f;

struct Vec {
    float x;
    float y;
};

// Each glyph is a union of signed distance shapes in a unit space: (0, 0) is the center, y points
// down and the glyph spans [-1, 1] on its larger side. Negative inside, positive outside.

float circle(Vec p, Vec c, float r) { return std::hypot(p.x - c.x, p.y - c.y) - r; }

float ellipse(Vec p, Vec c, float rx, float ry) {
    // A cheap distance to an ellipse: the circle distance in the space scaled by the radii
    const float k = std::min(rx, ry);
    return (std::hypot((p.x - c.x) / rx, (p.y - c.y) / ry) - 1.0f) * k;
}

float roundedBox(Vec p, Vec c, float hx, float hy, float r) {
    const float qx = std::fabs(p.x - c.x) - (hx - r);
    const float qy = std::fabs(p.y - c.y) - (hy - r);
    return std::hypot(std::max(qx, 0.0f), std::max(qy, 0.0f)) + std::min(std::max(qx, qy), 0.0f) - r;
}

/** A segment from a to b, `t` thick on each side, with round ends */
float capsule(Vec p, Vec a, Vec b, float t) {
    const float bax = b.x - a.x;
    const float bay = b.y - a.y;
    const float h   = std::clamp(((p.x - a.x) * bax + (p.y - a.y) * bay) / (bax * bax + bay * bay), 0.0f, 1.0f);
    return std::hypot(p.x - a.x - bax * h, p.y - a.y - bay * h) - t;
}

/** An arc of the circle of radius `r` around `c`, `half` radians to each side of the direction `mid` (y down), `t` thick on each side */
float arc(Vec p, Vec c, float r, float mid, float half, float t) {
    float delta = std::atan2(p.y - c.y, p.x - c.x) - mid;
    if (delta > kPi) delta -= 2 * kPi;
    if (delta < -kPi) delta += 2 * kPi;
    const float angle = mid + std::clamp(delta, -half, half);
    return std::hypot(p.x - c.x - r * std::cos(angle), p.y - c.y - r * std::sin(angle)) - t;
}

Vec rotate(Vec p, float angle) {
    const float c = std::cos(angle);
    const float s = std::sin(angle);
    return {p.x * c - p.y * s, p.x * s + p.y * c};
}

float music(Vec p) {
    p.x -= 0.075f;  // centers the pair, whose stems stand to the right of the heads
    float d = ellipse(p, {-0.55f, 0.55f}, 0.34f, 0.27f);
    d       = std::min(d, ellipse(p, {0.35f, 0.55f}, 0.34f, 0.27f));
    d       = std::min(d, roundedBox(p, {-0.24f, -0.1f}, 0.075f, 0.7f, 0.03f));
    d       = std::min(d, roundedBox(p, {0.66f, -0.1f}, 0.075f, 0.7f, 0.03f));
    d       = std::min(d, capsule(p, {-0.3f, -0.68f}, {0.72f, -0.68f}, 0.14f));
    return d;
}

float radio(Vec p) {
    // The right side is drawn, and the left one is its mirror
    const Vec q = {std::fabs(p.x), p.y};
    float d     = circle(p, {0.0f, 0.0f}, 0.2f);
    d           = std::min(d, arc(q, {0.0f, 0.0f}, 0.52f, 0.0f, 0.8f, 0.075f));
    d           = std::min(d, arc(q, {0.0f, 0.0f}, 0.88f, 0.0f, 0.7f, 0.075f));
    return d;
}

float podcast(Vec p) {
    float d = capsule(p, {0.0f, -0.52f}, {0.0f, -0.02f}, 0.3f);
    // The cradle under the capsule, with its stem and its foot
    d = std::min(d, arc(p, {0.0f, -0.02f}, 0.56f, kPi / 2, kPi / 2, 0.075f));
    d = std::min(d, capsule(p, {0.0f, 0.54f}, {0.0f, 0.86f}, 0.075f));
    d = std::min(d, capsule(p, {-0.34f, 0.9f}, {0.34f, 0.9f}, 0.075f));
    return d;
}

float book(Vec p) {
    // The two pages of an open book, one the mirror of the other, with lines of text carved in them
    const Vec q = {std::fabs(p.x), p.y};
    float d     = roundedBox(q, {0.52f, 0.0f}, 0.45f, 0.62f, 0.1f);
    for (float y : {-0.3f, -0.05f, 0.2f}) {
        const float line = capsule(q, {0.3f, y}, {0.72f, y}, 0.045f);
        d                = std::max(d, -line);
    }
    return d;
}

float playlist(Vec p) {
    float d = 1e9f;
    for (int i = 0; i < 3; i++) {
        const float y   = -0.5f + 0.5f * static_cast<float>(i);
        const float end = i == 2 ? 0.2f : 0.85f;
        d = std::min(d, circle(p, {-0.8f, y}, 0.1f));
        d = std::min(d, capsule(p, {-0.5f, y}, {end, y}, 0.08f));
    }
    return d;
}

float folder(Vec p) {
    float d = roundedBox(p, {0.0f, 0.12f}, 0.9f, 0.58f, 0.12f);
    d       = std::min(d, roundedBox(p, {-0.48f, -0.5f}, 0.4f, 0.2f, 0.1f));
    return d;
}

float neutral(Vec p) {
    float d = circle(p, {0.0f, 0.0f}, 0.62f);
    // Four bars through the center give the eight teeth
    for (int i = 0; i < 4; i++) {
        d = std::min(d, roundedBox(rotate(p, static_cast<float>(i) * kPi / 4), {0.0f, 0.0f}, 0.15f, 0.88f, 0.04f));
    }
    return std::max(d, -circle(p, {0.0f, 0.0f}, 0.26f));
}

float getDistance(ArtGlyph glyph, Vec p) {
    switch (glyph) {
        case ArtGlyph::Music: return music(p);
        case ArtGlyph::Radio: return radio(p);
        case ArtGlyph::Podcast: return podcast(p);
        case ArtGlyph::Book: return book(p);
        case ArtGlyph::Playlist: return playlist(p);
        case ArtGlyph::Folder: return folder(p);
        case ArtGlyph::Neutral: return neutral(p);
    }
    return 1e9f;
}

}  // namespace

float getArtGlyphCoverage(ArtGlyph glyph, int x, int y, int width, int height) {
    // How many pixels one unit of the glyph space covers: the glyph spans [-1, 1] over `kSpan` of the side
    const float scale = kSpan * static_cast<float>(std::min(width, height)) / 2.0f;
    if (scale <= 0.0f) return 0.0f;
    const Vec p = {(static_cast<float>(x) + 0.5f - static_cast<float>(width) / 2.0f) / scale,
                   (static_cast<float>(y) + 0.5f - static_cast<float>(height) / 2.0f) / scale};
    // The distance in pixels, as one pixel wide ramp
    return std::clamp(0.5f - getDistance(glyph, p) * scale, 0.0f, 1.0f);
}

void renderArtGlyph(uint32_t* pixels, int width, int height, int pitchPixels, ArtGlyph glyph, uint8_t alpha) {
    const float scale = kSpan * static_cast<float>(std::min(width, height)) / 2.0f;
    // The glyph stays within 1.1 units of the center
    const int reach = static_cast<int>(std::ceil(scale * 1.1f)) + 1;
    const int x0    = std::max(0, width / 2 - reach);
    const int x1    = std::min(width, width / 2 + reach);
    const int y0    = std::max(0, height / 2 - reach);
    const int y1    = std::min(height, height / 2 + reach);
    for (int y = y0; y < y1; y++) {
        uint32_t* row = pixels + static_cast<size_t>(y) * static_cast<size_t>(pitchPixels);
        for (int x = x0; x < x1; x++) {
            const float coverage = getArtGlyphCoverage(glyph, x, y, width, height);
            if (coverage <= 0.0f) continue;
            // White over the pixel, in premultiplied alpha: source + destination * (1 - source alpha)
            const float sourceAlpha = coverage * static_cast<float>(alpha) / 255.0f;
            const uint32_t pixel    = row[x];
            const auto blend        = [&](int shift) {
                const float destination = static_cast<float>((pixel >> shift) & 0xff);
                return static_cast<uint32_t>(std::clamp(255.0f * sourceAlpha + destination * (1.0f - sourceAlpha) + 0.5f, 0.0f, 255.0f));
            };
            row[x] = (blend(24) << 24) | (blend(16) << 16) | (blend(8) << 8) | blend(0);
        }
    }
}

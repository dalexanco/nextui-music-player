#include "ui/view/rounded_rect.hpp"

#include <algorithm>
#include <cmath>

namespace {

/**
 * The coverage of the pixel centered on (px, py) by the rounded rectangle that spans
 * [x0, x1] by [y0, y1]: from the signed distance to its outline, as one pixel wide ramp.
 */
float coverage(float px, float py, float x0, float y0, float x1, float y1, float radius) {
    const float halfW = (x1 - x0) / 2;
    const float halfH = (y1 - y0) / 2;
    if (halfW <= 0 || halfH <= 0) return 0.0f;
    const float r  = std::min(radius, std::min(halfW, halfH));
    const float qx = std::fabs(px - (x0 + x1) / 2) - (halfW - r);
    const float qy = std::fabs(py - (y0 + y1) / 2) - (halfH - r);
    const float outside = std::sqrt(std::max(qx, 0.0f) * std::max(qx, 0.0f) + std::max(qy, 0.0f) * std::max(qy, 0.0f));
    const float distance = outside + std::min(std::max(qx, qy), 0.0f) - r;
    return std::clamp(0.5f - distance, 0.0f, 1.0f);
}

}  // namespace

void renderRoundedRect(uint32_t* pixels, int width, int height, int pitchPixels, const RoundedRectStyle& style) {
    const float w      = static_cast<float>(width);
    const float h      = static_cast<float>(height);
    const float radius = static_cast<float>(std::max(0, style.radius));
    const float edge   = static_cast<float>(std::max(0, style.edgeWidth));
    const float fillA  = static_cast<float>(style.fill.a) / 255.0f;
    const float edgeA  = static_cast<float>(style.edge.a) / 255.0f;
    for (int y = 0; y < height; y++) {
        uint32_t* row = pixels + static_cast<size_t>(y) * static_cast<size_t>(pitchPixels);
        for (int x = 0; x < width; x++) {
            const float px    = static_cast<float>(x) + 0.5f;
            const float py    = static_cast<float>(y) + 0.5f;
            const float outer = coverage(px, py, 0, 0, w, h, radius);
            const float inner = edge > 0 ? coverage(px, py, edge, edge, w - edge, h - edge, std::max(0.0f, radius - edge)) : outer;
            // The ring is what the outline covers and the inside does not
            const float ring        = std::max(0.0f, outer - inner) * edgeA;
            const float inside      = inner * fillA;
            const float alpha       = ring + inside;
            const auto channel      = [&](uint8_t edgeValue, uint8_t fillValue) {
                const float value = static_cast<float>(edgeValue) * ring + static_cast<float>(fillValue) * inside;
                return static_cast<uint32_t>(std::clamp(value + 0.5f, 0.0f, 255.0f));
            };
            const uint32_t a = static_cast<uint32_t>(std::clamp(alpha * 255.0f + 0.5f, 0.0f, 255.0f));
            row[x] = (a << 24) | (channel(style.edge.r, style.fill.r) << 16) | (channel(style.edge.g, style.fill.g) << 8) |
                     channel(style.edge.b, style.fill.b);
        }
    }
}

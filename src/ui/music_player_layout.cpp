#include "ui/music_player_layout.hpp"

#include <algorithm>
#include <cstdio>

PlayerRects layoutPlayer(int width, int height, int padding, int inset, int pillHeight, int rowHeight) {
    PlayerRects rects{};
    // 620 of 1024 on the board
    const int artWidth = width * 620 / 1024;
    rects.art          = SDL_Rect{width - artWidth, 0, artWidth, height};
    rects.topRow       = SDL_Rect{padding + inset, padding, width / 2, pillHeight};
    // 132 of 768 on the board, and 560 of 1024 for the width
    const int textTop = height * 132 / 768;
    rects.textColumn  = SDL_Rect{padding + inset, textTop, width * 560 / 1024, 0};
    // 560 of 768 on the board
    const int progressTop = height * 560 / 768;
    rects.progressRow     = SDL_Rect{padding + inset, progressTop, width - 2 * (padding + inset), rowHeight};
    rects.textColumn.h    = std::max(0, progressTop - textTop);
    return rects;
}

SDL_Rect layoutLyricsRect(const PlayerRects& rects, int contentBottom, int gap) {
    const int top    = contentBottom + gap;
    const int bottom = rects.progressRow.y - gap;
    return SDL_Rect{rects.textColumn.x, top, rects.textColumn.w, std::max(0, bottom - top)};
}

int getArtFadeWidth(int width) {
    return width * 55 / 100;
}

int getArtFadeAlpha(int x, int width, int fadeWidth) {
    if (width <= 0 || fadeWidth <= 0 || x >= fadeWidth) return 255;
    if (x <= 0) return 0;
    // A smoothstep: 3t^2 - 2t^3, in fixed point of 1024
    const long t = static_cast<long>(x) * 1024 / fadeWidth;
    const long s = (3 * t * t * 1024 - 2 * t * t * t) / (1024L * 1024L);
    return static_cast<int>(std::clamp(s * 255 / 1024, 0L, 255L));
}

std::string formatPlayerTime(int milliseconds) {
    const int total = std::max(0, milliseconds) / 1000;
    char text[24];
    if (total >= 3600) {
        std::snprintf(text, sizeof(text), "%d:%02d:%02d", total / 3600, (total / 60) % 60, total % 60);
    } else {
        std::snprintf(text, sizeof(text), "%d:%02d", total / 60, total % 60);
    }
    return text;
}

std::string formatRemainingTime(int positionMs, int durationMs) {
    return "-" + formatPlayerTime(std::max(0, durationMs - positionMs));
}

std::string formatTrackCounter(int number, int total) {
    char text[24];
    std::snprintf(text, sizeof(text), "%02d / %02d", number, total);
    return text;
}

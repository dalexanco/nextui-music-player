#include "ui/view/art_fade_view.hpp"

#include <algorithm>
#include <cstdint>

#include "ui/music_player_layout.hpp"
#include "ui/view/source_gradient.hpp"

namespace {

/** Scales the art to cover the surface, and crops what falls outside. */
void drawCover(SDL_Surface& art, SDL_Surface& surface) {
    const float scale = std::max(static_cast<float>(surface.w) / static_cast<float>(art.w),
                                 static_cast<float>(surface.h) / static_cast<float>(art.h));
    const int cropW   = std::min(art.w, static_cast<int>(static_cast<float>(surface.w) / scale));
    const int cropH   = std::min(art.h, static_cast<int>(static_cast<float>(surface.h) / scale));
    SDL_Rect source{(art.w - cropW) / 2, (art.h - cropH) / 2, cropW, cropH};
    // The art covers the bitmap, thus its pixels replace the transparent ones
    SDL_SetSurfaceBlendMode(&art, SDL_BLENDMODE_NONE);
    SDL_BlitScaled(&art, &source, &surface, nullptr);
}

/** Multiplies the pixels by the fade of their column. The bitmap holds premultiplied alpha. */
void applyFade(SDL_Surface& surface) {
    const int fadeWidth = getArtFadeWidth(surface.w);
    for (int y = 0; y < surface.h; y++) {
        auto* row = reinterpret_cast<uint32_t*>(static_cast<uint8_t*>(surface.pixels) +
                                                static_cast<size_t>(y) * static_cast<size_t>(surface.pitch));
        for (int x = 0; x < fadeWidth && x < surface.w; x++) {
            const uint32_t alpha = static_cast<uint32_t>(getArtFadeAlpha(x, surface.w, fadeWidth));
            const uint32_t pixel = row[x];
            const uint32_t a     = ((pixel >> 24) * alpha + 127) / 255;
            const uint32_t r     = (((pixel >> 16) & 0xff) * alpha + 127) / 255;
            const uint32_t g     = (((pixel >> 8) & 0xff) * alpha + 127) / 255;
            const uint32_t b     = ((pixel & 0xff) * alpha + 127) / 255;
            row[x]               = (a << 24) | (r << 16) | (g << 8) | b;
        }
    }
}

}  // namespace

void ArtFadeView::setArt(SDL_Surface* newArt) {
    if (newArt == art) return;
    art   = newArt;
    dirty = true;
}

void ArtFadeView::setSource(ThemeSource newSource) {
    if (newSource == source) return;
    source = newSource;
    dirty  = true;
}

void ArtFadeView::draw(UiDrawContext& context) {
    dirty                = false;
    SDL_Surface* surface = context.getSurface();
    if (!surface) return;
    if (art && art->w > 0 && art->h > 0) {
        drawCover(*art, *surface);
    } else {
        const SDL_Color accent = Theme_getAccent(source);
        // The tile goes from a lighter accent to a darker one, as the art tile of the panels
        //noinspection HardcodedColor
        const SDL_Color white = {255, 255, 255, 255};
        renderArtTile(static_cast<uint32_t*>(surface->pixels), surface->w, surface->h, surface->pitch / 4,
                      Theme_mix(accent, white, 20), Theme_mix(accent, Theme_getDeep(source), 55), 0);
    }
    applyFade(*surface);
}

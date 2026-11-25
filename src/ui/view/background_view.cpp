#include "ui/view/background_view.hpp"

#include <cstring>

namespace {

/** The count of images that the view keeps: one for each source */
constexpr size_t kMaxImages = 5;

}  // namespace

BackgroundView::~BackgroundView() { dropImages(); }

void BackgroundView::setPalette(const SourcePalette& newPalette) {
    if (newPalette == palette) return;
    palette = newPalette;
    dirty   = true;
}

void BackgroundView::layout(UiLayoutContext& context) { context.setTargetLayer(UiTargetLayer::BackgroundLayer); }

SDL_Surface* BackgroundView::getImage(UiSize size) {
    // A new screen size makes each image wrong
    if (!images.empty() && (images.front().image->w != size.w || images.front().image->h != size.h)) dropImages();
    for (const CachedImage& cached : images) {
        if (cached.palette == palette) return cached.image;
    }
    SDL_Surface* image = SDL_CreateRGBSurfaceWithFormat(0, size.w, size.h, 32, SDL_PIXELFORMAT_ARGB8888);
    if (!image) return nullptr;
    renderSourceGradient(static_cast<uint32_t*>(image->pixels), size.w, size.h, image->pitch / 4, palette);
    if (images.size() >= kMaxImages) {
        SDL_FreeSurface(images.front().image);
        images.erase(images.begin());
    }
    images.push_back(CachedImage{palette, image});
    return image;
}

void BackgroundView::dropImages() {
    for (const CachedImage& cached : images) SDL_FreeSurface(cached.image);
    images.clear();
}

void BackgroundView::draw(UiDrawContext& context) {
    dirty = false;
    SDL_Surface* surface = context.getSurface();
    if (!surface) return;
    SDL_Surface* image = getImage(UiSize{surface->w, surface->h});
    if (!image) return;
    // Opaque pixels are the same with or without a premultiplied alpha
    for (int y = 0; y < surface->h; y++) {
        std::memcpy(static_cast<uint8_t*>(surface->pixels) + static_cast<size_t>(y) * static_cast<size_t>(surface->pitch),
                    static_cast<const uint8_t*>(image->pixels) + static_cast<size_t>(y) * static_cast<size_t>(image->pitch),
                    static_cast<size_t>(surface->w) * sizeof(uint32_t));
    }
}

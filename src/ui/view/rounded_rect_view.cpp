#include "ui/view/rounded_rect_view.hpp"

namespace {

bool same(SDL_Color a, SDL_Color b) { return a.r == b.r && a.g == b.g && a.b == b.b && a.a == b.a; }

}  // namespace

void RoundedRectView::setStyle(const RoundedRectStyle& newStyle) {
    if (same(newStyle.fill, style.fill) && same(newStyle.edge, style.edge) && newStyle.edgeWidth == style.edgeWidth &&
        newStyle.radius == style.radius) {
        return;
    }
    style = newStyle;
    dirty = true;
}

void RoundedRectView::draw(UiDrawContext& context) {
    dirty                = false;
    SDL_Surface* surface = context.getSurface();
    if (!surface) return;
    RoundedRectStyle drawn = style;
    if (drawn.radius <= 0) drawn.radius = surface->h;  // clamped to half the height by the render
    renderRoundedRect(static_cast<uint32_t*>(surface->pixels), surface->w, surface->h, surface->pitch / 4, drawn);
}

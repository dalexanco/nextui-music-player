#include "ui/view/icon_view.hpp"

void IconView::setSurfaces(SDL_Surface* normalIcon, SDL_Surface* selectedIcon) {
    if (normalIcon == normal && selectedIcon == selected) return;
    normal   = normalIcon;
    selected = selectedIcon;
    dirty    = true;
}

void IconView::setSelected(bool newSelected) {
    if (newSelected == isSelected) return;
    isSelected = newSelected;
    dirty      = true;
}

void IconView::draw(UiDrawContext& context) {
    dirty = false;
    SDL_Surface* icon    = isSelected ? selected : normal;
    SDL_Surface* surface = context.getSurface();
    if (!icon || !surface) return;
    SDL_SetSurfaceBlendMode(icon, SDL_BLENDMODE_BLEND);
    SDL_BlitScaled(icon, nullptr, surface, nullptr);
}

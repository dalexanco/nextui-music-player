#ifndef __UI_VIEW_ICON_VIEW_HPP__
#define __UI_VIEW_ICON_VIEW_HPP__

#include "ui/view/ui_view.hpp"

/**
 * An icon of the theme, scaled to the size of the view. It does not own the surfaces, which the
 * icon cache keeps for the whole run. It shows the second surface while it is selected.
 */
class IconView : public UiView {
public:
    /** Sets the surfaces for a row that is not selected and for the selected row. Null clears them. */
    void setSurfaces(SDL_Surface* normalIcon, SDL_Surface* selectedIcon);
    void setSelected(bool newSelected);

    bool isDirty() const override { return dirty; }
    void draw(UiDrawContext& context) override;

private:
    SDL_Surface* normal   = nullptr;
    SDL_Surface* selected = nullptr;
    bool isSelected       = false;
    bool dirty            = true;
};

#endif

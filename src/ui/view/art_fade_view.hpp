#ifndef __UI_VIEW_ART_FADE_VIEW_HPP__
#define __UI_VIEW_ART_FADE_VIEW_HPP__

#include "ui/view/ui_view.hpp"
#include "ui_theme.h"

/**
 * The art of a player, over the right of the screen, with its left edge fading into the background.
 * It shows the bitmap that the owner gives, or a generated tile in the accent of its source. The
 * fade is the one of `getArtFadeAlpha()`.
 */
class ArtFadeView : public UiView {
public:
    /** Shows `newArt`, which the owner keeps alive while it is set, or the generated tile for null. */
    void setArt(SDL_Surface* newArt);
    /** Sets the source whose accent the generated tile takes. The default is Music. */
    void setSource(ThemeSource newSource);

    bool isDirty() const override { return dirty; }
    void draw(UiDrawContext& context) override;

private:
    SDL_Surface* art    = nullptr;
    ThemeSource source  = THEME_SOURCE_MUSIC;
    bool dirty          = true;
};

#endif

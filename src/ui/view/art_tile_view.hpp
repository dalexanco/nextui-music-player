#ifndef __UI_VIEW_ART_TILE_VIEW_HPP__
#define __UI_VIEW_ART_TILE_VIEW_HPP__

#include <string>

#include "ui/view/bitmap_view.hpp"
#include "ui/view/text_view.hpp"
#include "ui/view/ui_layout.hpp"
#include "ui_theme.h"

/** The generated tile of an art that has no image: a gradient of the accent with rounded corners. */
class ArtTileFallbackView : public UiView {
public:
    void setSource(ThemeSource newSource);

    bool isDirty() const override { return dirty; }
    void draw(UiDrawContext& context) override;

private:
    ThemeSource source = THEME_SOURCE_NEUTRAL;
    bool dirty         = true;
};

/**
 * The art of an item, filling the rectangle that the parent gives it. It shows the bitmap while it
 * has one. Else it shows a generated tile in the accent of its source, with the title at the
 * bottom left. The parent gives it a square.
 */
class ArtTileView : public UiLayout {
public:
    ArtTileView();

    /** Takes `bitmap`, with straight alpha, and frees it with the view. A null bitmap shows the tile. */
    void setBitmap(SDL_Surface* bitmap) { bitmapView.setBitmap(bitmap); }
    bool loadFile(const char* path) { return bitmapView.loadFile(path); }
    bool hasBitmap() const { return bitmapView.hasBitmap(); }

    /** Sets the title of the tile and the source whose accent it takes. */
    void setFallback(const std::string& title, ThemeSource source);

    void layout(UiLayoutContext& context) override;

private:
    BitmapView bitmapView;
    ArtTileFallbackView tileView;
    TextView titleView;
};

#endif

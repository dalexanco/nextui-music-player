#ifndef __UI_VIEW_ART_TILE_VIEW_HPP__
#define __UI_VIEW_ART_TILE_VIEW_HPP__

#include <string>

#include "ui/view/art_glyph.hpp"
#include "ui/view/bitmap_view.hpp"
#include "ui/view/ui_layout.hpp"
#include "ui_theme.h"

/** The ArtGlyph that stands on the tile of a source: a note for music, waves for radio, a microphone for podcasts... */
ArtGlyph getArtGlyphOfSource(ThemeSource source);

/**
 * The generated tile of an art that has no image: a gradient of the accent with rounded corners,
 * and the pictogram of the content centered on it.
 */
class ArtTileFallbackView : public UiView {
public:
    /** Sets the source, whose accent the tile takes, and whose glyph it shows unless `setGlyph()` chose another. */
    void setSource(ThemeSource newSource);

    /** Shows `newGlyph` instead of the glyph of the source. */
    void setGlyph(ArtGlyph newGlyph);

    /** Goes back to the glyph of the source. */
    void clearGlyph();

    bool isDirty() const override { return dirty; }
    void draw(UiDrawContext& context) override;

private:
    ThemeSource source = THEME_SOURCE_NEUTRAL;
    ArtGlyph glyph     = ArtGlyph::Neutral;
    bool glyphChosen   = false;
    bool dirty         = true;
};

/**
 * The art of an item, filling the rectangle that the parent gives it. It shows the bitmap while it
 * has one. Else it shows a generated tile in the accent of its source. It never draws a text over
 * the image: the name of the item is in the list and in the info block under it. The parent gives
 * it a square.
 */
class ArtTileView : public UiLayout {
public:
    /** Takes `bitmap`, with straight alpha, and frees it with the view. A null bitmap shows the tile. */
    void setBitmap(SDL_Surface* bitmap) { bitmapView.setBitmap(bitmap); }
    bool loadFile(const char* path) { return bitmapView.loadFile(path); }
    bool hasBitmap() const { return bitmapView.hasBitmap(); }

    /** Sets the source whose accent and glyph the generated tile takes. */
    void setFallback(ThemeSource source) { tileView.setSource(source); }

    /** Puts `glyph` on the generated tile in place of the glyph of its source, as for a folder or a playlist. */
    void setGlyph(ArtGlyph glyph) { tileView.setGlyph(glyph); }
    void clearGlyph() { tileView.clearGlyph(); }

    void layout(UiLayoutContext& context) override;

private:
    BitmapView bitmapView;
    ArtTileFallbackView tileView;
};

#endif

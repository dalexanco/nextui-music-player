#ifndef __UI_VIEW_DETAIL_PANEL_VIEW_HPP__
#define __UI_VIEW_DETAIL_PANEL_VIEW_HPP__

#include "ui/view/art_tile_view.hpp"
#include "ui/view/info_block_view.hpp"
#include "ui/view/ui_layout.hpp"

/**
 * The panel at the right of a list: a square art of the width of the panel, and under it the info
 * block. Give it to ScreenView::setSidePanel(), which gives it the third of the screen. The page
 * fills the two parts with getArt() and getInfo() when its selection changes.
 */
class DetailPanelView : public UiLayout {
public:
    ArtTileView& getArt() { return art; }
    InfoBlockView& getInfo() { return info; }

    void layout(UiLayoutContext& context) override;

private:
    ArtTileView art;
    InfoBlockView info;
};

#endif

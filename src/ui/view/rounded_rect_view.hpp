#ifndef __UI_VIEW_ROUNDED_RECT_VIEW_HPP__
#define __UI_VIEW_ROUNDED_RECT_VIEW_HPP__

#include "ui/view/rounded_rect.hpp"
#include "ui/view/ui_view.hpp"

/**
 * A rounded rectangle that fills the space its parent gives: a panel, a pill, a chip. The bitmap of
 * the view is its cache, thus the shape renders again only when the style or the size changes.
 * A radius of 0 or less makes a pill, as round as the height allows.
 */
class RoundedRectView : public UiView {
public:
    void setStyle(const RoundedRectStyle& newStyle);

    bool isDirty() const override { return dirty; }
    void draw(UiDrawContext& context) override;

private:
    RoundedRectStyle style{};
    bool dirty = true;
};

#endif

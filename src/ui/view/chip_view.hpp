#ifndef __UI_VIEW_CHIP_VIEW_HPP__
#define __UI_VIEW_CHIP_VIEW_HPP__

#include <string>

#include "ui/view/text_view.hpp"
#include "ui/view/ui_layout.hpp"

/** A pill under a chip */
class PillView : public UiView {
public:
    bool isDirty() const override { return dirty; }
    void draw(UiDrawContext& context) override;

private:
    bool dirty = true;
};

/** A short label in a pill: the format of a track, a playback mode, the state of a stream */
class ChipView : public UiLayout {
public:
    ChipView();

    void setText(const std::string& label) { text.setText(label); }
    void setTextColor(SDL_Color color) { text.setColor(color); }

    void layout(UiLayoutContext& context) override;

private:
    PillView pill;
    TextView text;
};

#endif

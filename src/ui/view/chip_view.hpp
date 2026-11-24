#ifndef __UI_VIEW_CHIP_VIEW_HPP__
#define __UI_VIEW_CHIP_VIEW_HPP__

#include <string>

#include "ui/view/rounded_rect_view.hpp"
#include "ui/view/text_view.hpp"
#include "ui/view/ui_layout.hpp"

/**
 * A short label in a rounded shape: the format of a track, a playback mode, the state of a stream.
 * Two looks, as the board `tpl-lecteur` has them:
 * - the outline: a transparent box with a 2 px edge and small corners, for the format (MP3, RADIO);
 * - the pill (default): a round shape with a translucent white fill, for the other chips.
 * The shape is drawn anti-aliased by RoundedRectView, with no notch at the corners.
 */
class ChipView : public UiLayout {
public:
    ChipView();

    void setText(const std::string& label) { text.setText(label); }
    void setTextColor(SDL_Color color) { text.setColor(color); }

    /** Makes the chip an outline in the primary color of the theme */
    void setOutline();
    /** Makes the chip an outline in `color`, which also colors the text: a state that needs to be read, as OFFLINE */
    void setOutline(SDL_Color color);
    /** Goes back to the translucent pill */
    void setPill();

    void layout(UiLayoutContext& context) override;

private:
    RoundedRectView shape;
    TextView text;
    bool outline = false;
    bool hasColor = false;
    SDL_Color outlineColor{};
};

#endif

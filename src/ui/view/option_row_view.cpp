#include "ui/view/option_row_view.hpp"

#include <algorithm>

#include "ui_fonts.h"
#include "ui_theme.h"

// The NextUI headers declare C functions and have no guard of their own
extern "C" {
#include "defines.h"
#include "api.h"
}

namespace {

// The rows of the board are 84 px high where the lists have 90 px
int getRowHeight() { return SCALE1(PILL_SIZE) * 14 / 15; }

SDL_Color unpack(uint32_t rgba) {
    return SDL_Color{static_cast<Uint8>(rgba >> 24), static_cast<Uint8>(rgba >> 16), static_cast<Uint8>(rgba >> 8), 255};
}

}  // namespace

OptionRowView::OptionRowView(const std::string& labelText) : label(Fonts_getMedium(), true), valueText(Fonts_getMedium(), true) {
    label.setText(labelText);
    label.setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
    valueText.setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
}

void OptionRowView::setValue(const std::string& newValue) {
    if (newValue == value) return;
    value = newValue;
    refreshValueText();
}

void OptionRowView::setSelected(bool newSelected) {
    if (newSelected == selected) return;
    selected = newSelected;
    label.setColor(Theme_getColor(THEME_ROLE_PRIMARY, selected));
    // The value stays on the band, which is the accent: it keeps the color of the page text
    valueText.setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
    refreshValueText();
}

void OptionRowView::refreshValueText() {
    valueText.setText(selected && !value.empty() ? "\xe2\x80\xb9 " + value + " \xe2\x80\xba" : value);
}

void OptionRowView::layout(UiLayoutContext& context) {
    const int height  = std::min(context.getMaxSize().h, getRowHeight());
    const int width   = context.getMaxSize().w;
    const int padding = SCALE1(BUTTON_PADDING);
    context.setSize(UiSize{width, height});
    UiLayoutContext measuring = context.createMeasuringContext();

    const bool hasValue = !value.empty();
    int valueWidth      = 0;
    int valueHeight     = 0;
    if (hasValue) {
        const UiSize size = measuring.layoutChild(valueText, SDL_Rect{0, 0, width / 2, height}).getSize();
        valueWidth        = size.w;
        valueHeight       = size.h;
    }
    const int labelSpace  = std::max(0, width - 2 * padding - (hasValue ? valueWidth + 2 * padding : 0));
    const UiSize labelSize = measuring.layoutChild(label, SDL_Rect{0, 0, labelSpace, height}).getSize();

    const SDL_Color white = Theme_getColor(THEME_ROLE_PAGE_BACKGROUND, true);
    const bool showBand   = selected && hasValue;
    const uint32_t accent = Theme_getPillAccent();
    // Fully transparent: no color
    //noinspection HardcodedColor
    const SDL_Color clear = {0, 0, 0, 0};
    // The band: the accent of the page, or the band color of the theme where the page has no accent
    band.setStyle(RoundedRectStyle{showBand ? (accent ? unpack(accent) : Theme_getColor(THEME_ROLE_BAND_BACKGROUND, false)) : clear,
                                   clear, 0, 0});
    pill.setStyle(RoundedRectStyle{selected ? white : clear, clear, 0, 0});
    if (showBand) context.layoutChild(band, SDL_Rect{0, 0, width, height}).setVisible(true);
    if (selected) {
        context.layoutChild(pill, SDL_Rect{0, 0, std::min(width, labelSize.w + 2 * padding), height});
    }
    context.layoutChild(label, SDL_Rect{padding, (height - labelSize.h) / 2, labelSize.w, labelSize.h});
    if (hasValue) context.layoutChild(valueText, SDL_Rect{width - padding - valueWidth, (height - valueHeight) / 2, valueWidth, valueHeight});
}

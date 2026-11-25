#include "ui/view/list_row_view.hpp"

#include <algorithm>
#include <utility>

#include "ui_theme.h"

// The NextUI headers declare C functions and have no guard of their own
extern "C" {
#include "defines.h"
#include "api.h"
}

ListRowView::ListRowView(TTF_Font* font) : shortText(font, true), longText(font, false) {
    shortText.setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
    longText.setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
}

bool ListRowView::setText(const std::string& text) {
    shortText.setText(text);
    return longText.setText(text);
}

void ListRowView::setSelected(bool newSelected) {
    if (selected == newSelected) return;
    selected = newSelected;
    shortText.setColor(Theme_getColor(THEME_ROLE_PRIMARY, selected));
    longText.setColor(Theme_getColor(THEME_ROLE_PRIMARY, selected));
    if (iconView) iconView->setSelected(selected);
    if (!selected) {
        stopScrolling();
        resetScrolling();
    }
    dirty = true;
}

void ListRowView::setIcon(SDL_Surface* normalIcon, SDL_Surface* selectedIcon) {
    if (!normalIcon && !selectedIcon) {
        iconView.reset();
        dirty = true;
        return;
    }
    if (!iconView) {
        iconView = std::make_shared<IconView>();
        iconView->setSelected(selected);
    }
    iconView->setSurfaces(normalIcon, selectedIcon);
    dirty = true;
}

void ListRowView::setRightView(std::shared_ptr<UiView> view) {
    if (view == rightView) return;
    rightText.reset();
    rightView = std::move(view);
    dirty = true;
}

void ListRowView::setRightText(std::shared_ptr<TextView> text) {
    setRightView(text);
    rightText = std::move(text);
}

void ListRowView::layout(UiLayoutContext& context) {
    if (rightText) {
        rightText->setColor(selected ? Theme_getColor(THEME_ROLE_PRIMARY, true) : Theme_getColor(THEME_ROLE_SECONDARY, false));
    }
    // A row is as high as a selection pill
    const UiSize size       = {context.getMaxSize().w, std::min(context.getMaxSize().h, SCALE1(PILL_SIZE))};
    const int padding = SCALE1(BUTTON_PADDING);
    SDL_Rect textRect = {padding, 0, size.w - padding * 2, size.h};

    // The icon takes the left of the text, and the pill grows with it
    const int iconSize   = iconView ? SCALE1(24) : 0;
    const int iconOffset = iconView ? iconSize + SCALE1(6) : 0;
    textRect.x += iconOffset;
    textRect.w -= iconOffset;

    UiLayoutContext measuring = context.createMeasuringContext();
    SDL_Rect rightRect{};
    if (rightView) {
        const UiSize rightSize = measuring.layoutChild(*rightView, textRect).getSize();
        rightRect  = {size.w - padding - rightSize.w, (size.h - rightSize.h) / 2, rightSize.w, rightSize.h};
        // The text ends one padding before the right view, as the pill around it does
        textRect.w = std::max(0, rightRect.x - padding * 2 - textRect.x);
    }

    // Both texts know their space, thus the long text can start to scroll when the row is selected
    const int textHeight      = measuring.layoutChild(shortText, textRect).getSize().h;
    measuring.layoutChild(longText, textRect);
    textRect.y = (size.h - textHeight) / 2;
    textRect.h = textHeight;
    const bool clipped      = shortText.isClipped();
    const bool showLongText = selected && clipped;
    // The whole text lost its frame callback while the row showed the short text
    if (showLongText && !longTextShown && scrollRequested) longText.startScrolling();
    longTextShown   = showLongText;
    TextView& shown = showLongText ? longText : shortText;
    context.setSize(size);

    // A text that does not fit takes all of its space, and the pill with it
    const int textSpace    = textRect.w + padding * 2 + iconOffset;
    int newPillWidth = clipped ? textSpace : std::min(textSpace, shortText.getSize().w + padding * 2 + iconOffset);
    // The pill takes the whole row when there is a right view, thus the view is on it
    if (rightView) newPillWidth = size.w;
    if (newPillWidth != pillWidth || size.h != pillHeight) {
        pillWidth  = newPillWidth;
        pillHeight = size.h;
        dirty      = true;
    }
    context.layoutChild(shown, textRect);
    if (iconView) context.layoutChild(*iconView, SDL_Rect{padding, (size.h - iconSize) / 2, iconSize, iconSize});
    if (rightView) context.layoutChild(*rightView, rightRect);
}

void ListRowView::draw(UiDrawContext& context) {
    dirty = false;
    if (!selected) return;
    SDL_Rect pill = {0, 0, pillWidth, pillHeight};
    context.setContentRect(pill);
    // The fill argument marks a pill with no separate center color
    //noinspection HardcodedColor
    GFX_blitPillColor(ASSET_WHITE_PILL, context.getSurface(), &pill,
                      Theme_getPackedColor(THEME_ROLE_PAGE_BACKGROUND, true), RGB_WHITE);
}

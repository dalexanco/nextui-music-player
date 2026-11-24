#include "ui/view/options_panel_view.hpp"

#include <algorithm>

#include "ui/view/options_panel_layout.hpp"
#include "ui_fonts.h"
#include "ui_theme.h"

// The NextUI headers declare C functions and have no guard of their own
extern "C" {
#include "defines.h"
#include "api.h"
}

OptionsPanelView::OptionsPanelView() : titleView(Fonts_getLarge(), false), scopeView(Fonts_getTiny(), false) {
    titleView.setText("Options");
    titleView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
    scopeView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
    listView.setAdapter(adapter);
}

int OptionsPanelView::getRowCount() const { return static_cast<int>(adapter.rows.size()); }

void OptionsPanelView::setScope(const std::string& scope) { scopeView.setText(scope); }

void OptionsPanelView::setRows(const std::vector<OptionRow>& newRows) {
    const int selected = std::max(0, listView.getSelectedIndex());
    adapter.rows       = newRows;
    adapter.views.clear();
    adapter.valueViews.clear();
    for (const OptionRow& row : newRows) {
        auto view = std::make_shared<ListRowView>(Fonts_getMedium());
        view->setText(row.label);
        std::shared_ptr<TextView> value;
        if (!row.value.empty()) {
            value = std::make_shared<TextView>(Fonts_getMedium(), false);
            value->setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
            view->setRightText(value);
        }
        adapter.views.push_back(std::move(view));
        adapter.valueViews.push_back(std::move(value));
    }
    listView.notifyDataChanged();
    listView.setSelectedIndex(std::min(selected, std::max(0, getRowCount() - 1)));
    refreshValues();
}

void OptionsPanelView::setValue(int index, const std::string& value) {
    if (index < 0 || index >= getRowCount()) return;
    adapter.rows[static_cast<size_t>(index)].value = value;
}

void OptionsPanelView::moveSelection(int direction) {
    if (direction > 0) {
        listView.moveForward();
    } else {
        listView.moveBackward();
    }
}

void OptionsPanelView::refreshValues() {
    const int selected = listView.getSelectedIndex();
    for (size_t index = 0; index < adapter.rows.size(); index++) {
        const auto& value = adapter.valueViews[index];
        if (!value) continue;
        const std::string& text = adapter.rows[index].value;
        value->setText(static_cast<int>(index) == selected ? "\xe2\x80\xb9 " + text + " \xe2\x80\xba" : text);
    }
}

void OptionsPanelView::layout(UiLayoutContext& context) {
    refreshValues();
    const UiSize screen        = context.getMaxSize();
    UiLayoutContext measuring  = context.createMeasuringContext();
    const int padding          = SCALE1(PADDING);
    const int headerHeight     = measuring.layoutChild(titleView, SDL_Rect{0, 0, screen.w, screen.h}).getSize().h + padding;
    const int rowHeight        = SCALE1(PILL_SIZE);
    const int bottomLimit      = screen.h - padding - SCALE1(PILL_SIZE) - padding;
    const OptionsPanelRects rects = layoutOptionsPanel(screen.w, screen.h, padding, headerHeight, rowHeight, getRowCount(), bottomLimit);

    context.layoutChild(frame, rects.panel);
    const UiSize titleSize = measuring.layoutChild(titleView, rects.header).getSize();
    const UiSize scopeSize = measuring.layoutChild(scopeView, rects.header).getSize();
    context.layoutChild(titleView, SDL_Rect{rects.header.x, rects.header.y, titleSize.w, titleSize.h});
    // The scope ends the row at the right, on the baseline of the title
    context.layoutChild(scopeView, SDL_Rect{rects.header.x + rects.header.w - scopeSize.w,
                                            rects.header.y + titleSize.h - scopeSize.h - SCALE1(4), scopeSize.w, scopeSize.h});
    context.layoutChild(listView, rects.rows);
}

#ifndef __UI_VIEW_LIST_ROW_VIEW_HPP__
#define __UI_VIEW_LIST_ROW_VIEW_HPP__

#include <cstdint>
#include <memory>
#include <string>

#include <SDL2/SDL_ttf.h>

#include "ui/view/text_view.hpp"
#include "ui/view/list_item_view.hpp"

/**
 * A row of a list: a selection pill when it is selected, and its text. The row shows its whole
 * text, which can scroll, when it is selected and the text does not fit. Else the text ends in
 * "...".
 *
 * A row can have a right view at its right edge, and the text then takes the space to the left of
 * it. A selected row with a right view shows a band under the whole row.
 *
 * With the "Source colours" setting on, the selected pill of such a row extends over the right view
 * in place of the band, and a right text that the row owns is muted while the row is not selected.
 */
class ListRowView : public ListItemView {
public:
    explicit ListRowView(TTF_Font* font);

    /** Returns: true when the text changed */
    bool setText(const std::string& text);
    void setSelected(bool selected) override;

    /** Shows `view` at the right edge of the row. Pass nullptr in `view` to remove it. */
    void setRightView(std::shared_ptr<UiView> view);
    /**
     * Shows `text` at the right edge, as setRightView() does, and the row sets its color: the color
     * of the selected row on the pill, and with source colours on, the secondary color otherwise.
     * Without source colours the row leaves the color that the owner gave.
     */
    void setRightText(std::shared_ptr<TextView> text);
    bool isSelected() const { return selected; }

    /** The text moves again each time the row shows it, until stopScrolling(). */
    void startScrolling() override {
        scrollRequested = true;
        longText.startScrolling();
    }
    void stopScrolling() override {
        scrollRequested = false;
        longText.stopScrolling();
    }
    void resetScrolling() override { longText.resetScrolling(); }

    void layout(UiLayoutContext& context) override;
    bool isDirty() const override { return dirty; }
    void draw(UiDrawContext& context) override;

private:
    TextView shortText;
    TextView longText;
    std::shared_ptr<UiView> rightView;
    /** The right view when it is a text that the row colors, or null */
    std::shared_ptr<TextView> rightText;
    /** The row changed the color of the right text, thus it must give it back */
    bool rightTextRecolored = false;
    bool extendedPill       = false;
    bool selected  = false;
    bool dirty     = true;
    /** The owner started the scroll and did not stop it */
    bool scrollRequested = false;
    /** The last layout showed the whole text */
    bool longTextShown = false;
    int pillWidth  = 0;
    int pillHeight = 0;
    /** The width of the band, which is 0 for a row with no right view */
    int bandWidth  = 0;
};

#endif

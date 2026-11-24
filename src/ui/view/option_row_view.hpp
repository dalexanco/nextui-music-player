#ifndef __UI_VIEW_OPTION_ROW_VIEW_HPP__
#define __UI_VIEW_OPTION_ROW_VIEW_HPP__

#include <string>

#include "ui/view/list_item_view.hpp"
#include "ui/view/rounded_rect_view.hpp"
#include "ui/view/text_view.hpp"

/**
 * A row of the rounded options panel [`tpl-options`]: the label, and the value at the right. The
 * selected row has a white pill around its label, and an accent band under the whole row only
 * when it shows a value, as `‹ value ›`.
 */
class OptionRowView : public ListItemView {
public:
    explicit OptionRowView(const std::string& label);

    /** Sets the value without its arrows. An empty text makes a row that is an action. */
    void setValue(const std::string& newValue);
    void setSelected(bool newSelected) override;

    void layout(UiLayoutContext& context) override;

private:
    void refreshValueText();

    RoundedRectView band;
    RoundedRectView pill;
    TextView label;
    TextView valueText;
    std::string value;
    bool selected = false;
};

#endif

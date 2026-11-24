#ifndef __UI_VIEW_OPTIONS_PANEL_VIEW_HPP__
#define __UI_VIEW_OPTIONS_PANEL_VIEW_HPP__

#include <memory>
#include <string>
#include <vector>

#include "ui/view/dialog_view.hpp"
#include "ui/view/list_adapter.hpp"
#include "ui/view/list_row_view.hpp"
#include "ui/view/list_view.hpp"
#include "ui/view/text_view.hpp"
#include "ui/view/ui_layout.hpp"

/** A row of the options panel: a label, and the value that it shows at the right. */
struct OptionRow {
    std::string label;
    /** The value, or an empty text for a row that is an action and has no value */
    std::string value;
};

/**
 * The options panel of a player, at the right of the screen: a title and the scope of the options
 * (TRACK, STATION), then the rows. The selected row shows its value between arrows, `‹ On ›`. The
 * page moves the selection and changes the values, and gives them back through setValue().
 *
 * Give it the whole screen: it places itself with layoutOptionsPanel().
 */
class OptionsPanelView : public UiLayout {
public:
    OptionsPanelView();

    /** Sets the word at the right of the title, as "TRACK". */
    void setScope(const std::string& scope);

    /** Replaces the rows. The selection stays on its index where the new rows still have it. */
    void setRows(const std::vector<OptionRow>& newRows);

    /** Changes the value of the row at `index`. */
    void setValue(int index, const std::string& value);

    int getSelectedIndex() const { return listView.getSelectedIndex(); }
    int getRowCount() const;

    /** Moves the selection one row, and wraps at the ends. */
    void moveSelection(int direction);

    void layout(UiLayoutContext& context) override;

private:
    /** The rows of the panel. Each row keeps its view, thus a changed value does not rebuild the list. */
    class Adapter final : public ListAdapter {
    public:
        int getCount() const override { return static_cast<int>(rows.size()); }
        long getItemId(int index) const override { return index; }
        std::shared_ptr<ListItemView> getView(int index) override { return views[static_cast<size_t>(index)]; }

        std::vector<OptionRow> rows;
        std::vector<std::shared_ptr<ListRowView>> views;
        std::vector<std::shared_ptr<TextView>> valueViews;
    };

    /** Shows the value of each row, between arrows for the selected row. */
    void refreshValues();

    DialogFrameView frame;
    TextView titleView;
    TextView scopeView;
    ListView listView;
    Adapter adapter;
};

#endif

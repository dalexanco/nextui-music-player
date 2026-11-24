#ifndef __UI_VIEW_INFO_BLOCK_VIEW_HPP__
#define __UI_VIEW_INFO_BLOCK_VIEW_HPP__

#include <string>

#include "ui/view/progress_bar_view.hpp"
#include "ui/view/text_view.hpp"
#include "ui/view/ui_layout.hpp"

/**
 * The block under the art of a detail panel, the same on each source: a line (the artist, the
 * author or the source), a progress bar, the position (`Track 2 of 15`) at the left and the elapsed
 * and total time (`1:20 / 3:45`) at the right. A row with no text takes no room, but keeps its
 * place, thus the panel does not move. The bar shows only after setProgress().
 *
 * A text that is wider than its room scrolls, as the rows of a list do, and does not end in "...".
 * The times go under the position when the two do not fit on one row.
 */
class InfoBlockView : public UiLayout {
public:
    InfoBlockView();

    void setLine(const std::string& text) { lineView.setText(text); }
    void setPosition(const std::string& text) { positionView.setText(text); }
    void setTimes(const std::string& text) { timesView.setText(text); }

    /** Fills the bar to `percent`, from 0 to 100. A negative value hides the bar. */
    void setProgress(int percent);

    void layout(UiLayoutContext& context) override;

protected:
    /** The texts scroll again each time the block comes back to the layout. */
    void onAttach(UiContext& context) override;

private:
    TextView lineView;
    ProgressTrackView barView;
    TextView positionView;
    TextView timesView;
    bool barShown = false;
};

#endif

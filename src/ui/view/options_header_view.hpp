#ifndef __UI_VIEW_OPTIONS_HEADER_VIEW_HPP__
#define __UI_VIEW_OPTIONS_HEADER_VIEW_HPP__

#include <string>

#include "ui/view/text_view.hpp"
#include "ui/view/ui_layout.hpp"

/**
 * The text at the left of the options panel [`tpl-options`]: a line of meta (artist, show, genre)
 * and the large title under it. The text gets the room that the panel leaves at its left, no
 * more, and a title that does not fit scrolls. Give it the whole screen.
 */
class OptionsHeaderView : public UiLayout {
public:
    OptionsHeaderView();

    void setMeta(const std::string& text) { meta.setText(text); }
    void setTitle(const std::string& text) { title.setText(text); }

    void layout(UiLayoutContext& context) override;

private:
    TextView meta;
    TextView title;
};

#endif

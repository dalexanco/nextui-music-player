#ifndef __UI_VIEW_TITLE_TEXT_VIEW_HPP__
#define __UI_VIEW_TITLE_TEXT_VIEW_HPP__

#include "ui/view/text_view.hpp"

/**
 * The title of a screen. A title that does not fit fades out at the cut in place of the ellipsis.
 *
 * The heading look is a larger font in a color of its own. Once it is set, the title ignores the
 * color that the page sets.
 */
class TitleTextView : public TextView {
public:
    explicit TitleTextView(TTF_Font* font) : TextView(font, false) {}

    /** Sets the color of the regular look. It shows until the heading look is set. */
    void setColor(SDL_Color color);

    /** Uses `font` and `color` in place of the font of the page and of its color. */
    void setHeading(TTF_Font* font, SDL_Color color);

protected:
    void finishDraw(SDL_Surface* bitmap) override;

private:
    bool heading = false;
};

#endif

#ifndef __UI_VIEW_TITLE_TEXT_VIEW_HPP__
#define __UI_VIEW_TITLE_TEXT_VIEW_HPP__

#include "ui/view/text_view.hpp"

/**
 * The title of a screen. A title that does not fit fades out at the cut in place of the ellipsis.
 *
 * The heading look is a larger font in a color of its own, for the "Source colours" setting. While
 * it is on, the title ignores the color that the page sets, and takes it again when the look ends.
 */
class TitleTextView : public TextView {
public:
    explicit TitleTextView(TTF_Font* font) : TextView(font, false), regularFont(font) {}

    /** Sets the color of the regular look. It shows while the heading look is off. */
    void setColor(SDL_Color color);

    /** Uses `font` and `color` in place of the font of the page and of its color. */
    void setHeading(TTF_Font* font, SDL_Color color);

    /** Goes back to the font and to the color of the page. */
    void clearHeading();

protected:
    void finishDraw(SDL_Surface* bitmap) override;

private:
    TTF_Font* regularFont;
    // White, as in TextView, until the page sets a color
    //noinspection HardcodedColor
    SDL_Color regularColor = {255, 255, 255, 255};
    bool heading           = false;
};

#endif

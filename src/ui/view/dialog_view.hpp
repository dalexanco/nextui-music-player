#ifndef __UI_VIEW_DIALOG_VIEW_HPP__
#define __UI_VIEW_DIALOG_VIEW_HPP__

#include <string>

#include "ui/view/text_view.hpp"
#include "ui/view/ui_view.hpp"

/**
 * The ground and the border of a dialog. With the setting "Source colours" off it is the flat
 * rectangle with a 2 px border of NextUI; with it on, a rounded surface (anti-aliased, see
 * renderRoundedRect()) with a thin edge, or with no edge for a panel.
 */
class DialogFrameView : public UiView {
public:
    /** Draws a panel with no edge in the rounded look. The square look keeps its border. */
    void setBorderless(bool value);

    /** Draws the border in `color` in place of the primary color of the theme. */
    void setEdgeColor(SDL_Color color);
    /** Goes back to the primary color of the theme. */
    void clearEdgeColor();

    bool isDirty() const override { return dirty; }
    void draw(UiDrawContext& context) override;

private:
    bool hasEdgeColor = false;
    bool borderless   = false;
    SDL_Color edgeColor{};
    bool dirty = true;
};

/**
 * A dialog in the middle of the space that the parent gives: a surface with a title, an optional
 * content view, and a hint line at the bottom. It draws nothing of its own.
 */
class DialogView : public UiView {
public:
    enum class Alignment : uint8_t { Center, Left };

    /** Pass the width of the surface in width, in pixels. */
    DialogView(int width, Alignment alignment, TTF_Font* titleFont, TTF_Font* hintFont);

    void setTitle(const std::string& text) { title.setText(text); }
    void setHint(const std::string& text) { hint.setText(text); }

    /** Draws the border of the dialog in `color`. */
    void setEdgeColor(SDL_Color color) { frame.setEdgeColor(color); }
    void clearEdgeColor() { frame.clearEdgeColor(); }

    /**
     * Shows content between the title and the hint, as high as its layout makes it. A centered
     * dialog puts the content in the middle, as wide as its layout makes it.
     */
    void setContent(UiView* content) { this->content = content; }

    void layout(UiLayoutContext& context) override;

private:
    /** Lays out text in the row at y, at the left edge or in the middle of the surface. */
    void layoutText(UiLayoutContext& context, UiLayoutContext& measuring, TextView& text, const SDL_Rect& surface,
                    int y);

    int width;
    Alignment alignment;
    DialogFrameView frame;
    TextView title;
    TextView hint;
    UiView* content = nullptr;
};

#endif

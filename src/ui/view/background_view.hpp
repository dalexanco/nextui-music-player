#ifndef __UI_VIEW_BACKGROUND_VIEW_HPP__
#define __UI_VIEW_BACKGROUND_VIEW_HPP__

#include <vector>

#include "ui/view/source_gradient.hpp"
#include "ui/view/ui_view.hpp"

/**
 * The soft background of a page: see getSourceGradientPixel(). It fills its rectangle, which is the
 * whole screen, and goes on the layer under the regular surface. It renders each palette one time
 * and keeps the image, thus a new page of another source copies a bitmap and does not compute.
 */
class BackgroundView : public UiView {
public:
    ~BackgroundView() override;

    /** Sets the palette to show. A palette of the last draw costs nothing. */
    void setPalette(const SourcePalette& newPalette);

    void layout(UiLayoutContext& context) override;
    bool isDirty() const override { return dirty; }
    void draw(UiDrawContext& context) override;

private:
    struct CachedImage {
        SourcePalette palette;
        SDL_Surface* image;
    };

    SDL_Surface* getImage(UiSize size);
    void dropImages();

    SourcePalette palette{};
    std::vector<CachedImage> images;
    bool dirty = true;
};

#endif

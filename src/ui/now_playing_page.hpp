#ifndef __UI_NOW_PLAYING_PAGE_HPP__
#define __UI_NOW_PLAYING_PAGE_HPP__

#include <memory>

#include "ui/page/ui_page.hpp"

/** Runs the legacy module of the Now Playing row of Main. Closes when the module returns. */
class NowPlayingPage final : public UiPage {
public:
    /** true when the screen closed because the queue of the music ended */
    using Result = bool;
    std::unique_ptr<UiPageImplBase> createPageImpl() const override;
};

#endif

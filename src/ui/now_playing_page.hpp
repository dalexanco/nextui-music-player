#ifndef __UI_NOW_PLAYING_PAGE_HPP__
#define __UI_NOW_PLAYING_PAGE_HPP__

#include <memory>

#include "ui/page/ui_page.hpp"

/**
 * The Now Playing row of Main: the native player while the music plays, and the legacy module of
 * the podcast otherwise. Closes when the screen closes.
 */
class NowPlayingPage final : public UiPage {
public:
    /** true when the screen closed because the queue of the music ended */
    using Result = bool;
    std::unique_ptr<UiPageImplBase> createPageImpl() const override;
};

#endif

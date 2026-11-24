#ifndef __UI_MUSIC_PLAYER_PAGE_HPP__
#define __UI_MUSIC_PLAYER_PAGE_HPP__

#include <memory>

#include "ui/page/ui_page.hpp"

/**
 * Creates the page of the music player, the one that NowPlayingPage opens while the music plays. It
 * derives from UiPageImpl of NowPlayingPage, thus a caller reads its result: true when the screen
 * closed because the queue ended.
 */
std::unique_ptr<UiPageImplBase> createMusicPlayerPageImpl();

#endif

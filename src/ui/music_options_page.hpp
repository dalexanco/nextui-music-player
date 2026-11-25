#ifndef __UI_MUSIC_OPTIONS_PAGE_HPP__
#define __UI_MUSIC_OPTIONS_PAGE_HPP__

#include <memory>
#include <variant>

#include "ui/page/ui_page.hpp"

/**
 * The options of the track that plays, in the panel of the player: shuffle, repeat, lyrics, the
 * sleep timer, and the choice of a playlist. Left and Right change a value, A changes it too or
 * runs the action of the row, B closes the page.
 */
class MusicOptionsPage final : public UiPage {
public:
    using Result = std::monostate;
    std::unique_ptr<UiPageImplBase> createPageImpl() const override;
};

#endif

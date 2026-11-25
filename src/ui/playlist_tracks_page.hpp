#ifndef __UI_PLAYLIST_TRACKS_PAGE_HPP__
#define __UI_PLAYLIST_TRACKS_PAGE_HPP__

#include <memory>
#include <string>
#include <variant>

#include "ui/page/ui_page.hpp"

/** The tracks of one playlist: A plays from the track, X removes it from the playlist. */
class PlaylistTracksPage final : public UiPage {
public:
    using Result = std::monostate;

    PlaylistTracksPage(int playlistId, std::string name) : playlistId(playlistId), name(std::move(name)) {}

    std::unique_ptr<UiPageImplBase> createPageImpl() const override;

private:
    int playlistId;
    std::string name;
};

#endif

#ifndef __UI_ADD_TO_PLAYLIST_PAGE_HPP__
#define __UI_ADD_TO_PLAYLIST_PAGE_HPP__

#include <memory>
#include <variant>

#include "ui/page/ui_page.hpp"

/**
 * The choice of a playlist for one indexed file or for the files of a folder. The dialog is the
 * legacy one of add_to_playlist.c: it runs in its own frame loop, thus this is a legacy page.
 */
class AddToPlaylistPage final : public UiPage {
public:
    using Result = std::monostate;

    /** Adds the file `id` to the playlist that the user picks. */
    static AddToPlaylistPage forFile(int id) { return AddToPlaylistPage(id, false); }
    /** Adds the audio files of the folder `id` to the playlist that the user picks. */
    static AddToPlaylistPage forFolder(int id) { return AddToPlaylistPage(id, true); }

    std::unique_ptr<UiPageImplBase> createPageImpl() const override;

private:
    AddToPlaylistPage(int id, bool folder) : id(id), folder(folder) {}

    int id;
    bool folder;
};

#endif

#include "ui/add_to_playlist_page.hpp"

#include "add_to_playlist.h"
#include "ui/page/legacy_module_page.hpp"

namespace {

class AddToPlaylistPageImpl final : public LegacyModulePageImpl<AddToPlaylistPage> {
public:
    AddToPlaylistPageImpl(int id, bool folder) : id(id), folder(folder) {}

protected:
    ModuleExitReason runModule(DisplayContext* display) override {
        if (folder) {
            AddToPlaylist_openDir(id);
        } else {
            AddToPlaylist_open(id);
        }
        // A folder with no file, or a file that is not indexed, opens no dialog
        while (AddToPlaylist_isActive()) {
            ModuleCommon_frameBegin(true);
            SDL_Surface* const screen = DisplayHelper_getSurface(display);
            if (ModuleCommon_stopSignalled() || ModuleCommon_isQuitRequested()) return MODULE_EXIT_QUIT;
            // The frame that closes the dialog draws nothing: the page under it draws next
            if (AddToPlaylist_handleInput()) {
                ModuleCommon_frameEnd(screen);
                break;
            }
            AddToPlaylist_render(screen);
            ModuleCommon_markSurfaceDrawn();
            ModuleCommon_frameEnd(screen);
        }
        return MODULE_EXIT_BACK;
    }

private:
    int id;
    bool folder;
};

}  // namespace

std::unique_ptr<UiPageImplBase> AddToPlaylistPage::createPageImpl() const {
    return std::make_unique<AddToPlaylistPageImpl>(id, folder);
}

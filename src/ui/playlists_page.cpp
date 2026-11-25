#include "ui/playlists_page.hpp"

#include <memory>
#include <string>

#include "filedb.h"
#include "playlist_m3u.h"
#include "toast.h"
#include "ui/confirm_dialog_page.hpp"
#include "ui/choice_options_page.hpp"
#include "ui/page/list_screen_controller.hpp"
#include "ui/playlist_tracks_page.hpp"
#include "ui/text_input_page.hpp"
#include "ui/view/detail_panel_view.hpp"
#include "ui/view/empty_state_view.hpp"
#include "ui/view/list_adapter.hpp"
#include "ui/view/list_row_view.hpp"
#include "ui/view/list_screen_view.hpp"
#include "ui_fonts.h"
#include "ui_theme.h"

using namespace input;

namespace {

constexpr const char* kLeftHints[]       = {"Y", "OPTIONS", nullptr};
constexpr const char* kRightHints[]      = {"B", "BACK", "A", "OPEN", nullptr};
// With no playlist the one action besides B is the third, shown as it is
constexpr const char* kEmptyLeftHints[]  = {"Y", "NEW", nullptr};
constexpr const char* kEmptyRightHints[] = {"B", "BACK", nullptr};

/** The rows of the playlists, which the page reads again after a change */
class PlaylistListAdapter final : public ListAdapter {
public:
    /** Reads the playlists from the index. */
    void reload() { count = M3U_listPlaylists(playlists.get(), MAX_PLAYLISTS); }

    int getCount() const override { return count; }
    long getItemId(int index) const override { return playlists[index].id; }

    const PlaylistInfo* get(int index) const { return index >= 0 && index < count ? &playlists[index] : nullptr; }

    std::shared_ptr<ListItemView> getView(int index) override {
        const PlaylistInfo& playlist = playlists[index];
        auto row                     = std::make_shared<ListRowView>(Fonts_getMedium());
        row->setText(std::string(playlist.name) + " (" + std::to_string(playlist.track_count) + ")");
        return row;
    }

private:
    std::unique_ptr<PlaylistInfo[]> playlists = std::make_unique<PlaylistInfo[]>(MAX_PLAYLISTS);
    int count                                 = 0;
};

class PlaylistsPageImpl final : public UiPageImpl<PlaylistsPage>, private ListScreenListener {
public:
    PlaylistsPageImpl() : UiPageImpl(PageKind::FullScreen) {
        screenView.getTitleView().setText("Playlists");
        screenView.getTitleView().setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        emptyState.setMessage("No playlists saved");
        emptyState.setLine("Press Y to create a playlist");
        screenView.setEmptyState(&emptyState);
    }

    std::span<const ButtonRequest> getButtonRequests() const override {
        return ListScreenController::getButtonRequests();
    }

    HelpId getHelpId() const override { return HELP_PLAYLIST_LIST; }
    ThemeSource getSource() const override { return THEME_SOURCE_MUSIC; }

    void onEnter() override {
        M3U_init();
        FileDb_waitBlocking(FileDb_scanPlaylists());
        playlists.reload();
        screenView.setAdapter(playlists);
        list.start();
        updateHints();
    }

    /** The playlist that closed can have a new count of tracks */
    void resume() override { reloadKeepingCursor(); }

    void layout(UiLayoutContext& context) override {
        // An empty list has nothing to show in a panel
        screenView.setSidePanel(Theme_isSourceColoursEnabled() && playlists.getCount() > 0 ? &panel : nullptr);
        context.layoutChild(screenView, context.getMaxRect());
    }

    bool onButtonEvent(const ButtonEvent& event) override {
        if (list.onButtonEvent(event)) return true;
        if (event.isSingleButton(Button::A)) {
            openSelected();
            return true;
        }
        if (event.isSingleButton(Button::B)) {
            close();
            return true;
        }
        if (event.isSingleButton(Button::Y)) {
            openOptions();
            return true;
        }
        return false;
    }

private:
    void onSelectionChanged(int index) override { updatePanel(index); }

    /** Fills the detail panel for the playlist at `index`: a tile with its name, its count, its place */
    void updatePanel(int index) {
        const PlaylistInfo* playlist = playlists.get(index);
        if (!playlist) return;
        panel.getArt().setFallback(THEME_SOURCE_MUSIC);
        panel.getArt().setGlyph(ArtGlyph::Playlist);
        panel.getInfo().setLine(std::to_string(playlist->track_count) +
                                (playlist->track_count == 1 ? " track" : " tracks"));
    }

    void openSelected() {
        const PlaylistInfo* playlist = playlists.get(list.getSelectedIndex());
        if (!playlist) return;
        getNavigator().open(PlaylistTracksPage(playlist->id, playlist->name));
    }

    /** Y: a new playlist (alone while there is none), or the delete of the playlist under the cursor. */
    void openOptions() {
        const PlaylistInfo* playlist = playlists.get(list.getSelectedIndex());
        if (!playlist) {
            createPlaylist();
            return;
        }
        openActionOptions(getNavigator(), THEME_SOURCE_MUSIC, "PLAYLIST", "Playlist", playlist->name,
                          {{"New playlist", [this] { createPlaylist(); }}, {"Delete playlist", [this] { confirmDelete(); }}});
    }

    void createPlaylist() {
        getNavigator().openForResult(TextInputPage("Playlist name", MAX_PLAYLIST_NAME - 1), [this](const std::string& name) {
            if (name.empty()) return;
            if (M3U_create(name.c_str()) > 0) {
                Toast_show("Playlist created", TOAST_DURATION);
                reloadKeepingCursor();
            } else {
                Toast_show("Already exists", TOAST_DURATION);
            }
        });
    }

    void confirmDelete() {
        const PlaylistInfo* playlist = playlists.get(list.getSelectedIndex());
        if (!playlist) return;
        const int id = playlist->id;
        getNavigator().openForResult(ConfirmDialogPage("Delete Playlist?", playlist->name), [this, id](const bool& yes) {
            if (!yes) return;
            M3U_delete(id);
            Toast_show("Playlist deleted", TOAST_DURATION);
            reloadKeepingCursor();
        });
    }

    /** Reads the playlists again. The cursor keeps its position, or goes to the last row. */
    void reloadKeepingCursor() {
        const int position = list.getSelectedIndex();
        playlists.reload();
        list.notifyDataChanged();
        if (position >= 0) list.setSelectedIndex(position);
        updatePanel(list.getSelectedIndex());
        updateHints();
    }

    void updateHints() {
        const bool empty = playlists.getCount() == 0;
        screenView.getButtonHintsView().setHints(empty ? kEmptyLeftHints : kLeftHints, empty ? kEmptyRightHints : kRightHints);
    }

    PlaylistListAdapter playlists;
    ListScreenView screenView{Fonts_getLarge()};
    EmptyStateView emptyState;
    DetailPanelView panel;
    ListScreenController list{screenView, getScheduler(), *this};
};

}  // namespace

std::unique_ptr<UiPageImplBase> PlaylistsPage::createPageImpl() const {
    return std::make_unique<PlaylistsPageImpl>();
}

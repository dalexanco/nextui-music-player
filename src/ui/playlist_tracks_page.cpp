#include "ui/playlist_tracks_page.hpp"

#include <memory>
#include <string>

#include "music_session.h"
#include "playlist.h"
#include "playlist_m3u.h"
#include "toast.h"
#include "ui/confirm_dialog_page.hpp"
#include "ui/now_playing_page.hpp"
#include "ui/page/list_screen_controller.hpp"
#include "ui/view/detail_panel_view.hpp"
#include "ui/view/empty_state_view.hpp"
#include "ui/view/list_adapter.hpp"
#include "ui/view/list_row_view.hpp"
#include "ui/view/list_screen_view.hpp"
#include "ui_fonts.h"
#include "ui_icons.h"
#include "ui_theme.h"

using namespace input;

namespace {

constexpr const char* kLeftHints[]  = {"START", "CONTROLS", nullptr};
constexpr const char* kRightHints[] = {"B", "BACK", "A", "PLAY", nullptr};

/** The rows of the tracks of a playlist */
class TrackListAdapter final : public ListAdapter {
public:
    void reload(int playlistId) {
        const int loaded = M3U_loadTracks(playlistId, tracks.get(), PLAYLIST_MAX_TRACKS);
        count            = loaded > 0 ? loaded : 0;
    }

    int getCount() const override { return count; }
    long getItemId(int index) const override { return tracks[index].file_id; }

    const PlaylistTrack* get(int index) const { return index >= 0 && index < count ? &tracks[index] : nullptr; }
    const PlaylistTrack* getAll() const { return tracks.get(); }

    std::shared_ptr<ListItemView> getView(int index) override {
        const PlaylistTrack& track = tracks[index];
        auto row                   = std::make_shared<ListRowView>(Fonts_getMedium());
        row->setText(track.name);
        if (Icons_isLoaded()) {
            row->setIcon(Icons_getForFormat(track.format, THEME_ROLE_PRIMARY, false),
                         Icons_getForFormat(track.format, THEME_ROLE_PRIMARY, true));
        }
        return row;
    }

private:
    std::unique_ptr<PlaylistTrack[]> tracks = std::make_unique<PlaylistTrack[]>(PLAYLIST_MAX_TRACKS);
    int count                               = 0;
};

class PlaylistTracksPageImpl final : public UiPageImpl<PlaylistTracksPage>, private ListScreenListener {
public:
    PlaylistTracksPageImpl(int playlistId, std::string name) : UiPageImpl(PageKind::FullScreen), playlistId(playlistId), name(std::move(name)) {
        screenView.getTitleView().setText("Playlist " + this->name);
        screenView.getTitleView().setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        screenView.getButtonHintsView().setHints(kLeftHints, kRightHints);
        emptyState.setMessage("No tracks in playlist");
        emptyState.setLine("Add tracks from the music browser");
        screenView.setEmptyState(&emptyState);
    }

    std::span<const ButtonRequest> getButtonRequests() const override {
        return ListScreenController::getButtonRequests();
    }

    HelpId getHelpId() const override { return HELP_PLAYLIST_DETAIL; }
    ThemeSource getSource() const override { return THEME_SOURCE_MUSIC; }

    void onEnter() override {
        tracks.reload(playlistId);
        screenView.setAdapter(tracks);
        list.start();
    }

    /** The player can remove nothing, but the playlist can change on the card: read it again */
    void resume() override { reloadKeepingCursor(); }

    void layout(UiLayoutContext& context) override {
        screenView.setSidePanel(Theme_isSourceColoursEnabled() ? &panel : nullptr);
        context.layoutChild(screenView, context.getMaxRect());
    }

    bool onButtonEvent(const ButtonEvent& event) override {
        if (list.onButtonEvent(event)) return true;
        if (event.isSingleButton(Button::A)) {
            playFromSelected();
            return true;
        }
        if (event.isSingleButton(Button::B)) {
            close();
            return true;
        }
        if (event.isSingleButton(Button::X)) {
            confirmRemoval();
            return true;
        }
        return false;
    }

private:
    void onSelectionChanged(int index) override { updatePanel(index); }

    void updatePanel(int index) {
        const PlaylistTrack* track = tracks.get(index);
        if (!track) return;
        panel.getArt().setFallback(track->name, THEME_SOURCE_MUSIC);
        panel.getInfo().setLine(name);
        panel.getInfo().setPosition("Track " + std::to_string(index + 1) + " of " + std::to_string(tracks.getCount()));
    }

    void playFromSelected() {
        const int index = list.getSelectedIndex();
        if (!tracks.get(index)) return;
        if (MusicSession_startPlaylist(tracks.getAll(), tracks.getCount(), index, playlistId)) {
            getNavigator().open(NowPlayingPage{});
        }
    }

    void confirmRemoval() {
        const PlaylistTrack* track = tracks.get(list.getSelectedIndex());
        if (!track) return;
        const int fileId = track->file_id;
        getNavigator().openForResult(ConfirmDialogPage("Remove Track?", track->name), [this, fileId](const bool& yes) {
            if (!yes) return;
            M3U_removeTrack(playlistId, fileId);
            Toast_show("Track removed", TOAST_DURATION);
            reloadKeepingCursor();
        });
    }

    /** Reads the tracks again. The cursor keeps its position, or goes to the last row. */
    void reloadKeepingCursor() {
        const int position = list.getSelectedIndex();
        tracks.reload(playlistId);
        list.notifyDataChanged();
        if (position >= 0) list.setSelectedIndex(position);
        updatePanel(list.getSelectedIndex());
    }

    int playlistId;
    std::string name;
    TrackListAdapter tracks;
    ListScreenView screenView{Fonts_getLarge()};
    EmptyStateView emptyState;
    DetailPanelView panel;
    ListScreenController list{screenView, getScheduler(), *this};
};

}  // namespace

std::unique_ptr<UiPageImplBase> PlaylistTracksPage::createPageImpl() const {
    return std::make_unique<PlaylistTracksPageImpl>(playlistId, name);
}

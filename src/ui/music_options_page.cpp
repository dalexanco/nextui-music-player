#include "ui/music_options_page.hpp"

#include <string>
#include <variant>

#include "album_art.h"
#include "music_session.h"
#include "player.h"
#include "settings.h"
#include "sleep_timer.h"
#include "ui/add_to_playlist_page.hpp"
#include "ui/page/list_screen_controller.hpp"
#include "ui/view/art_fade_view.hpp"
#include "ui/view/box_view.hpp"
#include "ui/view/options_header_view.hpp"
#include "ui/view/options_panel_view.hpp"
#include "ui/view/screen_view.hpp"
#include "ui_fonts.h"
#include "ui_theme.h"

// The NextUI headers declare C functions and have no guard of their own
extern "C" {
#include "defines.h"
#include "api.h"
}

using namespace input;

namespace {

/** The rows of the panel, in order */
enum class Row : int { Shuffle, Repeat, Lyrics, SleepTimer, AddToPlaylist, Count };

constexpr const char* kLeftHints[]  = {"LEFT/RIGHT", "CHANGE", nullptr};
constexpr const char* kRightHints[] = {"B", "CLOSE", "A", "CHOOSE", nullptr};

std::string getOnOff(bool on) { return on ? "On" : "Off"; }

std::string getSleepValue() {
    const int minutes = MusicSession_getSleepTimer();
    return minutes > 0 ? std::to_string(minutes) + " min" : "Off";
}

class MusicOptionsPageImpl final : public UiPageImpl<MusicOptionsPage> {
public:
    MusicOptionsPageImpl()
        : UiPageImpl(PageKind::FullScreen) {
        screenView.getButtonHintsView().setHints(kLeftHints, kRightHints);
        // The dim over the player that shows behind the panel: 62% of the page color of the board
        SDL_Color dim = Theme_getColor(THEME_ROLE_PAGE_BACKGROUND, false);
        dim.a         = 158;
        dimView.setColor(dim);
        panel.setScope("TRACK");
    }

    std::span<const ButtonRequest> getButtonRequests() const override {
        return ListScreenController::getButtonRequests();
    }

    HelpId getHelpId() const override { return HELP_PLAYER_OPTIONS; }
    ThemeSource getSource() const override { return THEME_SOURCE_MUSIC; }

    void onEnter() override {
        if (const TrackInfo* info = Player_getTrackInfo()) {
            header.setMeta(info->artist[0] ? info->artist : "Unknown Artist");
            header.setTitle(info->title[0] ? info->title : "Unknown Title");
        }
        artView.setArt(album_art_is_fetching() ? nullptr : Player_getAlbumArt());
        panel.setRows(readRows());
    }

    void layout(UiLayoutContext& context) override {
        const UiSize screen = context.getMaxSize();
        context.layoutChild(artView, SDL_Rect{screen.w - screen.w * 620 / 1024, 0, screen.w * 620 / 1024, screen.h});
        context.layoutChild(dimView, context.getMaxRect());
        context.layoutChild(screenView, context.getMaxRect());
        context.layoutChild(header, context.getMaxRect());
        context.layoutChild(panel, context.getMaxRect());
    }

    void resume() override { panel.setRows(readRows()); }

    bool onButtonEvent(const ButtonEvent& event) override {
        const auto* shortPress = std::get_if<event::ShortPress>(&event.kind);
        const auto* repeat     = std::get_if<event::Repeat>(&event.kind);
        if (!shortPress && !repeat) return false;
        const Button button = shortPress ? shortPress->button : repeat->button;
        switch (button) {
            case Button::DpadUp:    panel.moveSelection(-1); return true;
            case Button::DpadDown:  panel.moveSelection(1); return true;
            case Button::DpadLeft:  change(-1); return true;
            case Button::DpadRight: change(1); return true;
            case Button::A:
                if (!shortPress) return true;
                choose();
                return true;
            case Button::B:
                if (shortPress) close();
                return true;
            default: return false;
        }
    }

private:
    std::vector<OptionRow> readRows() const {
        return {
            {"Shuffle", getOnOff(MusicSession_isShuffle())},
            {"Repeat", getOnOff(MusicSession_isRepeat())},
            {"Lyrics", getOnOff(Settings_getBool(&SETTING_LYRICS_ENABLED))},
            {"Sleep timer", getSleepValue()},
            {"Add to playlist", ""},
        };
    }

    /** Left and Right: changes the value of the selected row by one step. */
    bool change(int direction) {
        const int index = panel.getSelectedIndex();
        switch (static_cast<Row>(index)) {
            case Row::Shuffle:
                MusicSession_toggleShuffle();
                panel.setValue(index, getOnOff(MusicSession_isShuffle()));
                return true;
            case Row::Repeat:
                MusicSession_toggleRepeat();
                panel.setValue(index, getOnOff(MusicSession_isRepeat()));
                return true;
            case Row::Lyrics:
                MusicSession_toggleLyrics();
                panel.setValue(index, getOnOff(Settings_getBool(&SETTING_LYRICS_ENABLED)));
                return true;
            case Row::SleepTimer:
                MusicSession_setSleepTimer(SleepTimer_stepMinutes(MusicSession_getSleepTimer(), direction));
                panel.setValue(index, getSleepValue());
                return true;
            case Row::AddToPlaylist:
            case Row::Count:         break;
        }
        return false;
    }

    /** A: a value row moves to its next value, and the action row runs. */
    void choose() {
        if (static_cast<Row>(panel.getSelectedIndex()) != Row::AddToPlaylist) {
            change(1);
            return;
        }
        const PlaylistTrack* track = MusicSession_getTrack(MusicSession_getCurrentIndex());
        if (track) getNavigator().open(AddToPlaylistPage::forFile(track->file_id));
    }

    ArtFadeView artView;
    BoxView dimView;
    ScreenView screenView{Fonts_getLarge()};
    OptionsHeaderView header;
    OptionsPanelView panel;
};

}  // namespace

std::unique_ptr<UiPageImplBase> MusicOptionsPage::createPageImpl() const {
    return std::make_unique<MusicOptionsPageImpl>();
}

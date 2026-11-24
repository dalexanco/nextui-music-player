#include "ui/main_menu_page.hpp"

#include <array>
#include <initializer_list>
#include <memory>
#include <string>
#include <vector>

#include "background.h"
#include "player.h"
#include "error.h"
#include "resume.h"
#include "selfupdate.h"
#include "toast.h"
#include "ui/library_page.hpp"
#include "ui/now_playing_page.hpp"
#include "ui/page/list_screen_controller.hpp"
#include "ui/podcast_page.hpp"
#include "ui/radio_page.hpp"
#include "ui/resume_page.hpp"
#include "ui/settings_page.hpp"
#include "ui/view/detail_panel_view.hpp"
#include "ui/view/list_adapter.hpp"
#include "ui/view/list_row_view.hpp"
#include "ui/view/list_screen_view.hpp"
#include "ui_fonts.h"
#include "ui_main.h"
#include "ui_theme.h"
#include "ui_utils.h"

using namespace input;

namespace ui {
namespace {

/**
 * How long the exit prompt stays up, which is also how long the confirming press has to
 * arrive: the prompt being on screen is the armed state.
 */
constexpr uint32_t kExitConfirmationWindowMs = 3000;

/**
 * How long the "Exiting..." toast shows before the app quits. The last frame of the app stays on
 * the screen until NextUI draws its first frame, thus the toast must reach the screen.
 */
constexpr uint32_t kExitToastDelayMs = 100;

/** How often Main reads its rows and their labels again. Both can change with no input. */
constexpr uint32_t kRowRefreshMs = 3000;

/** An item of the main menu. */
enum class MenuItem : int8_t {
    None = -1,
    NowPlaying,
    Resume,
    Library,
    Radio,
    Podcast,
    Settings,
};

/** The count of the items of the menu, each a value of MenuItem from 0. */
constexpr size_t kMenuItemCount = static_cast<size_t>(MenuItem::Settings) + 1;

constexpr const char* kLeftHints[]  = {"START", "CONTROLS", nullptr};
constexpr const char* kRightHints[] = {"B", "EXIT", "A", "OPEN", nullptr};

/** The exit prompt, while it is showing. TOAST_TOKEN_NONE = not armed. */
ToastToken exitPrompt = TOAST_TOKEN_NONE;

/** State of the playback row. Now Playing wins over Resume when both apply. */
MenuItem getPlayingItem() {
    if (Background_isPlaying()) return MenuItem::NowPlaying;
    if (Resume_isAvailable()) return MenuItem::Resume;
    return MenuItem::None;
}

/** Returns the source of the playback that the Now Playing row names. */
const char* getNowPlayingSource() {
    switch (Background_getActive()) {
        case BG_MUSIC:   return "Music";
        case BG_RADIO:   return "Radio";
        case BG_PODCAST: return "Podcast";
        default:         return "Audio";
    }
}

/** Returns the label of the row of item. */
std::string getRowLabel(MenuItem item) {
    switch (item) {
        case MenuItem::NowPlaying: return std::string("Now Playing: ") + getNowPlayingSource();
        case MenuItem::Resume:
            if (const char* label = Resume_getLabel()) return label;
            return "Resume";
        case MenuItem::Library: return "Library";
        case MenuItem::Radio:   return "Online Radio";
        case MenuItem::Podcast: return "Podcasts";
        case MenuItem::Settings:
            return SelfUpdate_getStatus().update_available ? "Settings (Update available)" : "Settings";
        case MenuItem::None:    return "";
    }
    return "";
}

/** Returns the source whose accent the preview of item takes. */
ThemeSource getItemSource(MenuItem item) {
    switch (item) {
        case MenuItem::NowPlaying:
            switch (Background_getActive()) {
                case BG_RADIO:   return THEME_SOURCE_RADIO;
                case BG_PODCAST: return THEME_SOURCE_PODCAST;
                default:         return THEME_SOURCE_MUSIC;
            }
        case MenuItem::Resume:
        case MenuItem::Library:  return THEME_SOURCE_MUSIC;
        case MenuItem::Radio:    return THEME_SOURCE_RADIO;
        case MenuItem::Podcast:  return THEME_SOURCE_PODCAST;
        case MenuItem::Settings:
        case MenuItem::None:     break;
    }
    return THEME_SOURCE_NEUTRAL;
}

/** Returns "m:ss" for a time in milliseconds. */
std::string formatTime(int ms) {
    const int seconds = ms > 0 ? ms / 1000 : 0;
    return std::to_string(seconds / 60) + ":" + (seconds % 60 < 10 ? "0" : "") + std::to_string(seconds % 60);
}

/** Fills the preview panel for item: a tile in the accent of its source, then one line of context. */
void fillPreview(DetailPanelView& panel, MenuItem item, int position, int count) {
    const ThemeSource source = getItemSource(item);
    std::string title        = getRowLabel(item);
    std::string line;
    std::string times;
    int percent = -1;
    switch (item) {
        case MenuItem::NowPlaying: {
            const TrackInfo* info = Background_getActive() == BG_MUSIC ? Player_getTrackInfo() : nullptr;
            if (info && info->title[0]) {
                title = info->title;
                line  = info->artist;
                if (info->duration_ms > 0) {
                    percent = Player_getPosition() * 100 / info->duration_ms;
                    times   = formatTime(Player_getPosition()) + " / " + formatTime(info->duration_ms);
                }
            } else {
                line = getNowPlayingSource();
            }
            break;
        }
        case MenuItem::Resume: {
            const ResumeState* state = Resume_getState();
            title                    = state ? state->track_name : "Resume";
            line                     = "Resume where you stopped";
            if (state) times = formatTime(state->position_ms);
            break;
        }
        case MenuItem::Library:  title = "Library"; line = "Files, playlists and downloads"; break;
        case MenuItem::Radio:    title = "Online Radio"; line = "Internet stations"; break;
        case MenuItem::Podcast:  title = "Podcasts"; line = "Subscriptions and episodes"; break;
        case MenuItem::Settings: title = "Settings"; line = "Screen, sound and updates"; break;
        case MenuItem::None:     break;
    }
    panel.getArt().setFallback(title, source);
    panel.getInfo().setLine(line);
    panel.getInfo().setProgress(percent);
    panel.getInfo().setPosition("Item " + std::to_string(position + 1) + " of " + std::to_string(count));
    panel.getInfo().setTimes(times);
}

/** The rows of the main menu, and the view of each row. */
class MainMenuListAdapter final : public ListAdapter {
public:
    /**
     * Sets the items of the rows, in order.
     * Returns: true when the rows changed
     */
    bool setRows(const std::vector<MenuItem> &newRows) {
        if (newRows == rows) return false;
        rows = newRows;
        return true;
    }

    /** Returns the item of the row at index, or None for a row that is not there. */
    MenuItem getItem(int index) const {
        if (index < 0 || index >= getCount()) return MenuItem::None;
        return rows[static_cast<size_t>(index)];
    }

    int getCount() const override { return static_cast<int>(rows.size()); }

    long getItemId(int index) const override { return static_cast<long>(getItem(index)); }

    /** Each item keeps its view while the menu runs, thus the Resume row keeps its scrolling. */
    std::shared_ptr<ListItemView> getView(int index) override {
        std::shared_ptr<ListRowView>& row = rowViews[static_cast<size_t>(getItem(index))];
        if (!row) row = std::make_shared<ListRowView>(Fonts_getMedium());
        return row;
    }

private:
    std::vector<MenuItem> rows;
    std::array<std::shared_ptr<ListRowView>, kMenuItemCount> rowViews;
};

class MainMenuPageImpl final : public UiPageImpl<MainMenuPage>, private ListScreenListener {
public:
    MainMenuPageImpl() : UiPageImpl(PageKind::FullScreen) {
        screenView.getTitleView().setText("Music Player");
        screenView.getTitleView().setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        screenView.getButtonHintsView().setHints(kLeftHints, kRightHints);
    }

    std::span<const ButtonRequest> getButtonRequests() const override {
        return ListScreenController::getButtonRequests();
    }

    HelpId getHelpId() const override { return HELP_MAIN_MENU; }

    /** With source colours, the page takes the accent of the entry under the cursor */
    ThemeSource getSource() const override { return getItemSource(selectedItem); }

    void onEnter() override {
        items.setRows(getRows());
        screenView.setAdapter(items);
        updateLabels();
        list.start();
        getScheduler().postRepeating([this] { refreshRows(); }, kRowRefreshMs);
        Error_showToast();
    }

    void layout(UiLayoutContext& context) override {
        // The panel belongs to the source colours look, and the setting can change between frames
        screenView.setSidePanel(Theme_isSourceColoursEnabled() ? &panel : nullptr);
        context.layoutChild(screenView, context.getMaxRect());
    }

    /** A legacy module can change the playback, and with it the rows and their labels */
    void resume() override { refreshRows(); }

    /** While the "Exiting..." toast shows, no button acts */
    bool onRawEvent(const RawEvent&) override { return exiting; }

    bool onButtonEvent(const ButtonEvent& event) override {
        if (exiting) return true;
        if (list.onButtonEvent(event)) return true;
        if (const auto* typedEvent = std::get_if<event::ShortPress>(&event.kind)) {
            return pressButton(typedEvent->button);
        }
        if (const auto* typedEvent = std::get_if<event::Repeat>(&event.kind)) {
            return pressButton(typedEvent->button);
        }
        return false;
    }

private:
    void onSelectionChanged(int listItemPosition) override {
        selectedItem = items.getItem(listItemPosition);
        updatePanel();
    }

    /** Fills the preview of the focused entry. It also follows the track of a Now Playing row. */
    void updatePanel() {
        if (selectedItem == MenuItem::None) return;
        fillPreview(panel, selectedItem, list.getSelectedIndex(), items.getCount());
    }

    void refreshRows() {
        updateRowMap();
        updateLabels();
        updatePanel();
        Error_showToast();
    }

    /** Returns the items of the rows, in order. */
    static std::vector<MenuItem> getRows() {
        std::vector<MenuItem> rows = {MenuItem::Library, MenuItem::Radio, MenuItem::Podcast, MenuItem::Settings};
        const MenuItem playingItem = getPlayingItem();
        if (playingItem != MenuItem::None) rows.insert(rows.begin(), playingItem);
        return rows;
    }

    /** Builds the rows, and sets the labels of new rows. */
    void updateRowMap() {
        if (!items.setRows(getRows())) return;
        list.notifyDataChanged();
        updateLabels();
    }

    /** Sets the label of each row. A new label of the selected row restarts the wait of its text. */
    void updateLabels() {
        const int selectedIndex = list.getSelectedIndex();
        for (int index = 0; index < list.getItemCount(); index++) {
            const bool changed = getRow(index).setText(getRowLabel(items.getItem(index)));
            if (changed && index == selectedIndex) list.restartRowScrolling();
        }
    }

    ListRowView& getRow(int index) { return static_cast<ListRowView&>(list.getItemView(index)); }

    /** Returns: true when the button has an action on the menu */
    bool pressButton(Button button) {
        switch (button) {
            case Button::A: openItem(); return true;
            case Button::X: clearPlayingItem(); return true;
            case Button::B: askExit(); return true;
            default:        return false;
        }
    }

    void openItem() {
        switch (selectedItem) {
            case MenuItem::NowPlaying: openNowPlaying(); break;
            case MenuItem::Resume:     getNavigator().open(ResumePage{}); break;
            case MenuItem::Library:    getNavigator().open(LibraryPage{}); break;
            case MenuItem::Radio:      getNavigator().open(RadioPage{}); break;
            case MenuItem::Podcast:    getNavigator().open(PodcastPage{}); break;
            case MenuItem::Settings:   getNavigator().open(SettingsPage{}); break;
            case MenuItem::None:       break;
        }
    }

    /** The radio opens its station list, which opens the playing screen at once */
    void openNowPlaying() {
        if (Background_getActive() == BG_RADIO) {
            getNavigator().open(RadioPage{});
        } else {
            getNavigator().open(NowPlayingPage{});
        }
    }

    /** Stops the playback or forgets the Resume record of the row of the cursor */
    void clearPlayingItem() {
        if (selectedItem == MenuItem::NowPlaying) {
            Background_stopAll();
            updateRowMap();
        } else if (selectedItem == MenuItem::Resume) {
            Resume_clear();
            updateRowMap();
        }
    }

    void askExit() {
        if (Toast_isShowing(exitPrompt)) {
            // Unbound: it has to outlive the menu as the last frame on the screen
            Toast_show("Exiting...", TOAST_DURATION);
            exiting = true;
            getScheduler().postDelayed([this] { getNavigator().exit(); }, kExitToastDelayMs);
        } else {
            exitPrompt = Toast_showScreenBound("Press B again to exit", kExitConfirmationWindowMs);
        }
    }

    MainMenuListAdapter items;
    /** The "Exiting..." toast shows before the quit */
    bool exiting = false;

    ListScreenView screenView{Fonts_getLarge()};
    DetailPanelView panel;
    ListScreenController list{screenView, getScheduler(), *this};
    /** The item of the row of the cursor */
    MenuItem selectedItem = MenuItem::None;
};

}  // namespace
}  // namespace ui

std::unique_ptr<UiPageImplBase> MainMenuPage::createPageImpl() const {
    return std::make_unique<ui::MainMenuPageImpl>();
}

#include "ui/radio_playing_page.hpp"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <type_traits>

#include "album_art.h"
#include "background.h"
#include "module_common.h"
#include "radio.h"
#include "radio_player_state.hpp"
#include "radio_sleep.h"
#include "ui/music_player_layout.hpp"
#include "ui/now_playing_page.hpp"
#include "ui/radio_options_page.hpp"
#include "ui/radio_stations_page.hpp"
#include "ui/view/art_fade_view.hpp"
#include "ui/view/box_view.hpp"
#include "ui/view/chip_view.hpp"
#include "ui/view/progress_bar_view.hpp"
#include "ui/view/screen_view.hpp"
#include "ui/view/text_view.hpp"
#include "ui_fonts.h"
#include "ui_theme.h"
#include "ui_utils.h"

// The NextUI headers declare C functions and have no guard of their own
extern "C" {
#include "defines.h"
#include "api.h"
}

using namespace input;

namespace {

constexpr const char* kLeftHints[]         = {"Y", "OPTIONS", nullptr};
constexpr const char* kStopHints[]         = {"X", "STATIONS", "A", "STOP", nullptr};
constexpr const char* kCancelHints[]       = {"X", "STATIONS", "A", "CANCEL", nullptr};
constexpr const char* kListenHints[]       = {"X", "STATIONS", "A", "LISTEN", nullptr};
constexpr const char* kRetryHints[]        = {"X", "STATIONS", "A", "RETRY", nullptr};

/** Where the screen is: on, showing the wake hint before it goes dark, or dark */
enum class ScreenMode : uint8_t { On, Hint, Off };

const char* const* getRightHints(RadioView view) {
    switch (view) {
        case RadioView::Live:        return kStopHints;
        case RadioView::Connecting:
        case RadioView::Rebuffering: return kCancelHints;
        case RadioView::Paused:      return kListenHints;
        case RadioView::Offline:     return kRetryHints;
    }
    return kStopHints;
}

/** The playing screen of the radio, for the page that opens it (`Page`) */
template <class Page>
class RadioPlayerPageImpl final : public UiPageImpl<Page> {
public:
    explicit RadioPlayerPageImpl(std::string url)
        : UiPageImpl<Page>(PageKind::FullScreen),
          initialUrl(std::move(url)),
          counterView(Fonts_getSmall(), true),
          metaView(Fonts_getMedium(), true),
          titleView(Fonts_getHeading(), false),
          subView(Fonts_getMedium(), true),
          detailView(Fonts_getSmall(), true),
          listeningView(Fonts_getSmall(), true),
          bufferView(Fonts_getSmall(), false),
          wakeHintView(Fonts_getMedium(), false) {
        screenView.getButtonHintsView().setHints(kLeftHints, kStopHints);
        counterView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        metaView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        titleView.setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
        subView.setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
        detailView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        listeningView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        bufferView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        // The screen goes dark: the colors are the platform's black and white, whatever the theme is
        darkView.setColor(SDL_Color{0, 0, 0, 255});
        wakeHintView.setColor(SDL_Color{255, 255, 255, 255});
        wakeHintView.setText("Press SELECT + A to wake screen");
        artView.setSource(THEME_SOURCE_RADIO);
        radioChip.setText("RADIO");
        radioChip.setOutline();
    }

    std::span<const ButtonRequest> getButtonRequests() const override { return {}; }

    HelpId getHelpId() const override { return HELP_RADIO_PLAYING; }
    ThemeSource getSource() const override { return THEME_SOURCE_RADIO; }

    void onEnter() override {
        takeStation(initialUrl);
        if (getScreenUrl().empty()) {
            // No station to show
            this->finish(makeResult({}));
            return;
        }
        ModuleCommon_resetScreenOffHint();
        ModuleCommon_recordInputTime();
        ModuleCommon_setAutosleepDisabled(true);
        refreshAll();
    }

    void onExit() override {
        if (mode == ScreenMode::Off) PLAT_enableBacklight(1);
        // The keys lock the playback only
        ModuleCommon_resetScreenOffHint();
        // A quit stops the station, as it stopped it from the legacy screen
        if (ModuleCommon_isQuitRequested()) Radio_stop();
    }

    void resume() override { refreshAll(); }

    void layout(UiLayoutContext& context) override {
        const UiSize screen     = context.getMaxSize();
        const PlayerRects rects = layoutPlayer(screen.w, screen.h, SCALE1(PADDING), SCALE1(BUTTON_PADDING),
                                               SCALE1(PILL_SIZE), SCALE1(PILL_SIZE) / 2);
        context.layoutChild(artView, rects.art);
        // The status pill and the button hints sit over the art
        context.layoutChild(screenView, context.getMaxRect());

        layoutTopRow(context, rects);
        layoutTextColumn(context, rects);
        layoutProgress(context, rects);
        // A station that waits starts after one frame that says that it connects
        if (!pendingUrl.empty() && pendingFramesLaid < 2) pendingFramesLaid++;

        if (mode != ScreenMode::On) {
            context.layoutChild(darkView, context.getMaxRect());
            if (mode == ScreenMode::Hint) {
                UiLayoutContext measuring = context.createMeasuringContext();
                const UiSize size         = measuring.layoutChild(wakeHintView, context.getMaxRect()).getSize();
                context.layoutChild(wakeHintView, SDL_Rect{(screen.w - size.w) / 2, (screen.h - size.h) / 2, size.w, size.h});
            }
        }
    }

    bool onRawEvent(const RawEvent& event) override {
        if (!isButtonEvent(event)) return false;
        if (mode == ScreenMode::Off) {
            // Any press wakes the backlight and shows the hint, as the legacy screen did
            if (isButtonPressedEvent(event)) {
                PLAT_enableBacklight(1);
                ModuleCommon_startScreenOffHint();
                mode = ScreenMode::Hint;
            }
            return true;
        }
        if (mode == ScreenMode::Hint) {
            if (isButtonPressedEvent(event)) ModuleCommon_startScreenOffHint();
            return true;
        }
        if (isButtonPressedEvent(event)) ModuleCommon_recordInputTime();
        return false;
    }

    void onTick() override {
        RadioSleep_tick();
        ModuleCommon_setAutosleepDisabled(true);

        if (mode == ScreenMode::Hint) {
            // SELECT + A wakes the screen
            if (PAD_isPressed(BTN_SELECT) && PAD_isPressed(BTN_A)) {
                ModuleCommon_resetScreenOffHint();
                ModuleCommon_recordInputTime();
                mode = ScreenMode::On;
            } else if (ModuleCommon_processScreenOffHintTimeout()) {
                mode = ScreenMode::Off;
            }
        } else if (mode == ScreenMode::On) {
            if (Radio_getState() == RADIO_STATE_PLAYING && ModuleCommon_checkAutoScreenOffTimeout()) {
                mode = ScreenMode::Hint;
            }
        }

        if (!pendingUrl.empty()) {
            if (Radio_isActive()) {
                // Another path started the radio while the station waited
                cancelPending();
            } else if (pendingFramesLaid >= 2 || mode != ScreenMode::On) {
                // `Radio_play()` holds this thread for the whole of the connection. The screen that
                // says "connecting" reached the display in the frames before. A screen that draws
                // nothing has no frame to wait for.
                const std::string url = pendingUrl;
                Radio_play(url.c_str());
                cancelPending();
            }
        }
        Radio_update();
        refreshAll();
    }

    bool onButtonEvent(const ButtonEvent& event) override {
        if (mode != ScreenMode::On || ModuleCommon_isKeyLocked()) return true;
        const auto* press = std::get_if<event::ShortPress>(&event.kind);
        if (!press) return false;
        switch (press->button) {
            case Button::A:         toggleStation(); return true;
            case Button::B:         leave(); return true;
            case Button::X:         openStations(); return true;
            case Button::Y:         openOptions(); return true;
            case Button::DpadUp:
            case Button::R1:        stepStation(true); return true;
            case Button::DpadDown:
            case Button::L1:        stepStation(false); return true;
            case Button::Select:
                ModuleCommon_startScreenOffHint();
                // A screen that keeps its content locks the keys in place of the wake hint
                if (ModuleCommon_isScreenOffHintActive()) mode = ScreenMode::Hint;
                return true;
            default: return false;
        }
    }

private:
    static typename Page::Result makeResult(const std::string& shownUrl) {
        if constexpr (std::is_same_v<typename Page::Result, std::string>) {
            return shownUrl;
        } else {
            return {};
        }
    }

    /** The station that the screen shows: the station that waits, else the station of the radio. */
    std::string getScreenUrl() const {
        if (!pendingUrl.empty()) return pendingUrl;
        const char* current = Radio_getCurrentUrl();
        return current ? current : "";
    }

    static const RadioStation* findStation(const std::string& url, int* index = nullptr) {
        RadioStation* stations;
        const int count = Radio_getStations(&stations);
        for (int i = 0; i < count; i++) {
            if (url != stations[i].url) continue;
            if (index) *index = i;
            return &stations[i];
        }
        return nullptr;
    }

    void cancelPending() {
        pendingUrl.clear();
        pendingFramesLaid = 0;
    }

    /** Takes the old station down and gives `url` to the screen as the station that waits. */
    void handStationToScreen(const std::string& url) {
        Radio_stop();
        Radio_clearMetadata();
        pendingUrl        = url;
        pendingFramesLaid = 0;
        wasHealthy        = false;
    }

    /** Starts `url`, or takes back the station that plays when it is `url` or when `url` is empty. */
    void takeStation(const std::string& url) {
        cancelPending();
        const char* current = Radio_getCurrentUrl();
        const bool plays    = Radio_isActive();
        if (url.empty() || (plays && current && url == current)) {
            // `Background_stopAll()` would stop this very station
            if (plays && Background_getActive() == BG_RADIO) Background_setActive(BG_NONE);
            wasHealthy = plays;
            return;
        }
        Background_stopAll();
        handStationToScreen(url);
    }

    void stepStation(bool next) {
        RadioStation* stations;
        const int count = Radio_getStations(&stations);
        const std::string current = getScreenUrl();
        const int index           = Radio_pickStation(stations, count, current.empty() ? nullptr : current.c_str(), next);
        if (index >= 0) handStationToScreen(stations[index].url);
    }

    /** A: a station that waits is given up, a station that plays stops, a station that stopped starts again. */
    void toggleStation() {
        if (!pendingUrl.empty()) {
            cancelPending();
            Radio_clearMetadata();
        } else if (Radio_isActive()) {
            Radio_stop();
        } else {
            const std::string current = getScreenUrl();
            if (!current.empty()) handStationToScreen(current);
        }
    }

    /** B: a station that plays goes on in the background. */
    void leave() {
        const bool plays          = Radio_isActive();
        const std::string shown   = getScreenUrl();
        cancelPending();
        if (plays) {
            Background_setActive(BG_RADIO);
        } else {
            ModuleCommon_setAutosleepDisabled(false);
        }
        this->finish(makeResult(shown));
    }

    void openStations() {
        this->getNavigator().open(makeRadioStationsPage([this] { return getScreenUrl(); },
                                                        [this](const std::string& url) { handStationToScreen(url); }));
    }

    void openOptions() {
        const std::string url = getScreenUrl();
        const RadioStation* station = findStation(url);
        this->getNavigator().open(RadioOptionsPage::forPlayer(url, station ? station->name : "", station ? station->genre : ""));
    }

    RadioView readView() const {
        return getRadioView(RadioViewInput{Radio_getState(), !pendingUrl.empty(), Radio_getBufferLevel(), wasHealthy});
    }

    /** Reads the radio again: the look, the texts, the art and the hints. */
    void refreshAll() {
        const RadioState state = Radio_getState();
        const float buffer     = Radio_getBufferLevel();
        if (pendingUrl.empty()) {
            if (isRadioStreamHealthy(state, buffer)) {
                wasHealthy = true;
            } else if (state != RADIO_STATE_BUFFERING) {
                wasHealthy = false;
            }
        }
        view = readView();
        if (view == RadioView::Live && previousView != RadioView::Live) listenStart = SDL_GetTicks();
        previousView = view;

        refreshTexts();
        refreshProgress(buffer);
        refreshHints();
        const bool fetching = album_art_is_fetching();
        // The surface of the art is not safe to read while a fetch runs
        artView.setArt(fetching ? nullptr : Radio_getAlbumArt());
    }

    void refreshTexts() {
        const std::string url       = getScreenUrl();
        int index                   = -1;
        const RadioStation* station = findStation(url, &index);
        const RadioMetadata* meta   = Radio_getMetadata();
        RadioStation* all;
        const int total = Radio_getStations(&all);

        counterView.setText(station ? formatTrackCounter(index + 1, total) : "");
        metaView.setText(station && station->genre[0] ? station->genre : "Radio");
        const char* name = meta->station_name[0] ? meta->station_name : (station ? station->name : "Unknown Station");
        const bool titleChanged = titleView.setText(name);
        if (titleChanged) restartTitleScroll();

        const bool bad = view == RadioView::Offline;
        subView.setColor(Theme_getColor(bad ? THEME_ROLE_STATUS_ERROR : THEME_ROLE_PRIMARY, false));
        if (view == RadioView::Live) {
            // The title of the song, then its artist or else the slogan of the station
            subView.setText(meta->title[0] ? meta->title : (station ? station->slogan : ""));
            detailView.setText(meta->title[0] ? meta->artist : "");
        } else {
            subView.setText(getRadioStatusLine(view, view == RadioView::Offline ? Radio_getError() : nullptr));
            detailView.setText("");
        }
        const int bitrate = meta->bitrate;
        bitrateChip.setText(bitrate > 0 ? std::to_string(bitrate) + " kbps" : "");
        showBitrate = bitrate > 0;
    }

    void refreshProgress(float buffer) {
        stateChip.setText(getRadioChipText(view));
        if (view == RadioView::Offline) {
            // An outline in the error color reads on both backgrounds
            stateChip.setOutline(Theme_getColor(THEME_ROLE_STATUS_ERROR, false));
        } else {
            stateChip.setPill();
            stateChip.setTextColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
        }
        if (view == RadioView::Live) {
            listeningView.setText(formatListening(SDL_GetTicks() - listenStart));
        } else if (view == RadioView::Offline) {
            bar.setPercent(0);
            bufferView.setText("Failed");
        } else {
            bar.setPercent(getRadioBufferPercent(buffer));
            bufferView.setText("Buffer " + std::to_string(getRadioBufferPercent(buffer)) + " %");
        }
    }

    void refreshHints() {
        if (shownHintsView == view) return;
        shownHintsView = view;
        screenView.getButtonHintsView().setHints(kLeftHints, getRightHints(view));
    }

    void restartTitleScroll() {
        titleView.stopScrolling();
        titleView.resetScrolling();
        this->getScheduler().cancelTask(titleStart);
        titleStart = this->getScheduler().postDelayed([this] { titleView.startScrolling(); }, SCROLL_START_DELAY);
    }

    void layoutTopRow(UiLayoutContext& context, const PlayerRects& rects) {
        UiLayoutContext measuring = context.createMeasuringContext();
        const UiSize chipSize     = measuring.layoutChild(radioChip, rects.topRow).getSize();
        const int y               = rects.topRow.y + (rects.topRow.h - chipSize.h) / 2;
        context.layoutChild(radioChip, SDL_Rect{rects.topRow.x, y, chipSize.w, chipSize.h});
        const UiSize counterSize = measuring.layoutChild(counterView, rects.topRow).getSize();
        context.layoutChild(counterView, SDL_Rect{rects.topRow.x + chipSize.w + SCALE1(10),
                                                  rects.topRow.y + (rects.topRow.h - counterSize.h) / 2, counterSize.w,
                                                  counterSize.h});
    }

    /** Stacks the genre, the station, the song, its artist and the chip of the bitrate. */
    void layoutTextColumn(UiLayoutContext& context, const PlayerRects& rects) {
        const SDL_Rect column     = rects.textColumn;
        UiLayoutContext measuring = context.createMeasuringContext();
        const int gap             = SCALE1(6);
        int y                     = column.y;
        for (UiView* textView : {static_cast<UiView*>(&metaView), static_cast<UiView*>(&titleView),
                                 static_cast<UiView*>(&subView), static_cast<UiView*>(&detailView)}) {
            const UiSize size = measuring.layoutChild(*textView, SDL_Rect{0, 0, column.w, column.h}).getSize();
            context.layoutChild(*textView, SDL_Rect{column.x, y, column.w, size.h});
            y += size.h + gap;
        }
        if (!showBitrate) return;
        const UiSize size = measuring.layoutChild(bitrateChip, SDL_Rect{0, 0, column.w, column.h}).getSize();
        context.layoutChild(bitrateChip, SDL_Rect{column.x, y + gap, size.w, size.h});
    }

    /** The chip of the state, then the listening time of a live stream or the buffer bar of the others. */
    void layoutProgress(UiLayoutContext& context, const PlayerRects& rects) {
        const SDL_Rect row        = rects.progressRow;
        UiLayoutContext measuring = context.createMeasuringContext();
        const UiSize chipSize     = measuring.layoutChild(stateChip, row).getSize();
        context.layoutChild(stateChip, SDL_Rect{row.x, row.y + (row.h - chipSize.h) / 2, chipSize.w, chipSize.h});
        const int gap = SCALE1(14);
        int x         = row.x + chipSize.w + gap;
        if (view == RadioView::Live) {
            const UiSize size = measuring.layoutChild(listeningView, SDL_Rect{0, 0, row.x + row.w - x, row.h}).getSize();
            context.layoutChild(listeningView, SDL_Rect{x, row.y + (row.h - size.h) / 2, row.x + row.w - x, size.h});
            return;
        }
        if (view == RadioView::Paused) return;
        const UiSize bufferSize = measuring.layoutChild(bufferView, row).getSize();
        const int barHeight     = SCALE1(5);
        const int barWidth      = row.x + row.w - bufferSize.w - gap - x;
        context.layoutChild(bar, SDL_Rect{x, row.y + (row.h - barHeight) / 2, std::max(0, barWidth), barHeight});
        context.layoutChild(bufferView, SDL_Rect{row.x + row.w - bufferSize.w, row.y + (row.h - bufferSize.h) / 2,
                                                 bufferSize.w, bufferSize.h});
    }

    std::string initialUrl;
    /** The station that waits for its first frame, or an empty text */
    std::string pendingUrl;
    /** The frames laid out since the station began to wait */
    int pendingFramesLaid = 0;
    /** The stream of the station played with a healthy buffer since it started */
    bool wasHealthy = false;
    RadioView view = RadioView::Paused;
    RadioView previousView = RadioView::Paused;
    RadioView shownHintsView = RadioView::Live;
    uint32_t listenStart = 0;
    bool showBitrate = false;
    ScreenMode mode = ScreenMode::On;
    TaskId titleStart = 0;

    ArtFadeView artView;
    ScreenView screenView{Fonts_getLarge()};
    ChipView radioChip;
    TextView counterView;
    TextView metaView;
    TextView titleView;
    TextView subView;
    TextView detailView;
    ChipView bitrateChip;
    ChipView stateChip;
    TextView listeningView;
    ProgressTrackView bar;
    TextView bufferView;
    BoxView darkView;
    TextView wakeHintView;
};

}  // namespace

std::unique_ptr<UiPageImplBase> RadioPlayingPage::createPageImpl() const {
    return std::make_unique<RadioPlayerPageImpl<RadioPlayingPage>>(url);
}

std::unique_ptr<UiPageImplBase> createRadioPlayerPageImpl() {
    return std::make_unique<RadioPlayerPageImpl<NowPlayingPage>>(std::string{});
}

#include "ui/music_player_page.hpp"

#include <algorithm>
#include <array>
#include <memory>
#include <string>
#include <vector>

#include "album_art.h"
#include "lyric_window.h"
#include "lyrics.h"
#include "module_common.h"
#include "music_session.h"
#include "player.h"
#include "seek_scan.h"
#include "settings.h"
#include "ui/music_options_page.hpp"
#include "ui/music_queue_page.hpp"
#include "ui/music_player_layout.hpp"
#include "ui/now_playing_page.hpp"
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

/** The seek of a press of LEFT or RIGHT, in milliseconds */
constexpr int kSeekStepMs = 5000;

/** The most lyric rows that the window holds */
constexpr size_t kMaxLyricRows = 12;

/** The gap between two lyric rows, in pixels before the scale of the screen */
constexpr int kLyricRowGap = 2;

constexpr const char* kLeftHints[]       = {"Y", "OPTIONS", nullptr};
constexpr const char* kRightHintsPlaying[] = {"X", "QUEUE", "A", "PAUSE", nullptr};
constexpr const char* kRightHintsPaused[]  = {"X", "QUEUE", "A", "PLAY", nullptr};

/** Where the screen is: on, showing the wake hint before it goes dark, or dark */
enum class ScreenMode : uint8_t { On, Hint, Off };

class MusicPlayerPageImpl final : public UiPageImpl<NowPlayingPage> {
public:
    MusicPlayerPageImpl()
        : UiPageImpl(PageKind::FullScreen),
          counterView(Fonts_getSmall(), true),
          artistView(Fonts_getMedium(), true),
          titleView(Fonts_getHeading(), false),
          albumView(Fonts_getSmall(), true),
          elapsedView(Fonts_getSmall(), false),
          remainingView(Fonts_getSmall(), false),
          wakeHintView(Fonts_getMedium(), false) {
        screenView.getButtonHintsView().setHints(kLeftHints, kRightHintsPlaying);
        counterView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        artistView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        titleView.setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
        albumView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        elapsedView.setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
        remainingView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        // The screen goes dark: the colors are the platform's black and white, whatever the theme is
        darkView.setColor(SDL_Color{0, 0, 0, 255});
        wakeHintView.setColor(SDL_Color{255, 255, 255, 255});
        wakeHintView.setText("Press SELECT + A to wake screen");
        for (size_t row = 0; row < kMaxLyricRows; row++) {
            lyricViews.push_back(std::make_unique<TextView>(Fonts_getSmall(), false));
        }
        modeChips[0].setText("SHUFFLE");
        modeChips[1].setText("REPEAT");
        modeChips[2].setText("LYRICS");
    }

    std::span<const ButtonRequest> getButtonRequests() const override { return {}; }

    HelpId getHelpId() const override { return HELP_PLAYER; }
    ThemeSource getSource() const override { return THEME_SOURCE_MUSIC; }

    void onEnter() override {
        if (!MusicSession_isActive()) {
            close();
            return;
        }
        ModuleCommon_resetScreenOffHint();
        ModuleCommon_recordInputTime();
        shownSerial = MusicSession_getTrackSerial() - 1;
        refreshTrack();
    }

    void onExit() override {
        if (mode == ScreenMode::Off) PLAT_enableBacklight(1);
        // The keys lock the playback only
        ModuleCommon_resetScreenOffHint();
    }

    void resume() override {
        // Another page covered this one, and it can have changed the track
        shownSerial = MusicSession_getTrackSerial() - 1;
        refreshTrack();
    }

    void layout(UiLayoutContext& context) override {
        const UiSize screen     = context.getMaxSize();
        const PlayerRects rects = layoutPlayer(screen.w, screen.h, SCALE1(PADDING), SCALE1(BUTTON_PADDING),
                                               SCALE1(PILL_SIZE), SCALE1(PILL_SIZE) / 2);
        context.layoutChild(artView, rects.art);
        // The status pill and the button hints sit over the art
        context.layoutChild(screenView, context.getMaxRect());

        layoutTopRow(context, rects);
        const int contentBottom = layoutTextColumn(context, rects);
        layoutLyrics(context, rects, contentBottom);
        layoutProgress(context, rects);

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
        if (!MusicSession_isActive()) {
            finish(true);
            return;
        }
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
            if (!ModuleCommon_isKeyLocked()) ModuleCommon_updateSeekScan(kSeekStepMs, kSeekStepMs);
            if (Player_getState() == PLAYER_STATE_PLAYING && ModuleCommon_checkAutoScreenOffTimeout()) {
                mode = ScreenMode::Hint;
            }
        }
        refreshTrack();
        refreshProgress();
        refreshHints();
    }

    bool onButtonEvent(const ButtonEvent& event) override {
        if (mode != ScreenMode::On || ModuleCommon_isKeyLocked()) return true;
        const auto* press = std::get_if<event::ShortPress>(&event.kind);
        if (!press) return false;
        switch (press->button) {
            case Button::A:         Player_togglePause(); return true;
            case Button::B:         leave(); return true;
            case Button::X:         getNavigator().open(makeMusicQueuePage()); return true;
            case Button::Y:         getNavigator().open(MusicOptionsPage{}); return true;
            case Button::DpadDown:
            case Button::L1:        MusicSession_prev(); return true;
            case Button::DpadUp:
            case Button::R1:        MusicSession_next(); return true;
            case Button::R2:
            case Button::R3:        MusicSession_toggleLyrics(); return true;
            case Button::Select:
                ModuleCommon_startScreenOffHint();
                // A screen that keeps its content locks the keys in place of the wake hint
                if (ModuleCommon_isScreenOffHintActive()) mode = ScreenMode::Hint;
                return true;
            default: return false;
        }
    }

private:
    /** B: a playing track keeps its place for the next Resume, a track that is not playing ends the session. */
    void leave() {
        if (Player_getState() == PLAYER_STATE_PLAYING) {
            MusicSession_savePosition();
        } else {
            MusicSession_stop();
        }
        close();
    }

    /** Reads the track again where it changed: the texts, the chip of the format, the art. */
    void refreshTrack() {
        const unsigned serial = MusicSession_getTrackSerial();
        const bool fetching   = album_art_is_fetching();
        // The surface of the art is not safe to read while a fetch runs
        artView.setArt(fetching ? nullptr : Player_getAlbumArt());
        if (serial == shownSerial) return;
        shownSerial = serial;

        const TrackInfo* info = Player_getTrackInfo();
        if (!info) return;
        formatChip.setText(get_format_name(Player_detectFormat(Player_getCurrentFile())));
        artistView.setText(info->artist[0] ? info->artist : "Unknown Artist");
        const bool titleChanged = titleView.setText(info->title[0] ? info->title : "Unknown Title");
        albumView.setText(info->album);
        if (titleChanged) restartTitleScroll();
    }

    /** The counter, the times and the bar follow the playback. */
    void refreshProgress() {
        counterView.setText(formatTrackCounter(MusicSession_getTrackNumber(), MusicSession_getTrackCount()));
        const int position = SeekScan_displayPosition(SeekScan_shared(), Player_getPosition());
        const int duration = Player_getDuration();
        elapsedView.setText(formatPlayerTime(position));
        remainingView.setText(formatRemainingTime(position, duration));
        bar.setPercent(duration > 0 ? static_cast<int>(static_cast<long long>(position) * 100 / duration) : 0);
    }

    /** The label of A follows the state of the playback. */
    void refreshHints() {
        const bool playing = Player_getState() == PLAYER_STATE_PLAYING;
        if (playing == hintsShowPlaying) return;
        hintsShowPlaying = playing;
        screenView.getButtonHintsView().setHints(kLeftHints, playing ? kRightHintsPlaying : kRightHintsPaused);
    }

    void restartTitleScroll() {
        titleView.stopScrolling();
        titleView.resetScrolling();
        getScheduler().cancelTask(titleStart);
        titleStart = getScheduler().postDelayed([this] { titleView.startScrolling(); }, SCROLL_START_DELAY);
    }

    void layoutTopRow(UiLayoutContext& context, const PlayerRects& rects) {
        UiLayoutContext measuring = context.createMeasuringContext();
        const UiSize chipSize     = measuring.layoutChild(formatChip, rects.topRow).getSize();
        const int y               = rects.topRow.y + (rects.topRow.h - chipSize.h) / 2;
        context.layoutChild(formatChip, SDL_Rect{rects.topRow.x, y, chipSize.w, chipSize.h});
        const UiSize counterSize = measuring.layoutChild(counterView, rects.topRow).getSize();
        context.layoutChild(counterView, SDL_Rect{rects.topRow.x + chipSize.w + SCALE1(10), rects.topRow.y + (rects.topRow.h - counterSize.h) / 2,
                                                  counterSize.w, counterSize.h});
    }

    /** Stacks the artist, the title, the album and the mode chips. Returns the bottom of the last one. */
    int layoutTextColumn(UiLayoutContext& context, const PlayerRects& rects) {
        const SDL_Rect column     = rects.textColumn;
        UiLayoutContext measuring = context.createMeasuringContext();
        const int gap             = SCALE1(6);
        int y                     = column.y;
        for (UiView* view : {static_cast<UiView*>(&artistView), static_cast<UiView*>(&titleView), static_cast<UiView*>(&albumView)}) {
            const UiSize size = measuring.layoutChild(*view, SDL_Rect{0, 0, column.w, column.h}).getSize();
            context.layoutChild(*view, SDL_Rect{column.x, y, column.w, size.h});
            y += size.h + gap;
        }
        // The chips of the modes that are on, in a row
        int x                 = column.x;
        const bool modeOn[]   = {MusicSession_isShuffle(), MusicSession_isRepeat(), Settings_getBool(&SETTING_LYRICS_ENABLED)};
        int chipHeight        = 0;
        for (size_t index = 0; index < modeChips.size(); index++) {
            if (!modeOn[index]) continue;
            const UiSize size = measuring.layoutChild(modeChips[index], SDL_Rect{0, 0, column.w, column.h}).getSize();
            if (x + size.w > column.x + column.w) break;
            context.layoutChild(modeChips[index], SDL_Rect{x, y + gap, size.w, size.h});
            x += size.w + SCALE1(8);
            chipHeight = std::max(chipHeight, size.h + gap);
        }
        return y + chipHeight;
    }

    /** Draws the window of the lyrics, with the current one in the primary color, in the room left. */
    void layoutLyrics(UiLayoutContext& context, const PlayerRects& rects, int contentBottom) {
        if (!Settings_getBool(&SETTING_LYRICS_ENABLED)) return;
        const SDL_Rect room = layoutLyricsRect(rects, contentBottom, SCALE1(8));
        // The current lyric is larger than the others, thus each row has the height of the larger font
        const int rowHeight = TTF_FontHeight(Fonts_getMedium());
        const int gap       = SCALE1(kLyricRowGap);
        int rowCount        = rowHeight > 0 && room.h >= rowHeight ? (room.h + gap) / (rowHeight + gap) : 0;
        rowCount            = std::min(rowCount, static_cast<int>(kMaxLyricRows));

        const int position     = SeekScan_displayPosition(SeekScan_shared(), Player_getPosition());
        const int count        = Lyrics_lineCount();
        const LyricWindow lyricWindow = LyricWindow_layout(count, Lyrics_currentIndex(position), rowCount);
        const bool synced      = Lyrics_isSynced();
        for (int row = 0; row < lyricWindow.count; row++) {
            TextView& view = *lyricViews[static_cast<size_t>(row)];
            view.setText(Lyrics_lineText(lyricWindow.first + row));
            const ThemeRole role = (synced && row == lyricWindow.current_row) ? THEME_ROLE_PRIMARY : THEME_ROLE_SECONDARY;
            view.setColor(Theme_getColor(role, false));
            view.setFont(role == THEME_ROLE_PRIMARY ? Fonts_getMedium() : Fonts_getSmall());
            context.layoutChild(view, SDL_Rect{room.x, room.y + row * (rowHeight + gap), room.w, rowHeight});
        }
    }

    void layoutProgress(UiLayoutContext& context, const PlayerRects& rects) {
        const SDL_Rect row        = rects.progressRow;
        UiLayoutContext measuring = context.createMeasuringContext();
        const UiSize elapsedSize   = measuring.layoutChild(elapsedView, row).getSize();
        const UiSize remainingSize = measuring.layoutChild(remainingView, row).getSize();
        // The bar sits between the two times, and its ends do not move while the times change
        const int timeWidth = std::max(elapsedSize.w, remainingSize.w);
        const int gap       = SCALE1(10);
        const int barHeight = SCALE1(5);
        const int y         = row.y + (row.h - elapsedSize.h) / 2;
        context.layoutChild(elapsedView, SDL_Rect{row.x, y, timeWidth, elapsedSize.h});
        context.layoutChild(remainingView, SDL_Rect{row.x + row.w - remainingSize.w, y, remainingSize.w, remainingSize.h});
        context.layoutChild(bar, SDL_Rect{row.x + timeWidth + gap, row.y + (row.h - barHeight) / 2,
                                          row.w - 2 * timeWidth - 2 * gap, barHeight});
    }

    ScreenMode mode = ScreenMode::On;
    unsigned shownSerial = 0;
    bool hintsShowPlaying = true;
    TaskId titleStart = 0;

    ArtFadeView artView;
    ScreenView screenView{Fonts_getLarge()};
    ChipView formatChip;
    TextView counterView;
    TextView artistView;
    TextView titleView;
    TextView albumView;
    std::array<ChipView, 3> modeChips;
    std::vector<std::unique_ptr<TextView>> lyricViews;
    TextView elapsedView;
    TextView remainingView;
    ProgressTrackView bar;
    BoxView darkView;
    TextView wakeHintView;
};

}  // namespace

std::unique_ptr<UiPageImplBase> createMusicPlayerPageImpl() {
    return std::make_unique<MusicPlayerPageImpl>();
}

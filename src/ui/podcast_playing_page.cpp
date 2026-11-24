#include "ui/podcast_playing_page.hpp"

#include <algorithm>
#include <memory>
#include <string>
#include <unistd.h>

#include "background.h"
#include "module_common.h"
#include "module_podcast.h"
#include "player.h"
#include "podcast.h"
#include "podcast_format.hpp"
#include "podcast_sleep.h"
#include "seek_scan.h"
#include "toast.h"
#include "ui/confirm_dialog_page.hpp"
#include "ui/music_player_layout.hpp"
#include "ui/now_playing_page.hpp"
#include "ui/podcast_art.hpp"
#include "ui/podcast_episodes_page.hpp"
#include "ui/podcast_options_page.hpp"
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

/** The seek of a short press of LEFT (back) and RIGHT (forward), in milliseconds */
constexpr int kSeekBackMs    = 10000;
constexpr int kSeekForwardMs = 30000;

constexpr const char* kLeftHints[]          = {"Y", "OPTIONS", nullptr};
constexpr const char* kRightHintsPlaying[]  = {"X", "EPISODES", "A", "PAUSE", nullptr};
constexpr const char* kRightHintsPaused[]   = {"X", "EPISODES", "A", "PLAY", nullptr};
constexpr const char* kRightHintsSeeking[]  = {"B", "CANCEL", nullptr};

/** Where the screen is: on, showing the wake hint before it goes dark, or dark */
enum class ScreenMode : uint8_t { On, Hint, Off };

/** Whether the episode waits for its saved position or plays */
enum class Phase : uint8_t { Seeking, Playing };

/** The playing screen of the podcasts, for the page that opens it (`Page`) */
template <class Page>
class PodcastPlayerPageImpl final : public UiPageImpl<Page> {
public:
    PodcastPlayerPageImpl(int feedIndex, int episodeIndex)
        : UiPageImpl<Page>(PageKind::FullScreen),
          feedIndex(feedIndex),
          episodeIndex(episodeIndex),
          counterView(Fonts_getSmall(), true),
          showView(Fonts_getMedium(), true),
          titleView(Fonts_getHeading(), false),
          subView(Fonts_getMedium(), true),
          elapsedView(Fonts_getSmall(), false),
          remainingView(Fonts_getSmall(), false),
          wakeHintView(Fonts_getMedium(), false) {
        screenView.getButtonHintsView().setHints(kLeftHints, kRightHintsPlaying);
        counterView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        showView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        titleView.setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
        subView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        elapsedView.setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
        remainingView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        // The screen goes dark: the colors are the platform's black and white, whatever the theme is
        darkView.setColor(SDL_Color{0, 0, 0, 255});
        wakeHintView.setColor(SDL_Color{255, 255, 255, 255});
        wakeHintView.setText("Press SELECT + A to wake screen");
        artView.setSource(THEME_SOURCE_PODCAST);
        podcastChip.setText("PODCAST");
    }

    ~PodcastPlayerPageImpl() override { freeArt(); }

    std::span<const ButtonRequest> getButtonRequests() const override { return {}; }

    HelpId getHelpId() const override { return HELP_PODCAST_PLAYING; }
    ThemeSource getSource() const override { return THEME_SOURCE_PODCAST; }

    void onEnter() override {
        Podcast_init();
        if (episodeIndex >= 0) {
            if (!startEpisode(feedIndex, episodeIndex)) {
                this->close();
                return;
            }
        } else {
            // The episode of the background: this page owns the playback while it shows
            if (!Podcast_isActive()) {
                this->close();
                return;
            }
            if (Background_getActive() == BG_PODCAST) Background_setActive(BG_NONE);
            feedIndex    = Podcast_getCurrentFeedIndex();
            episodeIndex = Podcast_getCurrentEpisodeIndex();
            phase        = Phase::Playing;
        }
        ModuleCommon_resetScreenOffHint();
        ModuleCommon_recordInputTime();
        ModuleCommon_setAutosleepDisabled(true);
        readEpisode();
        refreshAll();
    }

    void onExit() override {
        if (mode == ScreenMode::Off) PLAT_enableBacklight(1);
        Toast_dismiss(seekToast);
        // The keys lock the playback only
        ModuleCommon_resetScreenOffHint();
        // A quit stops the episode, as it stopped it from the legacy screen
        if (ModuleCommon_isQuitRequested()) {
            Podcast_stop();
            Podcast_flushProgress();
        }
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
        ModuleCommon_setAutosleepDisabled(true);

        if (phase == Phase::Seeking) {
            // The seek to the saved position runs: the playback starts when it is done
            if (!Player_resume()) {
                Player_play();
                Toast_dismiss(seekToast);
                ModuleCommon_recordInputTime();
                phase = Phase::Playing;
            }
        } else if (PodcastModule_tickPlayback()) {
            // The episode ended, or the sleep timer stopped it
            ModuleCommon_setAutosleepDisabled(false);
            this->close();
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
        } else if (mode == ScreenMode::On && phase == Phase::Playing) {
            if (!ModuleCommon_isKeyLocked()) ModuleCommon_updateSeekScan(kSeekBackMs, kSeekForwardMs);
            if (Player_getState() == PLAYER_STATE_PLAYING && ModuleCommon_checkAutoScreenOffTimeout()) {
                mode = ScreenMode::Hint;
            }
        }
        refreshAll();
    }

    bool onButtonEvent(const ButtonEvent& event) override {
        if (mode != ScreenMode::On || ModuleCommon_isKeyLocked()) return true;
        const auto* press = std::get_if<event::ShortPress>(&event.kind);
        if (!press) return false;
        if (phase == Phase::Seeking) {
            // Only B works while the seek runs: it gives the episode up
            if (press->button == Button::B) cancelSeek();
            return true;
        }
        switch (press->button) {
            case Button::A:         Player_togglePause(); return true;
            case Button::B:         leave(); return true;
            case Button::X:         openEpisodes(); return true;
            case Button::Y:         openOptions(); return true;
            case Button::DpadUp:    changeSpeed(1); return true;
            case Button::DpadDown:  changeSpeed(-1); return true;
            case Button::Select:
                ModuleCommon_startScreenOffHint();
                // A screen that keeps its content locks the keys in place of the wake hint
                if (ModuleCommon_isScreenOffHintActive()) mode = ScreenMode::Hint;
                return true;
            default: return false;
        }
    }

private:
    /** Loads the downloaded episode and seeks to its saved position. Returns false (with a toast) on a failure. */
    bool startEpisode(int feed, int episode) {
        PodcastFeed* feedPtr = Podcast_getSubscription(feed);
        PodcastEpisode* ep   = Podcast_getEpisode(feed, episode);
        if (!feedPtr || !ep || !Podcast_episodeFileExists(feedPtr, episode)) {
            Toast_show("Episode not available", TOAST_DURATION);
            return false;
        }
        // The episode that plays now stops (and keeps its position), also one of the background
        if (Podcast_isActive()) {
            if (feed == Podcast_getCurrentFeedIndex() && episode == Podcast_getCurrentEpisodeIndex()) {
                // It plays already: the screen takes it back
                if (Background_getActive() == BG_PODCAST) Background_setActive(BG_NONE);
                feedIndex    = feed;
                episodeIndex = episode;
                phase        = Phase::Playing;
                return true;
            }
            Podcast_stop();
            Podcast_flushProgress();
        }
        Background_stopAll();
        const int loaded = Podcast_loadAndSeek(feedPtr, episode);
        if (loaded < 0) {
            Toast_show("Failed to play", TOAST_DURATION);
            return false;
        }
        // The flag of the new episodes goes only where the episode really plays
        Podcast_clearNewFlag(feed, episode);
        ep = Podcast_getEpisode(feed, episode);
        if (ep) {
            Podcast_updateContinueListening(feedPtr->feed_url, feedPtr->feed_id, ep->guid, ep->title, feedPtr->title,
                                            feedPtr->artwork_url);
        }
        feedIndex    = feed;
        episodeIndex = episode;
        ModuleCommon_recordInputTime();
        PodcastSleep_clear();
        if (loaded == 1) {
            // It has a saved position: the screen shows while the seek runs
            phase = Phase::Seeking;
            char message[64];
            if (ep && ep->progress_sec > 0) {
                std::snprintf(message, sizeof(message), "Resuming at %s...", formatPlayerTime(ep->progress_sec * 1000).c_str());
            } else {
                std::snprintf(message, sizeof(message), "Resuming...");
            }
            seekToast = Toast_showScreenBound(message, TOAST_DURATION_FOREVER);
        } else {
            Player_play();
            phase = Phase::Playing;
        }
        return true;
    }

    /** Takes the episode of another row of the list, or comes back to the one that plays. */
    void pickEpisode(int episode) {
        const int feed = feedIndex;
        if (!startEpisode(feed, episode)) return;
        readEpisode();
        refreshAll();
    }

    /** B while the seek runs: the episode stops, and the page closes. */
    void cancelSeek() {
        Toast_dismiss(seekToast);
        Podcast_stop();
        Podcast_flushProgress();
        ModuleCommon_setAutosleepDisabled(false);
        this->close();
    }

    /** B: an episode that plays goes on in the background, an episode that is paused stops. */
    void leave() {
        Podcast_flushProgress();
        if (Player_getState() == PLAYER_STATE_PLAYING) {
            Background_setActive(BG_PODCAST);
        } else {
            Podcast_stop();
            Podcast_flushProgress();
            ModuleCommon_setAutosleepDisabled(false);
        }
        this->close();
    }

    void changeSpeed(int direction) {
        const float speed = stepPlaybackSpeed(Player_getPlaybackSpeed(), direction);
        Player_setPlaybackSpeed(speed);
        const std::string text = "Speed: " + formatPlaybackSpeed(speed);
        Toast_show(text.c_str(), TOAST_DURATION);
    }

    void openEpisodes() {
        this->getNavigator().open(makePodcastEpisodesPage(feedIndex, [this](int episode) { pickEpisode(episode); }));
    }

    void openOptions() {
        const PodcastFeed* feed = Podcast_getSubscription(feedIndex);
        const PodcastEpisode* ep = Podcast_getEpisode(feedIndex, episodeIndex);
        this->getNavigator().openForResult(PodcastOptionsPage{feed ? feed->title : "", ep ? ep->title : ""},
                                           [this](const PodcastPlayerAction& action) {
                                               switch (action) {
                                                   case PodcastPlayerAction::None:         refreshAll(); break;
                                                   case PodcastPlayerAction::MarkListened: markListened(); break;
                                                   case PodcastPlayerAction::DeleteFile:   confirmDeletion(); break;
                                               }
                                           });
    }

    /** Stops the episode and marks it as listened. */
    void stopAsListened(const char* toast) {
        const PodcastFeed* feed = Podcast_getSubscription(feedIndex);
        const PodcastEpisode* ep = Podcast_getEpisode(feedIndex, episodeIndex);
        const std::string url  = feed ? feed->feed_url : "";
        const std::string guid = ep ? ep->guid : "";
        Podcast_stop();
        if (!url.empty() && !guid.empty()) {
            Podcast_markAsPlayed(url.c_str(), guid.c_str());
            Podcast_removeContinueListening(url.c_str(), guid.c_str());
        }
        Podcast_flushProgress();
        Toast_show(toast, TOAST_DURATION);
        ModuleCommon_setAutosleepDisabled(false);
        this->close();
    }

    void markListened() { stopAsListened("Marked as played"); }

    void confirmDeletion() {
        const PodcastEpisode* ep = Podcast_getEpisode(feedIndex, episodeIndex);
        this->getNavigator().openForResult(ConfirmDialogPage("Delete file?", ep ? ep->title : ""), [this](const bool& yes) {
            if (!yes) return;
            PodcastFeed* feed = Podcast_getSubscription(feedIndex);
            if (!feed) return;
            char path[PODCAST_MAX_URL];
            Podcast_getEpisodeLocalPath(feed, episodeIndex, path, sizeof(path));
            stopAsListened(unlink(path) == 0 ? "File deleted" : "Delete failed");
        });
    }

    void freeArt() {
        artView.setArt(nullptr);
        if (art) SDL_FreeSurface(art);
        art = nullptr;
    }

    /** Reads the episode that plays: its texts and its art. */
    void readEpisode() {
        const PodcastFeed* feed  = Podcast_getSubscription(feedIndex);
        const PodcastEpisode* ep = Podcast_getEpisode(feedIndex, episodeIndex);
        if (!feed || !ep) return;
        counterView.setText(formatTrackCounter(episodeIndex + 1, feed->episode_count));
        showView.setText(feed->title);
        const bool titleChanged = titleView.setText(ep->title[0] ? ep->title : "Unknown Episode");
        publishedText           = formatPublishedDate(ep->pub_date);
        if (titleChanged) restartTitleScroll();
        freeArt();
        art = loadPodcastArt(feed->feed_id);
        artView.setArt(art);
    }

    /** Reads the playback again: the line under the title, the chips, the times, the bar and the hints. */
    void refreshAll() {
        subView.setText(phase == Phase::Seeking ? "Resuming..." : publishedText);

        const float speed = Player_getPlaybackSpeed();
        showSpeed         = std::abs(speed - 1.0f) > 0.01f;
        if (showSpeed) speedChip.setText("Speed " + formatPlaybackSpeed(speed));
        const int minutes = PodcastSleep_getMinutes();
        showSleep         = minutes > 0;
        if (showSleep) sleepChip.setText("Sleep " + std::to_string(minutes) + " min");

        const int position = SeekScan_displayPosition(SeekScan_shared(), Player_getPosition());
        const int duration = Podcast_getDuration();
        elapsedView.setText(formatPlayerTime(position));
        remainingView.setText(formatRemainingTime(position, duration));
        bar.setPercent(duration > 0 ? std::clamp(static_cast<int>(static_cast<long long>(position) * 100 / duration), 0, 100) : 0);

        const bool playing = Player_getState() == PLAYER_STATE_PLAYING;
        const int hints    = phase == Phase::Seeking ? 2 : (playing ? 0 : 1);
        if (hints != shownHints) {
            shownHints = hints;
            const char* const* right = hints == 2 ? kRightHintsSeeking : (hints == 0 ? kRightHintsPlaying : kRightHintsPaused);
            screenView.getButtonHintsView().setHints(kLeftHints, right);
        }
    }

    void restartTitleScroll() {
        titleView.stopScrolling();
        titleView.resetScrolling();
        this->getScheduler().cancelTask(titleStart);
        titleStart = this->getScheduler().postDelayed([this] { titleView.startScrolling(); }, SCROLL_START_DELAY);
    }

    void layoutTopRow(UiLayoutContext& context, const PlayerRects& rects) {
        UiLayoutContext measuring = context.createMeasuringContext();
        const UiSize chipSize     = measuring.layoutChild(podcastChip, rects.topRow).getSize();
        const int y               = rects.topRow.y + (rects.topRow.h - chipSize.h) / 2;
        context.layoutChild(podcastChip, SDL_Rect{rects.topRow.x, y, chipSize.w, chipSize.h});
        const UiSize counterSize = measuring.layoutChild(counterView, rects.topRow).getSize();
        context.layoutChild(counterView, SDL_Rect{rects.topRow.x + chipSize.w + SCALE1(10),
                                                  rects.topRow.y + (rects.topRow.h - counterSize.h) / 2, counterSize.w,
                                                  counterSize.h});
    }

    /** Stacks the show, the episode, its date and the chips of the speed and of the sleep timer. */
    void layoutTextColumn(UiLayoutContext& context, const PlayerRects& rects) {
        const SDL_Rect column     = rects.textColumn;
        UiLayoutContext measuring = context.createMeasuringContext();
        const int gap             = SCALE1(6);
        int y                     = column.y;
        for (UiView* textView : {static_cast<UiView*>(&showView), static_cast<UiView*>(&titleView), static_cast<UiView*>(&subView)}) {
            const UiSize size = measuring.layoutChild(*textView, SDL_Rect{0, 0, column.w, column.h}).getSize();
            context.layoutChild(*textView, SDL_Rect{column.x, y, column.w, size.h});
            y += size.h + gap;
        }
        int x = column.x;
        for (const auto& [shown, chip] : {std::pair<bool, ChipView*>{showSpeed, &speedChip}, std::pair<bool, ChipView*>{showSleep, &sleepChip}}) {
            if (!shown) continue;
            const UiSize size = measuring.layoutChild(*chip, SDL_Rect{0, 0, column.w, column.h}).getSize();
            if (x + size.w > column.x + column.w) break;
            context.layoutChild(*chip, SDL_Rect{x, y + gap, size.w, size.h});
            x += size.w + SCALE1(8);
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

    int feedIndex;
    int episodeIndex;
    Phase phase     = Phase::Playing;
    ScreenMode mode = ScreenMode::On;
    ToastToken seekToast = TOAST_TOKEN_NONE;
    std::string publishedText;
    int shownHints = 0;
    bool showSpeed = false;
    bool showSleep = false;
    TaskId titleStart = 0;
    SDL_Surface* art  = nullptr;

    ArtFadeView artView;
    ScreenView screenView{Fonts_getLarge()};
    ChipView podcastChip;
    TextView counterView;
    TextView showView;
    TextView titleView;
    TextView subView;
    ChipView speedChip;
    ChipView sleepChip;
    TextView elapsedView;
    TextView remainingView;
    ProgressTrackView bar;
    BoxView darkView;
    TextView wakeHintView;
};

}  // namespace

std::unique_ptr<UiPageImplBase> PodcastPlayingPage::createPageImpl() const {
    return std::make_unique<PodcastPlayerPageImpl<PodcastPlayingPage>>(feedIndex, episodeIndex);
}

std::unique_ptr<UiPageImplBase> createPodcastPlayerPageImpl() {
    return std::make_unique<PodcastPlayerPageImpl<NowPlayingPage>>(-1, -1);
}

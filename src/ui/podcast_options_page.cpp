#include "ui/podcast_options_page.hpp"

#include <variant>
#include <vector>

#include "player.h"
#include "podcast_format.hpp"
#include "podcast_sleep.h"
#include "sleep_timer.h"
#include "ui/page/list_screen_controller.hpp"
#include "ui/view/art_fade_view.hpp"
#include "ui/view/box_view.hpp"
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

constexpr const char* kLeftHints[]  = {"LEFT/RIGHT", "CHANGE", nullptr};
constexpr const char* kRightHints[] = {"B", "CLOSE", "A", "CHOOSE", nullptr};

class PodcastOptionsPageImpl final : public UiPageImpl<PodcastOptionsPage> {
public:
    PodcastOptionsPageImpl(std::string show, std::string episode)
        : UiPageImpl(PageKind::FullScreen), showView(Fonts_getMedium(), true), titleView(Fonts_getHeading(), false) {
        screenView.getButtonHintsView().setHints(kLeftHints, kRightHints);
        showView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        titleView.setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
        // The dim over the player that shows behind the panel
        SDL_Color dim = Theme_getColor(THEME_ROLE_PAGE_BACKGROUND, false);
        dim.a         = 158;
        dimView.setColor(dim);
        artView.setSource(THEME_SOURCE_PODCAST);
        showView.setText(show.empty() ? "Podcast" : show);
        titleView.setText(episode);
        panel.setScope("EPISODE");
    }

    std::span<const ButtonRequest> getButtonRequests() const override {
        return ListScreenController::getButtonRequests();
    }

    HelpId getHelpId() const override { return HELP_PLAYER_OPTIONS; }
    ThemeSource getSource() const override { return THEME_SOURCE_PODCAST; }

    void onEnter() override {
        panel.setRows({{"Speed", getSpeedValue()}, {"Sleep timer", getSleepValue()}, {"Mark as listened", ""}, {"Delete file", ""}});
    }

    void layout(UiLayoutContext& context) override {
        const UiSize screen = context.getMaxSize();
        context.layoutChild(artView, SDL_Rect{screen.w - screen.w * 620 / 1024, 0, screen.w * 620 / 1024, screen.h});
        context.layoutChild(dimView, context.getMaxRect());
        context.layoutChild(screenView, context.getMaxRect());
        const int x               = SCALE1(PADDING) + SCALE1(BUTTON_PADDING);
        const int y               = screen.h * 132 / 768;
        const int width           = screen.w * 440 / 1024;
        UiLayoutContext measuring = context.createMeasuringContext();
        const int showHeight      = measuring.layoutChild(showView, SDL_Rect{0, 0, width, screen.h}).getSize().h;
        context.layoutChild(showView, SDL_Rect{x, y, width, showHeight});
        const int titleHeight = measuring.layoutChild(titleView, SDL_Rect{0, 0, width, screen.h}).getSize().h;
        context.layoutChild(titleView, SDL_Rect{x, y + showHeight + SCALE1(6), width, titleHeight});
        context.layoutChild(panel, context.getMaxRect());
    }

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
                if (shortPress) choose();
                return true;
            case Button::B:
                if (shortPress) close();
                return true;
            default: return false;
        }
    }

private:
    enum class Row : int { Speed, SleepTimer, MarkListened, DeleteFile };

    static std::string getSpeedValue() { return formatPlaybackSpeed(Player_getPlaybackSpeed()); }

    static std::string getSleepValue() {
        const int minutes = PodcastSleep_getMinutes();
        return minutes > 0 ? std::to_string(minutes) + " min" : "Off";
    }

    Row selectedRow() const { return static_cast<Row>(panel.getSelectedIndex()); }

    /** Left and Right: changes the value of the selected row. */
    void change(int direction) {
        switch (selectedRow()) {
            case Row::Speed:
                Player_setPlaybackSpeed(stepPlaybackSpeed(Player_getPlaybackSpeed(), direction));
                panel.setValue(panel.getSelectedIndex(), getSpeedValue());
                break;
            case Row::SleepTimer:
                PodcastSleep_set(SleepTimer_stepMinutes(PodcastSleep_getMinutes(), direction));
                panel.setValue(panel.getSelectedIndex(), getSleepValue());
                break;
            default: break;
        }
    }

    void choose() {
        switch (selectedRow()) {
            case Row::Speed:
            case Row::SleepTimer:   change(1); return;
            case Row::MarkListened: finish(PodcastPlayerAction::MarkListened); return;
            case Row::DeleteFile:   finish(PodcastPlayerAction::DeleteFile); return;
        }
    }

    ArtFadeView artView;
    BoxView dimView;
    ScreenView screenView{Fonts_getLarge()};
    TextView showView;
    TextView titleView;
    OptionsPanelView panel;
};

}  // namespace

std::unique_ptr<UiPageImplBase> PodcastOptionsPage::createPageImpl() const {
    return std::make_unique<PodcastOptionsPageImpl>(show, episode);
}

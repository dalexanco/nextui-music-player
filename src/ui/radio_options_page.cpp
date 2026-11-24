#include "ui/radio_options_page.hpp"

#include <variant>
#include <vector>

#include "radio_favourites.h"
#include "radio_sleep.h"
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

class RadioOptionsPageImpl final : public UiPageImpl<RadioOptionsPage> {
public:
    RadioOptionsPageImpl(std::string url, std::string name, std::string genre, bool player)
        : UiPageImpl(PageKind::FullScreen),
          player(player),
          url(std::move(url)),
          genreView(Fonts_getMedium(), true),
          titleView(Fonts_getHeading(), false) {
        screenView.getButtonHintsView().setHints(kLeftHints, kRightHints);
        genreView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        titleView.setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
        // The dim over the list that shows behind the panel
        SDL_Color dim = Theme_getColor(THEME_ROLE_PAGE_BACKGROUND, false);
        dim.a         = 158;
        dimView.setColor(dim);
        artView.setSource(THEME_SOURCE_RADIO);
        genreView.setText(genre.empty() ? "Radio" : genre);
        titleView.setText(name);
        panel.setScope("STATION");
    }

    std::span<const ButtonRequest> getButtonRequests() const override {
        return ListScreenController::getButtonRequests();
    }

    HelpId getHelpId() const override { return HELP_PLAYER_OPTIONS; }
    ThemeSource getSource() const override { return THEME_SOURCE_RADIO; }

    void onEnter() override { panel.setRows(readRows()); }

    void layout(UiLayoutContext& context) override {
        const UiSize screen = context.getMaxSize();
        context.layoutChild(artView, SDL_Rect{screen.w - screen.w * 620 / 1024, 0, screen.w * 620 / 1024, screen.h});
        context.layoutChild(dimView, context.getMaxRect());
        context.layoutChild(screenView, context.getMaxRect());
        const int x               = SCALE1(PADDING) + SCALE1(BUTTON_PADDING);
        const int y               = screen.h * 132 / 768;
        const int width           = screen.w * 440 / 1024;
        UiLayoutContext measuring = context.createMeasuringContext();
        const int genreHeight     = measuring.layoutChild(genreView, SDL_Rect{0, 0, width, screen.h}).getSize().h;
        context.layoutChild(genreView, SDL_Rect{x, y, width, genreHeight});
        const int titleHeight = measuring.layoutChild(titleView, SDL_Rect{0, 0, width, screen.h}).getSize().h;
        context.layoutChild(titleView, SDL_Rect{x, y + genreHeight + SCALE1(6), width, titleHeight});
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
    enum class Row : int { Favourite, Rename, Remove, AddStations, SleepTimer };

    /** The rows of the station, or the row of the list with no station */
    std::vector<OptionRow> readRows() const {
        if (player) return {{"Favourite", getFavouriteValue()}, {"Sleep timer", getSleepValue()}};
        if (url.empty()) return {{"Add stations", ""}};
        return {
            {"Favourite", getFavouriteValue()},
            {"Rename", ""},
            {"Remove", ""},
            {"Add stations", ""},
        };
    }

    /** Returns the row that the selected index names, as the list with no station has one row. */
    Row selectedRow() const {
        if (player) return panel.getSelectedIndex() == 0 ? Row::Favourite : Row::SleepTimer;
        if (url.empty()) return Row::AddStations;
        return static_cast<Row>(panel.getSelectedIndex());
    }

    std::string getFavouriteValue() const { return RadioFavourites_isFavourite(url.c_str()) ? "On" : "Off"; }

    static std::string getSleepValue() {
        const int minutes = RadioSleep_getMinutes();
        return minutes > 0 ? std::to_string(minutes) + " min" : "Off";
    }

    /** Left and Right: changes the value of the selected row. */
    void change(int direction) {
        switch (selectedRow()) {
            case Row::Favourite:
                RadioFavourites_toggleFavourite(url.c_str());
                panel.setValue(panel.getSelectedIndex(), getFavouriteValue());
                break;
            case Row::SleepTimer:
                RadioSleep_set(SleepTimer_stepMinutes(RadioSleep_getMinutes(), direction));
                panel.setValue(panel.getSelectedIndex(), getSleepValue());
                break;
            default: break;
        }
    }

    void choose() {
        switch (selectedRow()) {
            case Row::Favourite:
            case Row::SleepTimer:   change(1); return;
            case Row::Rename:       finish(RadioStationAction::Rename); return;
            case Row::Remove:       finish(RadioStationAction::Remove); return;
            case Row::AddStations:  finish(RadioStationAction::AddStations); return;
        }
    }

    bool player;
    std::string url;
    ArtFadeView artView;
    BoxView dimView;
    ScreenView screenView{Fonts_getLarge()};
    TextView genreView;
    TextView titleView;
    OptionsPanelView panel;
};

}  // namespace

std::unique_ptr<UiPageImplBase> RadioOptionsPage::createPageImpl() const {
    return std::make_unique<RadioOptionsPageImpl>(url, name, genre, player);
}

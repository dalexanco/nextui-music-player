#include "ui/radio_page.hpp"

#include <memory>
#include <string>
#include <unordered_map>

#include "background.h"
#include "radio.h"
#include "toast.h"
#include "ui/confirm_dialog_page.hpp"
#include "ui/choice_options_page.hpp"
#include "ui/page/list_screen_controller.hpp"
#include "ui/radio_manage_page.hpp"
#include "ui/radio_playing_page.hpp"
#include "ui/radio_station_panel.hpp"
#include "radio_favourites.h"
#include "ui/view/detail_panel_view.hpp"
#include "ui/view/empty_state_view.hpp"
#include "ui/view/list_adapter.hpp"
#include "ui/view/list_row_view.hpp"
#include "ui/view/list_screen_view.hpp"
#include "ui/view/text_view.hpp"
#include "ui/wifi_dialog_page.hpp"
#include "ui_fonts.h"
#include "ui_theme.h"

// The NextUI headers declare C functions and have no guard of their own
extern "C" {
#include "defines.h"
#include "api.h"
}

using namespace input;

namespace {

constexpr const char* kLeftHints[]       = {"Y", "OPTIONS", nullptr};
constexpr const char* kRightHints[]      = {"B", "BACK", "A", "LISTEN", nullptr};
// A list with no station has one action besides B: it is the third, shown as it is
constexpr const char* kEmptyLeftHints[]  = {"Y", "MANAGE", nullptr};
constexpr const char* kEmptyRightHints[] = {"B", "BACK", nullptr};

/** The URL of the station of the cursor when the station list closed last */
std::string lastSelectedUrl;

/**
 * The rows of the saved stations. The id of a row comes from the URL, the name and the genre of its
 * station, thus a station that does not change keeps its row view while the list shows.
 */
class StationListAdapter final : public ListAdapter {
public:
    int getCount() const override {
        RadioStation* stations;
        return Radio_getStations(&stations);
    }

    long getItemId(int index) const override {
        RadioStation* stations;
        Radio_getStations(&stations);
        const RadioStation& station = stations[index];
        std::string key = std::string(station.url) + '\n' + station.name + '\n' + station.genre;
        const auto [it, added] = ids.try_emplace(std::move(key), static_cast<long>(ids.size()));
        return it->second;
    }

    std::shared_ptr<ListItemView> getView(int index) override {
        RadioStation* stations;
        Radio_getStations(&stations);
        const RadioStation& station = stations[index];
        auto row = std::make_shared<ListRowView>(Fonts_getMedium());
        row->setText(station.name);
        return row;
    }

private:
    mutable std::unordered_map<std::string, long> ids;
};

class RadioPageImpl final : public UiPageImpl<RadioPage>, private ListScreenListener {
public:
    RadioPageImpl() : UiPageImpl(PageKind::FullScreen) {
        screenView.getTitleView().setText("Radio");
        screenView.getTitleView().setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        emptyState.setMessage("No stations saved");
        emptyState.setLine("Press Y to manage stations");
        screenView.setEmptyState(&emptyState);
    }

    std::span<const ButtonRequest> getButtonRequests() const override {
        return ListScreenController::getButtonRequests();
    }

    HelpId getHelpId() const override { return HELP_RADIO_LIST; }
    ThemeSource getSource() const override { return THEME_SOURCE_RADIO; }

    void onEnter() override {
        if (Radio_init() != 0) {
            Toast_show("Radio is not available", TOAST_DURATION);
            close();
            return;
        }
        radioReady = true;
        screenView.setAdapter(stations);
        list.start();
        selectStation(lastSelectedUrl);
        updateHints();
        if (Background_getActive() == BG_RADIO && Radio_isActive()) openPlayingScreen({});
    }

    void onExit() override {
        if (!radioReady) return;
        int index;
        if (const RadioStation* station = getSelectedStation(index)) lastSelectedUrl = station->url;
        if (!Radio_isActive()) Radio_quit();
    }

    void layout(UiLayoutContext& context) override {
        // An empty list has nothing to show in a panel
        screenView.setSidePanel(stations.getCount() > 0 ? &panel : nullptr);
        context.layoutChild(screenView, context.getMaxRect());
    }

    bool onButtonEvent(const ButtonEvent& event) override {
        if (list.onButtonEvent(event)) return true;
        if (event.isSingleButton(Button::A)) {
            playSelectedStation();
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

    /** Fills the detail panel for the station at `index`: its tile, slogan, genre, bitrate and mark */
    void updatePanel(int index) {
        RadioStation* all;
        const int count = Radio_getStations(&all);
        if (index < 0 || index >= count) return;
        const RadioStation& station = all[index];
        fillStationPanel(panel, makeStationPanelText(station, RadioFavourites_isFavourite(station.url) ? "Favourite" : ""));
    }

    /** Returns the station of the selected row, or nullptr with no station. */
    const RadioStation* getSelectedStation(int& index) {
        index = list.getSelectedIndex();
        RadioStation* all;
        const int count = Radio_getStations(&all);
        return index >= 0 && index < count ? &all[index] : nullptr;
    }

    void playSelectedStation() {
        int index;
        const RadioStation* station = getSelectedStation(index);
        if (!station) return;
        std::string url = station->url;
        getNavigator().openForResult(WifiDialogPage{}, [this, url = std::move(url)](const bool& connected) {
            if (connected) openPlayingScreen(url);
        });
    }

    /** Opens the playing screen, and puts the cursor on the station that it showed when it closes. */
    void openPlayingScreen(std::string url) {
        getNavigator().openForResult(RadioPlayingPage{std::move(url)},
                                     [this](const std::string& shownUrl) { selectStation(shownUrl); });
    }

    void selectStation(const std::string& url) {
        RadioStation* all;
        const int count = Radio_getStations(&all);
        for (int index = 0; index < count; index++) {
            if (url != all[index].url) continue;
            list.setSelectedIndex(index);
            return;
        }
    }

    void manageStations() {
        getNavigator().openForResult(RadioManagePage{}, [this](const std::monostate&) { reloadStations(); });
    }

    /** Y: manage the stations (alone while the list is empty), or remove the one under the cursor. */
    void openOptions() {
        int index;
        const RadioStation* station = getSelectedStation(index);
        if (!station) {
            manageStations();
            return;
        }
        openActionOptions(getNavigator(), THEME_SOURCE_RADIO, "RADIO", "Radio", station->name,
                          {{"Manage stations", [this] { manageStations(); }}, {"Remove station", [this] { confirmRemoval(); }}});
    }

    void confirmRemoval() {
        int index;
        const RadioStation* station = getSelectedStation(index);
        if (!station) return;
        // The list does not change while the dialog shows, thus the index names the station of the dialog
        getNavigator().openForResult(ConfirmDialogPage("Remove Station?", station->name), [this, index](const bool& yes) {
            if (!yes) return;
            if (Radio_removeStation(index)) {
                Radio_saveStations();
                Toast_show("Removed", TOAST_DURATION);
            }
            reloadStations();
        });
    }

    /** Reads the stations again. The cursor keeps its position, or goes to the last row. */
    void reloadStations() {
        const int position = list.getSelectedIndex();
        list.notifyDataChanged();
        if (position >= 0) list.setSelectedIndex(position);
        updatePanel(list.getSelectedIndex());
        updateHints();
    }

    void updateHints() {
        const bool empty = stations.getCount() == 0;
        screenView.getButtonHintsView().setHints(empty ? kEmptyLeftHints : kLeftHints, empty ? kEmptyRightHints : kRightHints);
    }

    StationListAdapter stations;
    ListScreenView screenView{Fonts_getLarge()};
    EmptyStateView emptyState;
    DetailPanelView panel;
    ListScreenController list{screenView, getScheduler(), *this};
    bool radioReady = false;
};

}  // namespace

std::unique_ptr<UiPageImplBase> RadioPage::createPageImpl() const {
    return std::make_unique<RadioPageImpl>();
}

#include "ui/radio_manage_page.hpp"

#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>

#include "radio.h"
#include "radio_favourites.h"
#include "radio_station_name.hpp"
#include "toast.h"
#include "ui/confirm_dialog_page.hpp"
#include "ui/page/list_screen_controller.hpp"
#include "ui/radio_search_page.hpp"
#include "ui/radio_options_page.hpp"
#include "ui/radio_playing_page.hpp"
#include "ui/radio_station_panel.hpp"
#include "ui/text_input_page.hpp"
#include "ui/view/detail_panel_view.hpp"
#include "ui/view/empty_state_view.hpp"
#include "ui/view/list_adapter.hpp"
#include "ui/view/list_row_view.hpp"
#include "ui/view/list_screen_view.hpp"
#include "ui/view/star_icon.hpp"
#include "ui/view/text_view.hpp"
#include "ui/wifi_dialog_page.hpp"
#include "ui_fonts.h"
#include "ui_theme.h"

using namespace input;

namespace {

constexpr const char* kLeftHints[]  = {"Y", "OPTIONS", nullptr};
constexpr const char* kRightHints[] = {"B", "BACK", "X", "FAVOURITE", "A", "LISTEN", nullptr};

/** The rows of the stations. A favourite has a star before its name. */
class ManageAdapter final : public ListAdapter {
public:
    int getCount() const override {
        RadioStation* stations;
        return Radio_getStations(&stations);
    }

    long getItemId(int index) const override {
        RadioStation* stations;
        Radio_getStations(&stations);
        const RadioStation& station = stations[index];
        std::string key = std::string(station.url) + '\n' + station.name + '\n' + station.genre +
                          (RadioFavourites_isFavourite(station.url) ? "\n1" : "\n0") +
                          (Theme_isSourceColoursEnabled() ? "\n1" : "\n0");
        const auto [it, added] = ids.try_emplace(std::move(key), static_cast<long>(ids.size()));
        return it->second;
    }

    std::shared_ptr<ListItemView> getView(int index) override {
        RadioStation* stations;
        Radio_getStations(&stations);
        const RadioStation& station = stations[index];
        auto row = std::make_shared<ListRowView>(Fonts_getMedium());
        row->setText(station.name);
        if (RadioFavourites_isFavourite(station.url)) row->setIcon(getStarIcon(false), getStarIcon(true));
        // The detail panel of the new look carries the genre
        if (station.genre[0] && !Theme_isSourceColoursEnabled()) {
            auto genre = std::make_shared<TextView>(Fonts_getTiny(), false);
            genre->setText(station.genre);
            genre->setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
            row->setRightText(std::move(genre));
        }
        return row;
    }

private:
    mutable std::unordered_map<std::string, long> ids;
};

class RadioManagePageImpl final : public UiPageImpl<RadioManagePage>, private ListScreenListener {
public:
    RadioManagePageImpl() : UiPageImpl(PageKind::FullScreen) {
        screenView.getTitleView().setText("Manage Stations");
        screenView.getTitleView().setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        emptyState.setMessage("No stations saved");
        emptyState.setLine("Press Y to add stations");
        screenView.setEmptyState(&emptyState);
        screenView.getButtonHintsView().setHints(kLeftHints, kRightHints);
    }

    std::span<const ButtonRequest> getButtonRequests() const override {
        return ListScreenController::getButtonRequests();
    }

    HelpId getHelpId() const override { return HELP_RADIO_MANAGE; }
    ThemeSource getSource() const override { return THEME_SOURCE_RADIO; }

    void onEnter() override {
        screenView.setAdapter(stations);
        list.start();
    }

    void layout(UiLayoutContext& context) override {
        screenView.setSidePanel(Theme_isSourceColoursEnabled() ? &panel : nullptr);
        context.layoutChild(screenView, context.getMaxRect());
    }

    bool onButtonEvent(const ButtonEvent& event) override {
        if (list.onButtonEvent(event)) return true;
        if (event.isSingleButton(Button::A)) {
            listen();
            return true;
        }
        if (event.isSingleButton(Button::B)) {
            close();
            return true;
        }
        if (event.isSingleButton(Button::X)) {
            toggleFavourite();
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

    /** Returns the station of the selected row, or nullptr with no station. */
    RadioStation* getSelectedStation() {
        const int index = list.getSelectedIndex();
        RadioStation* all;
        const int count = Radio_getStations(&all);
        return index >= 0 && index < count ? &all[index] : nullptr;
    }

    void updatePanel(int index) {
        RadioStation* all;
        const int count = Radio_getStations(&all);
        if (index < 0 || index >= count) return;
        const RadioStation& station = all[index];
        fillStationPanel(panel, makeStationPanelText(station, RadioFavourites_isFavourite(station.url) ? "Favourite" : ""));
    }

    void listen() {
        const RadioStation* station = getSelectedStation();
        if (!station) return;
        std::string url = station->url;
        getNavigator().openForResult(WifiDialogPage{}, [this, url = std::move(url)](const bool& connected) {
            if (connected) getNavigator().open(RadioPlayingPage{url});
        });
    }

    void toggleFavourite() {
        const RadioStation* station = getSelectedStation();
        if (!station) return;
        const bool now = RadioFavourites_toggleFavourite(station->url);
        Toast_show(now ? "Added to favourites" : "Removed from favourites", TOAST_DURATION);
        reloadKeepingCursor();
    }

    void openOptions() {
        const RadioStation* station = getSelectedStation();
        const std::string url       = station ? station->url : "";
        getNavigator().openForResult(
            RadioOptionsPage{url, station ? station->name : "", station ? station->genre : ""},
            [this, url](const RadioStationAction& action) {
                switch (action) {
                    case RadioStationAction::None:        reloadKeepingCursor(); break;
                    case RadioStationAction::Rename:      rename(); break;
                    case RadioStationAction::Remove:      confirmRemoval(); break;
                    case RadioStationAction::AddStations: addStations(); break;
                }
            });
    }

    void rename() {
        const int index = list.getSelectedIndex();
        getNavigator().openForResult(TextInputPage("Station name", RADIO_MAX_NAME - 1), [this, index](const std::string& typed) {
            RadioStation* all;
            const int count = Radio_getStations(&all);
            const std::string name = cleanStationName(typed, RADIO_MAX_NAME - 1);
            if (name.empty() || index < 0 || index >= count) return;
            std::strncpy(all[index].name, name.c_str(), RADIO_MAX_NAME - 1);
            all[index].name[RADIO_MAX_NAME - 1] = '\0';
            Radio_saveStations();
            Toast_show("Renamed", TOAST_DURATION);
            reloadKeepingCursor();
        });
    }

    void confirmRemoval() {
        const RadioStation* station = getSelectedStation();
        if (!station) return;
        const int index = list.getSelectedIndex();
        // The list does not change while the dialog shows, thus the index names the station of the dialog
        getNavigator().openForResult(ConfirmDialogPage("Remove Station?", station->name), [this, index](const bool& yes) {
            if (!yes) return;
            RadioStation* all;
            const int count = Radio_getStations(&all);
            if (index < 0 || index >= count) return;
            RadioFavourites_forget(all[index].url);
            if (Radio_removeStation(index)) {
                Radio_saveStations();
                Toast_show("Removed", TOAST_DURATION);
            }
            reloadKeepingCursor();
        });
    }

    void addStations() {
        getNavigator().openForResult(RadioSearchPage{}, [this](const std::monostate&) { reloadKeepingCursor(); });
    }

    /** Reads the stations again. The cursor keeps its position, or goes to the last row. */
    void reloadKeepingCursor() {
        const int position = list.getSelectedIndex();
        list.notifyDataChanged();
        if (position >= 0) list.setSelectedIndex(position);
        updatePanel(list.getSelectedIndex());
    }

    ManageAdapter stations;
    ListScreenView screenView{Fonts_getLarge()};
    EmptyStateView emptyState;
    DetailPanelView panel;
    ListScreenController list{screenView, getScheduler(), *this};
};

}  // namespace

std::unique_ptr<UiPageImplBase> RadioManagePage::createPageImpl() const {
    return std::make_unique<RadioManagePageImpl>();
}

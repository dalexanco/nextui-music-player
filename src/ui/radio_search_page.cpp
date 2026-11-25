#include "ui/radio_search_page.hpp"

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "radio.h"
#include "radio_search.hpp"
#include "toast.h"
#include "ui/choice_options_page.hpp"
#include "ui/page/list_screen_controller.hpp"
#include "ui/radio_station_panel.hpp"
#include "ui/text_input_page.hpp"
#include "ui/view/detail_panel_view.hpp"
#include "ui/view/list_adapter.hpp"
#include "ui/view/list_row_view.hpp"
#include "ui/view/list_screen_view.hpp"
#include "ui/view/text_view.hpp"
#include "ui_fonts.h"
#include "ui_theme.h"

using namespace input;

namespace {

constexpr const char* kLeftHints[]       = {"Y", "OPTIONS", nullptr};
constexpr const char* kFieldHints[]      = {"B", "BACK", "A", "SEARCH", nullptr};
constexpr const char* kStationHints[]    = {"B", "BACK", "A", "ADD", nullptr};

/** The most characters of a search */
constexpr size_t kMaxQuery = 64;

/** What the page knows of the directory and of the search, which the adapter and the page share */
struct SearchState {
    std::vector<SearchEntry> entries;
    /** The station of each entry */
    std::vector<const CuratedStation*> stations;
    /** The entries that answer the query */
    std::vector<int> results;
    std::string query;
    /** The country of the filter, or -1 for all of them */
    int countryIndex = -1;

    std::string getCountryCode() const {
        if (countryIndex < 0 || countryIndex >= Radio_getCuratedCountryCount()) return {};
        return Radio_getCuratedCountries()[countryIndex].code;
    }

    void load() {
        const int countryCount = Radio_getCuratedCountryCount();
        const CuratedCountry* countries = Radio_getCuratedCountries();
        for (int c = 0; c < countryCount; c++) {
            int count = 0;
            const CuratedStation* first = Radio_getCuratedStations(countries[c].code, &count);
            for (int i = 0; i < count; i++) {
                const CuratedStation& station = first[i];
                entries.push_back(SearchEntry{station.name, station.genre, station.slogan, countries[c].name, countries[c].code});
                stations.push_back(&station);
            }
        }
        refresh();
    }

    void refresh() { results = searchStations(entries, query, getCountryCode()); }

    /** The row at `index` of the list is the station of the result at `index - 1`. */
    const CuratedStation* getStation(int index) const {
        if (index < 1 || index > static_cast<int>(results.size())) return nullptr;
        return stations[static_cast<size_t>(results[static_cast<size_t>(index - 1)])];
    }

    const SearchEntry* getEntry(int index) const {
        if (index < 1 || index > static_cast<int>(results.size())) return nullptr;
        return &entries[static_cast<size_t>(results[static_cast<size_t>(index - 1)])];
    }

    /** The text at the right of the search field: the country of the filter and the count of results */
    std::string getFieldValue() const {
        const int count = static_cast<int>(results.size());
        std::string text = std::to_string(count) + (count == 1 ? " result" : " results");
        const std::string code = getCountryCode();
        return code.empty() ? text : code + " · " + text;
    }
};

class SearchAdapter final : public ListAdapter {
public:
    explicit SearchAdapter(SearchState& state) : state(state) {}

    int getCount() const override { return 1 + static_cast<int>(state.results.size()); }

    long getItemId(int index) const override {
        std::string key;
        if (index == 0) {
            key = "field\n" + state.query + '\n' + state.getFieldValue();
        } else {
            const CuratedStation* station = state.getStation(index);
            key = std::string(station->url) + (Radio_stationExists(station->url) ? "\n1" : "\n0");
        }
        const auto [it, added] = ids.try_emplace(std::move(key), static_cast<long>(ids.size()));
        return it->second;
    }

    std::shared_ptr<ListItemView> getView(int index) override {
        auto row = std::make_shared<ListRowView>(Fonts_getMedium());
        std::string right;
        if (index == 0) {
            row->setText(state.query.empty() ? "Search stations" : state.query);
            right = state.getFieldValue();
        } else {
            const CuratedStation* station = state.getStation(index);
            row->setText(station->name);
            right = Radio_stationExists(station->url) ? "Added" : station->country_code;
        }
        if (!right.empty()) {
            auto text = std::make_shared<TextView>(Fonts_getTiny(), false);
            text->setText(right);
            text->setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
            row->setRightText(std::move(text));
        }
        return row;
    }

private:
    SearchState& state;
    mutable std::unordered_map<std::string, long> ids;
};

class RadioSearchPageImpl final : public UiPageImpl<RadioSearchPage>, private ListScreenListener {
public:
    RadioSearchPageImpl() : UiPageImpl(PageKind::FullScreen), adapter(state) {
        screenView.getTitleView().setText("Add Stations");
        screenView.getTitleView().setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        screenView.getButtonHintsView().setHints(kLeftHints, kFieldHints);
    }

    std::span<const ButtonRequest> getButtonRequests() const override {
        return ListScreenController::getButtonRequests();
    }

    HelpId getHelpId() const override { return HELP_RADIO_BROWSE; }
    ThemeSource getSource() const override { return THEME_SOURCE_RADIO; }

    void onEnter() override {
        state.load();
        screenView.setAdapter(adapter);
        list.start();
    }

    void layout(UiLayoutContext& context) override {
        screenView.setSidePanel(&panel);
        context.layoutChild(screenView, context.getMaxRect());
    }

    bool onButtonEvent(const ButtonEvent& event) override {
        if (list.onButtonEvent(event)) return true;
        if (event.isSingleButton(Button::A)) {
            if (list.getSelectedIndex() <= 0) {
                openKeyboard();
            } else {
                addSelected();
            }
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
    void onSelectionChanged(int index) override {
        updatePanel(index);
        screenView.getButtonHintsView().setHints(kLeftHints, index <= 0 ? kFieldHints : kStationHints);
    }

    void updatePanel(int index) {
        const CuratedStation* station = state.getStation(index);
        if (!station) {
            panel.getArt().setFallback(THEME_SOURCE_RADIO);
            panel.getInfo().setLine("Name, genre or country");
            panel.getInfo().setPosition(std::to_string(state.entries.size()) + " stations in the directory");
            panel.getInfo().setTimes("");
            return;
        }
        const SearchEntry* entry = state.getEntry(index);
        StationPanelText text;
        text.name   = station->name;
        text.line   = station->slogan[0] ? station->slogan : station->name;
        text.detail = formatStationDetail(station->genre, 0);
        if (!entry->countryName.empty()) text.detail += (text.detail.empty() ? "" : " · ") + entry->countryName;
        text.status = Radio_stationExists(station->url) ? "Added" : "New";
        fillStationPanel(panel, text);
    }

    void openKeyboard() {
        getNavigator().openForResult(TextInputPage("Search stations", kMaxQuery), [this](const std::string& typed) {
            // An empty text is a cancel. A space clears the search.
            if (typed.empty()) return;
            state.query = typed;
            reload(0);
        });
    }

    /** Y: the keyboard of the search, and the country of the filter. */
    void openOptions() {
        const std::string code = state.getCountryCode();
        const std::vector<std::string> labels = {"Search with the keyboard",
                                                 "Next country (now " + (code.empty() ? std::string("all") : code) + ")"};
        getNavigator().openForResult(ChoiceOptionsPage(THEME_SOURCE_RADIO, "STATIONS", "Add stations", "Search", labels),
                                     [this](const ChoiceResult& result) {
                                         if (result.index == 0) openKeyboard();
                                         if (result.index == 1) nextCountry();
                                     });
    }

    /** All the countries, then each country in turn */
    void nextCountry() {
        const int countries = Radio_getCuratedCountryCount();
        state.countryIndex  = state.countryIndex + 1 >= countries ? -1 : state.countryIndex + 1;
        reload(0);
    }

    void addSelected() {
        const CuratedStation* station = state.getStation(list.getSelectedIndex());
        if (!station) return;
        if (Radio_stationExists(station->url)) {
            Toast_show("Already in your stations", TOAST_DURATION);
            return;
        }
        if (Radio_addStation(station->name, station->url, station->genre, station->slogan) < 0) {
            Toast_show("Maximum 32 stations reached", TOAST_DURATION);
            return;
        }
        Radio_saveStations();
        Toast_show((std::string("Added: ") + station->name).c_str(), TOAST_DURATION);
        reload(list.getSelectedIndex());
    }

    /** Searches again and puts the cursor on `index`, or on the last row */
    void reload(int index) {
        state.refresh();
        list.notifyDataChanged();
        list.setSelectedIndex(index);
        updatePanel(list.getSelectedIndex());
    }

    SearchState state;
    SearchAdapter adapter;
    ListScreenView screenView{Fonts_getLarge()};
    DetailPanelView panel;
    ListScreenController list{screenView, getScheduler(), *this};
};

}  // namespace

std::unique_ptr<UiPageImplBase> RadioSearchPage::createPageImpl() const {
    return std::make_unique<RadioSearchPageImpl>();
}

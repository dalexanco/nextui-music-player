#include "ui/radio_stations_page.hpp"

#include <cstring>

#include "radio.h"

namespace {

class RadioStationsSource final : public SubListSource {
public:
    RadioStationsSource(std::function<std::string()> getCurrentUrl, std::function<void(const std::string&)> onPick)
        : getCurrentUrl(std::move(getCurrentUrl)), onPick(std::move(onPick)) {}

    int getCount() const override {
        RadioStation* stations;
        return Radio_getStations(&stations);
    }

    std::string getRowText(int index) const override {
        const RadioStation* station = get(index);
        return station ? station->name : std::string();
    }

    std::string getRowValue(int index) const override {
        const RadioStation* station = get(index);
        return station ? station->genre : std::string();
    }

    bool isPlaying(int index) const override {
        const RadioStation* station = get(index);
        return station && station->url == getCurrentUrl();
    }

    int getInitialIndex() const override {
        for (int index = 0; index < getCount(); index++) {
            if (isPlaying(index)) return index;
        }
        return 0;
    }

    std::string getEmptyMessage() const override { return "No stations saved"; }

    bool onActivate(int index) override {
        const RadioStation* station = get(index);
        if (!station) return false;
        onPick(station->url);
        return true;
    }

private:
    static const RadioStation* get(int index) {
        RadioStation* stations;
        const int count = Radio_getStations(&stations);
        return index >= 0 && index < count ? &stations[index] : nullptr;
    }

    std::function<std::string()> getCurrentUrl;
    std::function<void(const std::string&)> onPick;
};

}  // namespace

SubListPage makeRadioStationsPage(std::function<std::string()> getCurrentUrl,
                                  std::function<void(const std::string&)> onPick) {
    return SubListPage("Radio", "STATIONS", "LISTEN", THEME_SOURCE_RADIO,
                       std::make_shared<RadioStationsSource>(std::move(getCurrentUrl), std::move(onPick)),
                       HELP_RADIO_STATIONS);
}

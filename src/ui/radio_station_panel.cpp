#include "ui/radio_station_panel.hpp"

#include <cstring>

int getStationBitrate(const char* url) {
    if (!url || !Radio_isActive()) return 0;
    const char* current = Radio_getCurrentUrl();
    if (!current || std::strcmp(current, url) != 0) return 0;
    const RadioMetadata* meta = Radio_getMetadata();
    return meta ? meta->bitrate : 0;
}

std::string formatStationDetail(const char* genre, int bitrate) {
    std::string detail = genre ? genre : "";
    if (bitrate > 0) {
        if (!detail.empty()) detail += " · ";
        detail += std::to_string(bitrate) + " kbps";
    }
    return detail;
}

StationPanelText makeStationPanelText(const RadioStation& station, const std::string& status) {
    StationPanelText text;
    text.name   = station.name;
    text.line   = station.slogan[0] ? station.slogan : station.name;
    text.detail = formatStationDetail(station.genre, getStationBitrate(station.url));
    text.status = status;
    return text;
}

void fillStationPanel(DetailPanelView& panel, const StationPanelText& text) {
    panel.getArt().setFallback(THEME_SOURCE_RADIO);
    panel.getInfo().setLine(text.line);
    panel.getInfo().setPosition(text.detail);
    panel.getInfo().setTimes(text.status);
}

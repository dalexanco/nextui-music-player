#ifndef __UI_RADIO_STATION_PANEL_HPP__
#define __UI_RADIO_STATION_PANEL_HPP__

#include <string>

#include "radio.h"
#include "ui/view/detail_panel_view.hpp"

/** What the detail panel of a station says. The lists of the radio share it. */
struct StationPanelText {
    /** The title of the tile of the logo */
    std::string name;
    /** The slogan of the station, or its name for a station with none */
    std::string line;
    /** The genre and the bitrate: "Pop · 128 kbps". The bitrate shows where it is known. */
    std::string detail;
    /** The state of the station in the stations of the user: "Favourite", or nothing */
    std::string status;
};

/**
 * Returns the bitrate in kbps of the stream of `url` while the radio plays it, else 0. A station
 * that does not play has no known bitrate.
 */
int getStationBitrate(const char* url);

/** Returns "genre · 128 kbps", with the parts that exist. */
std::string formatStationDetail(const char* genre, int bitrate);

/** Returns the texts of the panel for `station`. `status` is the text at the right of the detail. */
StationPanelText makeStationPanelText(const RadioStation& station, const std::string& status);

/** Puts the texts in the panel. The tile has no logo, thus it is the generated tile of the radio. */
void fillStationPanel(DetailPanelView& panel, const StationPanelText& text);

#endif

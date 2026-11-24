#ifndef __UI_RADIO_STATIONS_PAGE_HPP__
#define __UI_RADIO_STATIONS_PAGE_HPP__

#include <functional>
#include <string>

#include "ui/sub_list_page.hpp"

/**
 * The stations of the user as a sub-list, reached with X from the radio player. The station that the
 * player shows has the playing indicator. A calls `onPick` with the URL of the station and closes
 * the page, and the owner starts that station.
 */
SubListPage makeRadioStationsPage(std::function<std::string()> getCurrentUrl,
                                  std::function<void(const std::string&)> onPick);

#endif

#ifndef __RADIO_STATION_NAME_HPP__
#define __RADIO_STATION_NAME_HPP__

#include <cstddef>
#include <string>

/**
 * Cleans the name that the user typed for a station: the ends lose their spaces, the characters
 * that the station file uses as separators or that break a line become spaces, and the name ends
 * on a character before `maxBytes`. Returns an empty text when nothing is left, which means that
 * the user cancelled.
 */
std::string cleanStationName(const std::string& typed, size_t maxBytes);

#endif

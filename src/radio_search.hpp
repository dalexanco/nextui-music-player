#ifndef __RADIO_SEARCH_HPP__
#define __RADIO_SEARCH_HPP__

#include <string>
#include <string_view>
#include <vector>

/** What the search reads of a station of the directory */
struct SearchEntry {
    std::string name;
    std::string genre;
    std::string slogan;
    std::string countryName;
    std::string countryCode;
};

/**
 * Returns true when `entry` answers `query`. The query is cut at its spaces into words, and each
 * word must be in the name, the genre, the slogan or the country name. The case of the letters
 * does not count, for the letters of ASCII. An empty query answers every entry.
 */
bool matchesQuery(const SearchEntry& entry, std::string_view query);

/**
 * Returns the positions in `entries` of the stations that answer `query`, sorted by name without
 * the case. A `countryCode` that is not empty keeps only the stations of that country.
 */
std::vector<int> searchStations(const std::vector<SearchEntry>& entries, std::string_view query,
                                std::string_view countryCode);

#endif

#include "radio_search.hpp"

#include <algorithm>
#include <cctype>

namespace {

char lower(char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }

bool containsFolded(std::string_view text, std::string_view word) {
    const auto found = std::search(text.begin(), text.end(), word.begin(), word.end(),
                                   [](char a, char b) { return lower(a) == lower(b); });
    return found != text.end();
}

bool lessByName(const std::string& a, const std::string& b) {
    return std::lexicographical_compare(a.begin(), a.end(), b.begin(), b.end(),
                                        [](char x, char y) { return lower(x) < lower(y); });
}

}  // namespace

bool matchesQuery(const SearchEntry& entry, std::string_view query) {
    size_t at = 0;
    while (at < query.size()) {
        while (at < query.size() && std::isspace(static_cast<unsigned char>(query[at]))) at++;
        const size_t start = at;
        while (at < query.size() && !std::isspace(static_cast<unsigned char>(query[at]))) at++;
        if (at == start) break;
        const std::string_view word = query.substr(start, at - start);
        const bool found = containsFolded(entry.name, word) || containsFolded(entry.genre, word) ||
                           containsFolded(entry.slogan, word) || containsFolded(entry.countryName, word);
        if (!found) return false;
    }
    return true;
}

std::vector<int> searchStations(const std::vector<SearchEntry>& entries, std::string_view query,
                                std::string_view countryCode) {
    std::vector<int> found;
    for (size_t index = 0; index < entries.size(); index++) {
        if (!countryCode.empty() && entries[index].countryCode != countryCode) continue;
        if (matchesQuery(entries[index], query)) found.push_back(static_cast<int>(index));
    }
    std::stable_sort(found.begin(), found.end(), [&](int a, int b) {
        return lessByName(entries[static_cast<size_t>(a)].name, entries[static_cast<size_t>(b)].name);
    });
    return found;
}

#include "radio_station_name.hpp"

std::string cleanStationName(const std::string& typed, size_t maxBytes) {
    std::string name;
    name.reserve(typed.size());
    for (const char c : typed) {
        const bool control = static_cast<unsigned char>(c) < 0x20 || c == 0x7f;
        name.push_back(control || c == '|' ? ' ' : c);
    }
    const size_t first = name.find_first_not_of(' ');
    if (first == std::string::npos) return {};
    name.erase(0, first);
    name.erase(name.find_last_not_of(' ') + 1);

    if (name.size() > maxBytes) {
        size_t end = maxBytes;
        // A byte of the form 10xxxxxx continues a character, thus the cut moves back to its start
        while (end > 0 && (static_cast<unsigned char>(name[end]) & 0xC0) == 0x80) end--;
        name.erase(end);
        const size_t last = name.find_last_not_of(' ');
        if (last == std::string::npos) return {};
        name.erase(last + 1);
    }
    return name;
}

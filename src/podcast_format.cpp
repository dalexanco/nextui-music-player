#include "podcast_format.hpp"

#include <cstdio>

std::string formatRelativeDate(std::time_t now, uint32_t timestamp) {
    if (timestamp == 0) return {};
    const std::time_t published = static_cast<std::time_t>(timestamp);
    const long days             = published >= now ? 0 : static_cast<long>((now - published) / (24 * 3600));
    if (days == 0) return "Today";
    if (days == 1) return "Yesterday";
    if (days < 7) return std::to_string(days) + " days ago";
    if (days < 30) return std::to_string(days / 7) + (days / 7 == 1 ? " week ago" : " weeks ago");
    char buffer[32] = {};
    const std::tm* tm = std::localtime(&published);
    if (!tm || std::strftime(buffer, sizeof(buffer), "%b %d", tm) == 0) return {};
    return buffer;
}

std::string formatEpisodeCounts(int total, int newCount) {
    if (total <= 0) return "No episodes";
    std::string text = std::to_string(total) + (total == 1 ? " episode" : " episodes");
    if (newCount > 0) text += " · " + std::to_string(newCount) + " new";
    return text;
}

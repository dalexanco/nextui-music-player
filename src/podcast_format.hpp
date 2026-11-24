#ifndef __PODCAST_FORMAT_HPP__
#define __PODCAST_FORMAT_HPP__

#include <cstdint>
#include <ctime>
#include <string>

/**
 * How long ago `timestamp` was, for the lists of the podcasts: "Today", "Yesterday", "3 days ago",
 * "2 weeks ago", else the date as "Mar 04". A timestamp of zero (unknown) or in the future gives
 * an empty text, a future one reads as "Today".
 */
std::string formatRelativeDate(std::time_t now, uint32_t timestamp);

/** "42 episodes · 3 new", "1 episode", "No episodes". The new part shows only where it is above 0. */
std::string formatEpisodeCounts(int total, int newCount);

#endif

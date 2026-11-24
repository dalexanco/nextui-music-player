#ifndef __UI_PODCAST_EPISODES_PAGE_HPP__
#define __UI_PODCAST_EPISODES_PAGE_HPP__

#include <functional>

#include "ui/sub_list_page.hpp"

/**
 * The episodes of the subscription at `feedIndex`, as a sub-list: a star on the new episodes, a
 * check on the listened ones, the indicator on the episode that plays, and no duration. A plays
 * (a downloaded episode), downloads (else) or cancels the download, X marks the episode as
 * listened or not, Y looks for new episodes. A plays through the player page. With `onPick` (the
 * list that the player opens) A on a downloaded episode calls it with the index of the episode
 * and closes the list, and the player takes the episode.
 */
SubListPage makePodcastEpisodesPage(int feedIndex, std::function<void(int)> onPick = nullptr);

#endif

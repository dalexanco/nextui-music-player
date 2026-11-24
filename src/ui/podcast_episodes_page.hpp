#ifndef __UI_PODCAST_EPISODES_PAGE_HPP__
#define __UI_PODCAST_EPISODES_PAGE_HPP__

#include "ui/sub_list_page.hpp"

/**
 * The episodes of the subscription at `feedIndex`, as a sub-list: a star on the new episodes, a
 * check on the listened ones, the indicator on the episode that plays, and no duration. A plays
 * (a downloaded episode), downloads (else) or cancels the download, X marks the episode as
 * listened or not, Y looks for new episodes.
 */
SubListPage makePodcastEpisodesPage(int feedIndex);

#endif

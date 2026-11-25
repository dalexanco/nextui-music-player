#ifndef __UI_PODCAST_PLAYING_PAGE_HPP__
#define __UI_PODCAST_PLAYING_PAGE_HPP__

#include <memory>
#include <variant>

#include "ui/page/ui_page.hpp"

/**
 * The playing screen of the podcasts. Starts the episode `episodeIndex` of the subscription
 * `feedIndex` (it seeks to the saved position first), or shows the episode that plays in the
 * background when both are -1.
 */
class PodcastPlayingPage final : public UiPage {
public:
    using Result = std::monostate;

    explicit PodcastPlayingPage(int feedIndex = -1, int episodeIndex = -1) : feedIndex(feedIndex), episodeIndex(episodeIndex) {}

    std::unique_ptr<UiPageImplBase> createPageImpl() const override;

private:
    int feedIndex;
    int episodeIndex;
};

/** Creates the playing screen of the podcasts for NowPlayingPage, with the episode that plays in the background. */
std::unique_ptr<UiPageImplBase> createPodcastPlayerPageImpl();

#endif

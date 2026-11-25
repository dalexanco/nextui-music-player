#ifndef __UI_PODCAST_PAGE_HPP__
#define __UI_PODCAST_PAGE_HPP__

#include <memory>
#include <variant>

#include "ui/page/ui_page.hpp"

/**
 * The Podcasts row of Main: the episodes to continue, the subscriptions with a detail panel in the
 * new look, and the downloads while there are some. A opens, X unsubscribes, Y manages.
 */
class PodcastPage final : public UiPage {
public:
    using Result = std::monostate;
    std::unique_ptr<UiPageImplBase> createPageImpl() const override;
};

#endif

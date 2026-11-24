#ifndef __UI_PODCAST_MANAGE_PAGE_HPP__
#define __UI_PODCAST_MANAGE_PAGE_HPP__

#include <memory>
#include <variant>

#include "ui/page/ui_page.hpp"

/**
 * Manage Podcasts (Y of the shows): Search asks for a text on the keyboard and lists the answer,
 * Top Shows lists the charts, and Import from SD card subscribes to the feeds of the files in
 * Podcasts/Subscriptions. All need the network.
 */
class PodcastManagePage final : public UiPage {
public:
    using Result = std::monostate;
    std::unique_ptr<UiPageImplBase> createPageImpl() const override;
};

#endif

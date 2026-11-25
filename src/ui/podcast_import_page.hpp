#ifndef __UI_PODCAST_IMPORT_PAGE_HPP__
#define __UI_PODCAST_IMPORT_PAGE_HPP__

#include <memory>
#include <variant>

#include "ui/page/ui_page.hpp"

/**
 * Import from SD card (an entry of Manage Podcasts): subscribes to the feeds that the user dropped
 * in Podcasts/Subscriptions of the card, and lists what became of each one: added, already
 * subscribed, or failed with the reason. The import runs on a thread of the podcast module, thus the
 * page stays alive; a import that the Podcasts page started by itself shows here too. Open it after
 * the Wi-Fi dialog gave a connection.
 */
class PodcastImportPage final : public UiPage {
public:
    using Result = std::monostate;
    std::unique_ptr<UiPageImplBase> createPageImpl() const override;
};

#endif

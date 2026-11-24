#ifndef __UI_PODCAST_LEGACY_PAGE_HPP__
#define __UI_PODCAST_LEGACY_PAGE_HPP__

#include <memory>
#include <variant>

#include "module_podcast.h"
#include "ui/page/ui_page.hpp"

/**
 * Runs the legacy module of the podcasts on one of its screens, for the screens that no native
 * page replaces yet. The module returns where it would go back to its menu.
 */
class PodcastLegacyPage final : public UiPage {
public:
    using Result = std::monostate;

    PodcastLegacyPage(PodcastEntry entry, int arg = 0) : entry(entry), arg(arg) {}

    std::unique_ptr<UiPageImplBase> createPageImpl() const override;

private:
    PodcastEntry entry;
    int arg;
};

#endif

#ifndef __UI_PODCAST_LEGACY_PAGE_HPP__
#define __UI_PODCAST_LEGACY_PAGE_HPP__

#include <memory>
#include <variant>

#include "module_podcast.h"
#include "ui/page/ui_page.hpp"

/** Runs the legacy module of the podcasts: Manage, Search and Top Shows, until their own pages. */
class PodcastLegacyPage final : public UiPage {
public:
    using Result = std::monostate;

    std::unique_ptr<UiPageImplBase> createPageImpl() const override;
};

#endif

#ifndef __UI_PODCAST_CATALOG_PAGE_HPP__
#define __UI_PODCAST_CATALOG_PAGE_HPP__

#include <memory>
#include <variant>

#include "ui/page/ui_page.hpp"

/** What a catalogue lists: the answer to a search, or the charts of the shows */
enum class PodcastCatalogKind : uint8_t { SearchResults, TopShows };

/**
 * A list of shows that the user can subscribe to, read from the search or from the charts of the
 * podcast module (which the caller started: `Podcast_startSearch()` or `Podcast_loadCharts()`).
 * A subscribes to the show, or asks to unsubscribe from one that is subscribed. X refreshes the
 * charts. The detail panel of the new look carries the author, the genre and the cover.
 */
class PodcastCatalogPage final : public UiPage {
public:
    using Result = std::monostate;

    explicit PodcastCatalogPage(PodcastCatalogKind kind) : kind(kind) {}

    std::unique_ptr<UiPageImplBase> createPageImpl() const override;

private:
    PodcastCatalogKind kind;
};

#endif

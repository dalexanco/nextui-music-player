#ifndef __UI_PODCAST_QUEUE_PAGE_HPP__
#define __UI_PODCAST_QUEUE_PAGE_HPP__

#include <memory>
#include <variant>

#include "ui/page/ui_page.hpp"

/**
 * The download queue of the episodes: the title of each, and at the right its state (`45 %`,
 * `Queued`, `Failed`, `Done`). X removes the selected download or its row, B goes back.
 */
class PodcastQueuePage final : public UiPage {
public:
    using Result = std::monostate;
    std::unique_ptr<UiPageImplBase> createPageImpl() const override;
};

#endif

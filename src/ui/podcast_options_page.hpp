#ifndef __UI_PODCAST_OPTIONS_PAGE_HPP__
#define __UI_PODCAST_OPTIONS_PAGE_HPP__

#include <cstdint>
#include <memory>
#include <string>

#include "ui/page/ui_page.hpp"

/** What the user chose in the options of an episode, for the player to carry out. */
enum class PodcastPlayerAction : uint8_t { None, MarkListened, DeleteFile };

/**
 * The options of the episode that plays (Y): speed and sleep timer apply at once, `Mark as
 * listened` and `Delete file` end the page with their action in the result. A full page with the
 * options panel at the right, as the options of the radio.
 */
class PodcastOptionsPage final : public UiPage {
public:
    using Result = PodcastPlayerAction;

    PodcastOptionsPage(std::string show, std::string episode) : show(std::move(show)), episode(std::move(episode)) {}

    std::unique_ptr<UiPageImplBase> createPageImpl() const override;

private:
    std::string show;
    std::string episode;
};

#endif

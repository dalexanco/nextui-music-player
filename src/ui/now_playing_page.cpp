#include "ui/now_playing_page.hpp"

#include "background.h"
#include "ui/music_player_page.hpp"
#include "ui/page/legacy_module_page.hpp"
#include "ui/podcast_playing_page.hpp"
#include "ui/radio_playing_page.hpp"

namespace {

/** Nothing plays: the page closes at once. */
class NowPlayingPageImpl final : public LegacyModulePageImpl<NowPlayingPage> {
protected:
    ModuleExitReason runModule(DisplayContext*) override { return MODULE_EXIT_TO_MENU; }
};

}  // namespace

std::unique_ptr<UiPageImplBase> NowPlayingPage::createPageImpl() const {
    // Each source has its native playing screen
    if (Background_getActive() == BG_MUSIC) return createMusicPlayerPageImpl();
    if (Background_getActive() == BG_RADIO) return createRadioPlayerPageImpl();
    if (Background_getActive() == BG_PODCAST) return createPodcastPlayerPageImpl();
    return std::make_unique<NowPlayingPageImpl>();
}

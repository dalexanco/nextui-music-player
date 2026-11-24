#include "ui/now_playing_page.hpp"

#include "background.h"
#include "module_podcast.h"
#include "ui/music_player_page.hpp"
#include "ui/page/legacy_module_page.hpp"

namespace {

class NowPlayingPageImpl final : public LegacyModulePageImpl<NowPlayingPage> {
protected:
    ModuleExitReason runModule(DisplayContext* display) override {
        switch (Background_getActive()) {
            case BG_PODCAST: return PodcastModule_run(display);
            default:         return MODULE_EXIT_TO_MENU;
        }
    }

};

}  // namespace

std::unique_ptr<UiPageImplBase> NowPlayingPage::createPageImpl() const {
    // The music has its own native page. A podcast still runs its module.
    if (Background_getActive() == BG_MUSIC) return createMusicPlayerPageImpl();
    return std::make_unique<NowPlayingPageImpl>();
}

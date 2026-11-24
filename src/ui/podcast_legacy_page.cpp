#include "ui/podcast_legacy_page.hpp"

#include "ui/page/legacy_module_page.hpp"

namespace {

class PodcastLegacyPageImpl final : public LegacyModulePageImpl<PodcastLegacyPage> {
public:
protected:
    ModuleExitReason runModule(DisplayContext* display) override {
        return PodcastModule_run(display);
    }
};

}  // namespace

std::unique_ptr<UiPageImplBase> PodcastLegacyPage::createPageImpl() const {
    return std::make_unique<PodcastLegacyPageImpl>();
}

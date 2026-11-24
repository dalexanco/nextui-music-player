#include "ui/podcast_legacy_page.hpp"

#include "ui/page/legacy_module_page.hpp"

namespace {

class PodcastLegacyPageImpl final : public LegacyModulePageImpl<PodcastLegacyPage> {
public:
    explicit PodcastLegacyPageImpl(PodcastEntry entry) : entry(entry) {}

protected:
    ModuleExitReason runModule(DisplayContext* display) override {
        PodcastModule_setEntry(entry);
        return PodcastModule_run(display);
    }

private:
    PodcastEntry entry;
};

}  // namespace

std::unique_ptr<UiPageImplBase> PodcastLegacyPage::createPageImpl() const {
    return std::make_unique<PodcastLegacyPageImpl>(entry);
}

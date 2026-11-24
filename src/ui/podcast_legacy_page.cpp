#include "ui/podcast_legacy_page.hpp"

#include "ui/page/legacy_module_page.hpp"

namespace {

class PodcastLegacyPageImpl final : public LegacyModulePageImpl<PodcastLegacyPage> {
public:
    PodcastLegacyPageImpl(PodcastEntry entry, int arg) : entry(entry), arg(arg) {}

protected:
    ModuleExitReason runModule(DisplayContext* display) override {
        PodcastModule_setEntry(entry, arg);
        return PodcastModule_run(display);
    }

private:
    PodcastEntry entry;
    int arg;
};

}  // namespace

std::unique_ptr<UiPageImplBase> PodcastLegacyPage::createPageImpl() const {
    return std::make_unique<PodcastLegacyPageImpl>(entry, arg);
}

#include "ui/podcast_legacy_page.hpp"

#include "ui/page/legacy_module_page.hpp"

namespace {

class PodcastLegacyPageImpl final : public LegacyModulePageImpl<PodcastLegacyPage> {
public:
    PodcastLegacyPageImpl(PodcastEntry entry, int arg, int arg2) : entry(entry), arg(arg), arg2(arg2) {}

protected:
    ModuleExitReason runModule(DisplayContext* display) override {
        PodcastModule_setEntry(entry, arg, arg2);
        return PodcastModule_run(display);
    }

private:
    PodcastEntry entry;
    int arg;
    int arg2;
};

}  // namespace

std::unique_ptr<UiPageImplBase> PodcastLegacyPage::createPageImpl() const {
    return std::make_unique<PodcastLegacyPageImpl>(entry, arg, arg2);
}

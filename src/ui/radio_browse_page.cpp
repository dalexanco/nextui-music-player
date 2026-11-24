#include "ui/radio_browse_page.hpp"

#include "module_radio_manage.h"
#include "ui/page/legacy_module_page.hpp"

namespace {

class RadioBrowsePageImpl final : public LegacyModulePageImpl<RadioBrowsePage> {
protected:
    ModuleExitReason runModule(DisplayContext* display) override { return RadioManageModule_run(display); }
};

}  // namespace

std::unique_ptr<UiPageImplBase> RadioBrowsePage::createPageImpl() const {
    return std::make_unique<RadioBrowsePageImpl>();
}

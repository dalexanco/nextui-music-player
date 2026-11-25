#include "ui/text_input_page.hpp"

#include <cstdlib>

#include "keyboard.h"
#include "ui/page/legacy_module_page.hpp"

namespace {

class TextInputPageImpl final : public LegacyModulePageImpl<TextInputPage> {
public:
    TextInputPageImpl(std::string prompt, size_t maxBytes) : prompt(std::move(prompt)), maxBytes(maxBytes) {}

protected:
    ModuleExitReason runModule(DisplayContext*) override {
        char* typed = Keyboard_open(prompt.c_str(), maxBytes);
        if (typed) {
            text = typed;
            free(typed);
        }
        return MODULE_EXIT_BACK;
    }

    std::string getModuleResult() override { return text; }

private:
    std::string prompt;
    size_t maxBytes;
    std::string text;
};

}  // namespace

std::unique_ptr<UiPageImplBase> TextInputPage::createPageImpl() const {
    return std::make_unique<TextInputPageImpl>(prompt, maxBytes);
}

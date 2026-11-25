#ifndef __UI_TEXT_INPUT_PAGE_HPP__
#define __UI_TEXT_INPUT_PAGE_HPP__

#include <memory>
#include <string>

#include "ui/page/ui_page.hpp"

/**
 * Asks the user for a text on the on-screen keyboard. The keyboard runs its own frame loop, thus
 * this page is a legacy page: it takes the screen until the user is done.
 */
class TextInputPage final : public UiPage {
public:
    /** The text that the user typed, or an empty text when the user cancelled or typed nothing */
    using Result = std::string;

    TextInputPage(std::string prompt, size_t maxBytes) : prompt(std::move(prompt)), maxBytes(maxBytes) {}

    std::unique_ptr<UiPageImplBase> createPageImpl() const override;

private:
    std::string prompt;
    size_t maxBytes;
};

#endif

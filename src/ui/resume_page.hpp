#ifndef __UI_RESUME_PAGE_HPP__
#define __UI_RESUME_PAGE_HPP__

#include <memory>
#include <variant>

#include "ui/page/ui_page.hpp"

/** Plays the Resume record of Main and shows the player. Closes when the player closes. */
class ResumePage final : public UiPage {
public:
    using Result = std::monostate;
    std::unique_ptr<UiPageImplBase> createPageImpl() const override;
};

#endif

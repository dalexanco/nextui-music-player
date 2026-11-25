#ifndef __UI_RADIO_MANAGE_PAGE_HPP__
#define __UI_RADIO_MANAGE_PAGE_HPP__

#include <memory>
#include <variant>

#include "ui/page/ui_page.hpp"

/**
 * Manage Stations: the stations of the user with a star on the favourites. A listens to a station,
 * X marks it as favourite, and Y opens its options (rename, remove, add stations).
 */
class RadioManagePage final : public UiPage {
public:
    using Result = std::monostate;
    std::unique_ptr<UiPageImplBase> createPageImpl() const override;
};

#endif

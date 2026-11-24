#ifndef __UI_RADIO_BROWSE_PAGE_HPP__
#define __UI_RADIO_BROWSE_PAGE_HPP__

#include <memory>
#include <variant>

#include "ui/page/ui_page.hpp"

/**
 * Runs the directory of stations by country, where the user adds a station to the station list or
 * removes one. Still the legacy module, until the search page replaces it.
 */
class RadioBrowsePage final : public UiPage {
public:
    using Result = std::monostate;
    std::unique_ptr<UiPageImplBase> createPageImpl() const override;
};

#endif

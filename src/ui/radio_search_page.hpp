#ifndef __UI_RADIO_SEARCH_PAGE_HPP__
#define __UI_RADIO_SEARCH_PAGE_HPP__

#include <memory>
#include <variant>

#include "ui/page/ui_page.hpp"

/**
 * Add Stations: searches the directory of stations by name, genre, slogan or country. The first row
 * is the search field, and A or Y opens the keyboard. A on a station adds it to the stations of
 * the user. X goes through the countries.
 */
class RadioSearchPage final : public UiPage {
public:
    using Result = std::monostate;
    std::unique_ptr<UiPageImplBase> createPageImpl() const override;
};

#endif

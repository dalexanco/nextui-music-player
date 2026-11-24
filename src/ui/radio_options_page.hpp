#ifndef __UI_RADIO_OPTIONS_PAGE_HPP__
#define __UI_RADIO_OPTIONS_PAGE_HPP__

#include <cstdint>
#include <memory>
#include <string>

#include "ui/page/ui_page.hpp"

/** What the user chose in the options of a station, for the page that opened them to carry out. */
enum class RadioStationAction : uint8_t { None, Rename, Remove, AddStations };

/**
 * The options of a station (Y): favourite, rename, remove, and the way to add stations. A full
 * page with the panel of the options at the right, as the options of the music. The favourite is
 * changed here; the other rows end the page with the action in the result.
 */
class RadioOptionsPage final : public UiPage {
public:
    using Result = RadioStationAction;

    /** Pass an empty `url` where the list has no station: only the way to add stations shows. */
    RadioOptionsPage(std::string url, std::string name, std::string genre)
        : url(std::move(url)), name(std::move(name)), genre(std::move(genre)) {}

    std::unique_ptr<UiPageImplBase> createPageImpl() const override;

private:
    std::string url;
    std::string name;
    std::string genre;
};

#endif

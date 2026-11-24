#ifndef __UI_PODCAST_SHOW_PANEL_HPP__
#define __UI_PODCAST_SHOW_PANEL_HPP__

#include <string>

#include "podcast.h"
#include "ui/view/detail_panel_view.hpp"

/** What the detail panel of a show says. The lists of the podcasts share it. */
struct ShowPanelText {
    /** The title of the tile of the art */
    std::string name;
    /** The last episode of the show, or its author */
    std::string line;
    /** The count of the episodes: "42 episodes · 3 new" */
    std::string detail;
    /** The date of the last update: "Updated 3 days ago", or a status */
    std::string status;
    /** The id of the feed whose art the tile shows, or an empty text for the generated tile */
    std::string feedId;
};

/** Returns the texts of the panel for the subscription at `index`. */
ShowPanelText makeShowPanelText(int index);

/** Puts the texts in the panel: the art of the feed where it is on the disk, else the tile. */
void fillShowPanel(DetailPanelView& panel, const ShowPanelText& text);

#endif

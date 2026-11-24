#include "ui/podcast_show_panel.hpp"

#include <cstdio>
#include <ctime>

#include "podcast_format.hpp"

ShowPanelText makeShowPanelText(int index) {
    ShowPanelText text;
    const PodcastFeed* feed = Podcast_getSubscription(index);
    if (!feed) return text;
    text.name   = feed->title;
    text.feedId = feed->feed_id;
    // The last episode is the first of the list
    const PodcastEpisode* last = feed->episode_count > 0 ? Podcast_getEpisode(index, 0) : nullptr;
    text.line   = last && last->title[0] ? last->title : feed->author;
    text.detail = formatEpisodeCounts(feed->episode_count, feed->new_episode_count);
    const std::string when = formatRelativeDate(std::time(nullptr), feed->last_updated);
    text.status = when.empty() ? std::string() : "Updated " + when;
    return text;
}

void fillShowPanel(DetailPanelView& panel, const ShowPanelText& text) {
    panel.getArt().setFallback(text.name, THEME_SOURCE_PODCAST);
    bool loaded = false;
    if (!text.feedId.empty()) {
        char directory[512];
        Podcast_getFeedDataPath(text.feedId.c_str(), directory, sizeof(directory));
        char path[768];
        std::snprintf(path, sizeof(path), "%s/artwork.jpg", directory);
        // A feed with no art on the disk keeps the tile
        if (std::FILE* file = std::fopen(path, "rb")) {
            std::fclose(file);
            loaded = panel.getArt().loadFile(path);
        }
    }
    if (!loaded) panel.getArt().setBitmap(nullptr);
    panel.getInfo().setLine(text.line);
    panel.getInfo().setPosition(text.detail);
    panel.getInfo().setTimes(text.status);
}

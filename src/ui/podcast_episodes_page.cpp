#include "ui/podcast_episodes_page.hpp"

#include <memory>
#include <string>
#include <vector>

#include <functional>
#include "podcast.h"
#include "toast.h"
#include "ui/podcast_playing_page.hpp"
#include "ui/wifi_dialog_page.hpp"

namespace {

/** The most episodes of one feed that the list holds */
constexpr int kMaxEpisodes = 2000;

/** The episodes of a subscription, read once into briefs: the rows must not read the file again. */
class PodcastEpisodesSource final : public SubListSource {
public:
    PodcastEpisodesSource(int feedIndex, std::function<void(int)> onPick)
        : feedIndex(feedIndex), onPick(std::move(onPick)), briefs(kMaxEpisodes) {
        read();
    }

    int getCount() const override { return count; }

    std::string getRowText(int index) const override {
        return index >= 0 && index < count ? briefs[static_cast<size_t>(index)].title : std::string();
    }

    /** A download that runs, waits or failed. The duration is not a value of the rows. */
    std::string getRowValue(int index) const override {
        int progress = 0;
        switch (getDownloadStatus(index, progress)) {
            case PODCAST_DOWNLOAD_DOWNLOADING: return std::to_string(progress) + " %";
            case PODCAST_DOWNLOAD_PENDING:     return "Queued";
            case PODCAST_DOWNLOAD_FAILED:      return "Failed";
            default:                           return {};
        }
    }

    SubListMark getRowMark(int index) const override {
        if (index < 0 || index >= count) return SubListMark::None;
        const PodcastEpisodeBrief& brief = briefs[static_cast<size_t>(index)];
        if (brief.is_new) return SubListMark::New;
        return brief.progress_sec == -1 ? SubListMark::Done : SubListMark::None;
    }

    bool isPlaying(int index) const override {
        return Podcast_isActive() && Podcast_getCurrentFeedIndex() == feedIndex && Podcast_getCurrentEpisodeIndex() == index;
    }

    int getInitialIndex() const override {
        for (int index = 0; index < count; index++) {
            if (isPlaying(index)) return index;
        }
        return 0;
    }

    std::string getEmptyMessage() const override { return "No episodes available"; }

    std::string getVerb(int index) const override {
        int progress = 0;
        const int status = getDownloadStatus(index, progress);
        if (status == PODCAST_DOWNLOAD_DOWNLOADING || status == PODCAST_DOWNLOAD_PENDING) return "CANCEL";
        const PodcastFeed* feed = Podcast_getSubscription(feedIndex);
        if (!feed || index < 0 || index >= count || !Podcast_episodeFileExists(const_cast<PodcastFeed*>(feed), index)) return "DOWNLOAD";
        return briefs[static_cast<size_t>(index)].progress_sec > 0 ? "RESUME" : "PLAY";
    }

    long getVersion() const override {
        // The downloads of this feed: their state moves the rows and the label of A
        long version = localVersion * 1000003L;
        int queueCount = 0;
        const PodcastDownloadItem* queue = Podcast_getDownloadQueue(&queueCount);
        const PodcastFeed* feed          = Podcast_getSubscription(feedIndex);
        for (int index = 0; feed && index < queueCount; index++) {
            if (std::string(queue[index].feed_url) != feed->feed_url) continue;
            version = version * 31 + queue[index].status * 101 + queue[index].progress_percent / 10;
        }
        return version;
    }

    std::vector<SubListKey> getKeys() const override {
        return {SubListKey{input::Button::Y, "Refresh the episodes"}, SubListKey{input::Button::X, "Mark as played"}};
    }

    std::string getKeyLabel(const SubListKey& key, int index) const override {
        if (key.button != input::Button::X) return key.label;
        // The row that is listened to goes back to new
        return index >= 0 && index < count && briefs[static_cast<size_t>(index)].progress_sec == -1 ? "Mark as new" : key.label;
    }

    void onTick() override {
        Podcast_update();
        if (!Podcast_checkRefreshCompleted()) return;
        Podcast_invalidateEpisodeCache();
        const PodcastFeed* feed = Podcast_getSubscription(feedIndex);
        if (feed && feed->new_episode_count > 0) {
            const std::string text = std::to_string(feed->new_episode_count) + (feed->new_episode_count > 1 ? " new episodes found!" : " new episode found!");
            Toast_show(text.c_str(), TOAST_DURATION);
        } else {
            Toast_show("Already up to date", TOAST_DURATION);
        }
        Podcast_saveSubscriptions();
        localVersion++;
    }

    void onReload() override { read(); }

    bool onActivate(int index, PageNavigator& navigator) override {
        PodcastFeed* feed = Podcast_getSubscription(feedIndex);
        if (!feed || index < 0 || index >= count) return false;
        const PodcastEpisodeBrief& brief = briefs[static_cast<size_t>(index)];
        int progress                     = 0;
        const int status                 = getDownloadStatus(index, progress);
        if (status == PODCAST_DOWNLOAD_DOWNLOADING || status == PODCAST_DOWNLOAD_PENDING) {
            Toast_show(Podcast_cancelEpisodeDownload(feed->feed_url, brief.guid) == 0 ? "Download cancelled" : "Cancel failed", TOAST_DURATION);
            localVersion++;
        } else if (Podcast_episodeFileExists(feed, index)) {
            if (onPick) {
                // The player that opened the list takes the episode
                if (!isPlaying(index)) onPick(index);
                return true;
            }
            // An episode that plays already gets its player back
            navigator.open(isPlaying(index) ? PodcastPlayingPage{} : PodcastPlayingPage{feedIndex, index});
        } else {
            const std::string guid = brief.guid;
            navigator.openForResult(WifiDialogPage{}, [this, guid](const bool& connected) {
                if (!connected) {
                    Toast_show("No network connection", TOAST_DURATION);
                    return;
                }
                PodcastFeed* now = Podcast_getSubscription(feedIndex);
                const int at     = findGuid(guid);
                if (!now || at < 0) return;
                Toast_show(Podcast_queueDownload(now, at) == 0 ? "Downloading..." : "Download failed", TOAST_DURATION);
                localVersion++;
            });
        }
        return false;
    }

    void onKey(input::Button button, int index, PageNavigator& navigator) override {
        PodcastFeed* feed = Podcast_getSubscription(feedIndex);
        if (!feed) return;
        if (button == input::Button::Y) {
            if (Podcast_isRefreshing()) {
                Toast_show("Already refreshing...", TOAST_DURATION);
                return;
            }
            navigator.openForResult(WifiDialogPage{}, [this](const bool& connected) {
                if (!connected) {
                    Toast_show("No network connection", TOAST_DURATION);
                } else if (Podcast_startRefreshFeed(feedIndex) == 0 || Podcast_isRefreshing()) {
                    Toast_show("Checking for new episodes...", TOAST_DURATION);
                }
            });
            return;
        }
        if (button != input::Button::X || index < 0 || index >= count) return;
        PodcastEpisode* ep = Podcast_getEpisode(feedIndex, index);
        if (!ep) return;
        // Toggle the listened state
        if (ep->progress_sec == -1) {
            ep->progress_sec = 0;
            Podcast_saveProgress(feed->feed_url, ep->guid, 0);
            Toast_show("Marked as unplayed", TOAST_DURATION);
        } else {
            ep->progress_sec = -1;
            Podcast_markAsPlayed(feed->feed_url, ep->guid);
            Podcast_removeContinueListening(feed->feed_url, ep->guid);
            Toast_show("Marked as played", TOAST_DURATION);
        }
        Podcast_flushProgress();
    }

private:
    void read() {
        count = Podcast_getEpisodeBriefs(feedIndex, briefs.data(), kMaxEpisodes);
    }

    int findGuid(const std::string& guid) const {
        for (int index = 0; index < count; index++) {
            if (guid == briefs[static_cast<size_t>(index)].guid) return index;
        }
        return -1;
    }

    /** Returns the status of the download of the episode at `index`, or -1 when it is not in the queue. */
    int getDownloadStatus(int index, int& progress) const {
        progress = 0;
        const PodcastFeed* feed = Podcast_getSubscription(feedIndex);
        if (!feed || index < 0 || index >= count) return -1;
        return Podcast_getEpisodeDownloadStatus(feed->feed_url, briefs[static_cast<size_t>(index)].guid, &progress);
    }

    int feedIndex;
    std::function<void(int)> onPick;
    std::vector<PodcastEpisodeBrief> briefs;
    int count = 0;
    long localVersion = 0;
};

}  // namespace

SubListPage makePodcastEpisodesPage(int feedIndex, std::function<void(int)> onPick) {
    const PodcastFeed* feed = Podcast_getSubscription(feedIndex);
    return SubListPage(feed ? feed->title : "Episodes", "EPISODES", "PLAY", THEME_SOURCE_PODCAST,
                       std::make_shared<PodcastEpisodesSource>(feedIndex, std::move(onPick)), HELP_PODCAST_EPISODES);
}

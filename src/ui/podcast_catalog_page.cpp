#include "ui/podcast_catalog_page.hpp"

#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>

#include "podcast.h"
#include "toast.h"
#include "ui/confirm_dialog_page.hpp"
#include "ui/page/list_screen_controller.hpp"
#include "ui/podcast_art.hpp"
#include "ui/view/check_icon.hpp"
#include "ui/view/detail_panel_view.hpp"
#include "ui/view/empty_state_view.hpp"
#include "ui/view/list_adapter.hpp"
#include "ui/view/list_row_view.hpp"
#include "ui/view/list_screen_view.hpp"
#include "ui/view/text_view.hpp"
#include "ui/wifi_dialog_page.hpp"
#include "ui_fonts.h"
#include "ui_theme.h"

using namespace input;

namespace {

constexpr const char* kLeftHints[]            = {"START", "CONTROLS", nullptr};
constexpr const char* kSearchSubscribe[]      = {"B", "BACK", "A", "SUBSCRIBE", nullptr};
constexpr const char* kSearchUnsubscribe[]    = {"B", "BACK", "A", "UNSUBSCRIBE", nullptr};
constexpr const char* kTopSubscribe[]         = {"B", "BACK", "X", "REFRESH", "A", "SUBSCRIBE", nullptr};
constexpr const char* kTopUnsubscribe[]       = {"B", "BACK", "X", "REFRESH", "A", "UNSUBSCRIBE", nullptr};
constexpr const char* kBackOnly[]             = {"B", "BACK", nullptr};
constexpr const char* kTopBackOnly[]          = {"B", "BACK", "X", "REFRESH", nullptr};

/** The frames that the page waits after it says that it subscribes, thus the words reach the screen */
constexpr int kSubscribeWaitFrames = 2;

/** The fields of a show of the search or of the charts that the page reads */
struct CatalogEntry {
    std::string itunesId;
    std::string title;
    std::string author;
    std::string genre;
    std::string feedUrl;
    std::string artworkUrl;
};

class CatalogData {
public:
    explicit CatalogData(PodcastCatalogKind kind) : kind(kind) {}

    PodcastCatalogKind getKind() const { return kind; }

    /** True while the search or the charts load */
    bool isLoading() const {
        return kind == PodcastCatalogKind::SearchResults ? Podcast_getSearchStatus()->searching : Podcast_getChartsStatus()->loading;
    }

    std::string getError() const {
        return kind == PodcastCatalogKind::SearchResults ? Podcast_getSearchStatus()->error_message : Podcast_getChartsStatus()->error_message;
    }

    int getCount() const {
        if (isLoading()) return 0;
        int count = 0;
        if (kind == PodcastCatalogKind::SearchResults) {
            Podcast_getSearchResults(&count);
        } else {
            Podcast_getTopShows(&count);
        }
        return count;
    }

    CatalogEntry get(int index) const {
        CatalogEntry entry;
        if (index < 0 || index >= getCount()) return entry;
        int count = 0;
        if (kind == PodcastCatalogKind::SearchResults) {
            const PodcastSearchResult& result = Podcast_getSearchResults(&count)[index];
            entry = {result.itunes_id, result.title, result.author, result.genre, result.feed_url, result.artwork_url};
        } else {
            const PodcastChartItem& item = Podcast_getTopShows(&count)[index];
            entry                        = {item.itunes_id, item.title, item.author, item.genre, item.feed_url, item.artwork_url};
        }
        return entry;
    }

    bool isSubscribed(const CatalogEntry& entry) const {
        if (kind == PodcastCatalogKind::SearchResults) return !entry.feedUrl.empty() && Podcast_isSubscribed(entry.feedUrl.c_str());
        return Podcast_isSubscribedByItunesId(entry.itunesId.c_str());
    }

    /** Returns the index of the subscription of the entry, or -1 */
    int findSubscription(const CatalogEntry& entry) const {
        int count          = 0;
        PodcastFeed* feeds = Podcast_getSubscriptions(&count);
        for (int index = 0; index < count; index++) {
            const bool same = kind == PodcastCatalogKind::SearchResults
                                  ? entry.feedUrl == feeds[index].feed_url
                                  : feeds[index].itunes_id[0] && entry.itunesId == feeds[index].itunes_id;
            if (same) return index;
        }
        return -1;
    }

private:
    PodcastCatalogKind kind;
};

class CatalogAdapter final : public ListAdapter {
public:
    explicit CatalogAdapter(const CatalogData& data) : data(data) {}

    int getCount() const override { return data.getCount(); }

    long getItemId(int index) const override {
        const CatalogEntry entry = data.get(index);
        std::string key          = std::to_string(index) + '\n' + entry.itunesId + '\n' + entry.title + (data.isSubscribed(entry) ? "\n1" : "\n0") +
                          (Theme_isSourceColoursEnabled() ? "\n1" : "\n0");
        const auto [it, added] = ids.try_emplace(std::move(key), static_cast<long>(ids.size()));
        return it->second;
    }

    std::shared_ptr<ListItemView> getView(int index) override {
        const CatalogEntry entry = data.get(index);
        auto row                 = std::make_shared<ListRowView>(Fonts_getMedium());
        row->setText(entry.title);
        if (data.isSubscribed(entry)) row->setIcon(getCheckIcon(false), getCheckIcon(true));
        // The detail panel of the new look carries the author
        if (!entry.author.empty() && !Theme_isSourceColoursEnabled()) {
            auto author = std::make_shared<TextView>(Fonts_getTiny(), false);
            author->setText(entry.author);
            author->setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
            row->setRightText(std::move(author));
        }
        return row;
    }

private:
    const CatalogData& data;
    mutable std::unordered_map<std::string, long> ids;
};

class PodcastCatalogPageImpl final : public UiPageImpl<PodcastCatalogPage>, private ListScreenListener {
public:
    explicit PodcastCatalogPageImpl(PodcastCatalogKind kind)
        : UiPageImpl(PageKind::FullScreen), data(kind), rows(data) {
        const bool search = kind == PodcastCatalogKind::SearchResults;
        screenView.getTitleView().setText(search ? "Search Results" : "Top Shows");
        screenView.getTitleView().setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        screenView.setEmptyState(&emptyState);
    }

    std::span<const ButtonRequest> getButtonRequests() const override {
        return ListScreenController::getButtonRequests();
    }

    HelpId getHelpId() const override {
        return data.getKind() == PodcastCatalogKind::SearchResults ? HELP_PODCAST_SEARCH_RESULTS : HELP_PODCAST_TOP_SHOWS;
    }
    ThemeSource getSource() const override { return THEME_SOURCE_PODCAST; }

    void onEnter() override {
        screenView.setAdapter(rows);
        list.start();
        shownSignature = getSignature();
        refreshEmptyState();
        updatePanel(list.getSelectedIndex());
        updateHints();
    }

    void onExit() override {
        if (data.getKind() == PodcastCatalogKind::SearchResults) Podcast_cancelSearch();
        Toast_dismiss(subscribeToast);
    }

    void layout(UiLayoutContext& context) override {
        screenView.setSidePanel(Theme_isSourceColoursEnabled() ? &panel : nullptr);
        context.layoutChild(screenView, context.getMaxRect());
    }

    void onTick() override {
        Podcast_update();
        if (pendingEntry && ++framesWaited >= kSubscribeWaitFrames) subscribePending();
        const std::string signature = getSignature();
        if (signature != shownSignature) reload();
        fetchCoverOfSelection();
    }

    bool onButtonEvent(const ButtonEvent& event) override {
        if (pendingEntry) return true;
        if (list.onButtonEvent(event)) return true;
        if (event.isSingleButton(Button::A)) {
            activate();
            return true;
        }
        if (event.isSingleButton(Button::B)) {
            close();
            return true;
        }
        if (event.isSingleButton(Button::X) && data.getKind() == PodcastCatalogKind::TopShows) {
            refreshCharts();
            return true;
        }
        return false;
    }

private:
    void onSelectionChanged(int index) override {
        idleFrames = 0;
        updatePanel(index);
        updateHints();
    }

    /** What the page shows depends on this: the load, the count and the subscriptions */
    std::string getSignature() const {
        return std::to_string(data.isLoading()) + "/" + std::to_string(data.getCount()) + "/" + std::to_string(Podcast_getSubscriptionCount()) + "/" +
               data.getError();
    }

    void refreshEmptyState() {
        const bool search = data.getKind() == PodcastCatalogKind::SearchResults;
        if (data.isLoading()) {
            emptyState.setMessage(search ? "Searching..." : "Loading...");
        } else {
            const std::string error = data.getError();
            emptyState.setMessage(!error.empty() ? error : (search ? "No results found" : "No shows available"));
        }
    }

    void reload() {
        const int position = list.getSelectedIndex();
        shownSignature     = getSignature();
        refreshEmptyState();
        list.notifyDataChanged();
        if (position >= 0 && position < rows.getCount()) list.setSelectedIndex(position);
        updatePanel(list.getSelectedIndex());
        updateHints();
    }

    void updateHints() {
        const bool top        = data.getKind() == PodcastCatalogKind::TopShows;
        const CatalogEntry at = data.get(list.getSelectedIndex());
        const bool any        = rows.getCount() > 0 && list.getSelectedIndex() >= 0;
        const char* const* right;
        if (!any) {
            right = top ? kTopBackOnly : kBackOnly;
        } else if (data.isSubscribed(at)) {
            right = top ? kTopUnsubscribe : kSearchUnsubscribe;
        } else {
            right = top ? kTopSubscribe : kSearchSubscribe;
        }
        screenView.getButtonHintsView().setHints(kLeftHints, right);
    }

    void updatePanel(int index) {
        const CatalogEntry entry = data.get(index);
        if (entry.title.empty()) return;
        panel.getArt().setFallback(entry.title, THEME_SOURCE_PODCAST);
        const std::string path = getPodcastCoverPath(entry.itunesId.c_str());
        // A show with no cover in the cache keeps the tile: the cover comes when the cursor rests
        if (!entry.itunesId.empty()) {
            if (std::FILE* file = std::fopen(path.c_str(), "rb")) {
                std::fclose(file);
                panel.getArt().loadFile(path.c_str());
            } else {
                panel.getArt().setBitmap(nullptr);
            }
        } else {
            panel.getArt().setBitmap(nullptr);
        }
        panel.getInfo().setLine(entry.author);
        panel.getInfo().setPosition(entry.genre);
        panel.getInfo().setTimes(data.isSubscribed(entry) ? "Subscribed" : "");
    }

    /** The cursor that rests on a show fetches its cover, one blocking call, as the old list did. */
    void fetchCoverOfSelection() {
        if (!Theme_isSourceColoursEnabled() || pendingEntry) return;
        if (++idleFrames != kCoverIdleFrames) return;
        const CatalogEntry entry = data.get(list.getSelectedIndex());
        if (entry.itunesId.empty() || entry.artworkUrl.empty()) return;
        if (fetchPodcastCover(entry.itunesId.c_str(), entry.artworkUrl.c_str())) updatePanel(list.getSelectedIndex());
    }

    void activate() {
        const int index          = list.getSelectedIndex();
        const CatalogEntry entry = data.get(index);
        if (entry.title.empty()) return;
        if (data.isSubscribed(entry)) {
            const int subscription = data.findSubscription(entry);
            if (subscription < 0) return;
            getNavigator().openForResult(ConfirmDialogPage("Unsubscribe?", entry.title), [this, subscription](const bool& yes) {
                if (!yes) return;
                Podcast_unsubscribe(subscription);
                Toast_show("Unsubscribed", TOAST_DURATION);
                reload();
            });
            return;
        }
        // The network answers after a while: the words come first, the call in the next frames
        pendingEntry  = std::make_unique<CatalogEntry>(entry);
        framesWaited  = 0;
        subscribeToast = Toast_show("Subscribing...", TOAST_DURATION_FOREVER);
    }

    void subscribePending() {
        const CatalogEntry entry = *pendingEntry;
        pendingEntry.reset();
        const int result = !entry.feedUrl.empty() ? Podcast_subscribe(entry.feedUrl.c_str()) : Podcast_subscribeFromItunes(entry.itunesId.c_str());
        Toast_dismiss(subscribeToast);
        if (result == 0) {
            Toast_show("Subscribed!", TOAST_DURATION);
        } else {
            const char* error = Podcast_getError();
            Toast_show(error && error[0] ? error : "Subscribe failed", TOAST_DURATION);
        }
        reload();
    }

    void refreshCharts() {
        getNavigator().openForResult(WifiDialogPage{}, [this](const bool& connected) {
            if (!connected) {
                Toast_show("Internet connection required", TOAST_DURATION);
                return;
            }
            Podcast_clearChartsCache();
            Podcast_loadCharts(nullptr);
            Toast_show("Refreshing...", TOAST_DURATION);
            reload();
        });
    }

    /** The frames that the cursor rests before the cover of its show is fetched */
    static constexpr int kCoverIdleFrames = 20;

    CatalogData data;
    CatalogAdapter rows;
    std::string shownSignature;
    std::unique_ptr<CatalogEntry> pendingEntry;
    int framesWaited = 0;
    int idleFrames   = 0;
    ToastToken subscribeToast = TOAST_TOKEN_NONE;
    ListScreenView screenView{Fonts_getLarge()};
    EmptyStateView emptyState;
    DetailPanelView panel;
    ListScreenController list{screenView, getScheduler(), *this};
};

}  // namespace

std::unique_ptr<UiPageImplBase> PodcastCatalogPage::createPageImpl() const {
    return std::make_unique<PodcastCatalogPageImpl>(kind);
}

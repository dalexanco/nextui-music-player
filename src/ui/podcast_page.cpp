#include "ui/podcast_page.hpp"

#include <algorithm>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>

#include "background.h"
#include "module_common.h"
#include "module_podcast.h"
#include "podcast.h"
#include "podcast_import.h"
#include "toast.h"
#include "ui/choice_options_page.hpp"
#include "ui/confirm_dialog_page.hpp"
#include "ui/page/list_screen_controller.hpp"
#include "ui/podcast_episodes_page.hpp"
#include "ui/podcast_manage_page.hpp"
#include "ui/podcast_playing_page.hpp"
#include "ui/podcast_queue_page.hpp"
#include "ui/podcast_show_panel.hpp"
#include "ui/view/detail_panel_view.hpp"
#include "ui/view/empty_state_view.hpp"
#include "ui/view/list_adapter.hpp"
#include "ui/view/list_row_view.hpp"
#include "ui/view/list_screen_view.hpp"
#include "ui/view/star_icon.hpp"
#include "ui/view/text_view.hpp"
#include "ui_fonts.h"
#include "ui_theme.h"

using namespace input;

namespace {

constexpr const char* kLeftHints[]      = {"Y", "OPTIONS", nullptr};
constexpr const char* kRightHints[]     = {"B", "BACK", "A", "OPEN", nullptr};
constexpr const char* kEmptyRightHints[] = {"B", "BACK", nullptr};

/** What a row of the list is */
enum class RowKind { Continue, Show, Downloads, None };

/**
 * The rows of the list: the episodes to continue (the two latest), then the shows, then the
 * downloads while there are some. A show with new episodes has a star.
 */
class ShowsAdapter final : public ListAdapter {
public:
    int getContinueCount() const {
        return std::min(Podcast_getContinueListeningCount(), PODCAST_CONTINUE_LISTENING_DISPLAY);
    }
    int getShowCount() const { return Podcast_getSubscriptionCount(); }
    bool hasDownloads() const {
        int queue = 0;
        Podcast_getDownloadQueue(&queue);
        return queue > 0;
    }

    int getCount() const override { return getContinueCount() + getShowCount() + (hasDownloads() ? 1 : 0); }

    /** Returns what the row at `index` is, and its index among the rows of its kind. */
    RowKind getKind(int index, int& indexInKind) const {
        const int continues = getContinueCount();
        const int shows     = getShowCount();
        if (index < 0) return RowKind::None;
        if (index < continues) {
            indexInKind = index;
            return RowKind::Continue;
        }
        if (index < continues + shows) {
            indexInKind = index - continues;
            return RowKind::Show;
        }
        if (index == continues + shows && hasDownloads()) return RowKind::Downloads;
        return RowKind::None;
    }

    long getItemId(int index) const override {
        int inKind = 0;
        std::string key;
        switch (getKind(index, inKind)) {
            case RowKind::Continue:  key = std::string("c\n") + Podcast_getContinueListening(inKind)->episode_guid; break;
            case RowKind::Show: {
                const PodcastFeed* feed = Podcast_getSubscription(inKind);
                key = std::string("s\n") + feed->feed_url + (feed->new_episode_count > 0 ? "\n1" : "\n0");
                break;
            }
            case RowKind::Downloads: key = "d"; break;
            case RowKind::None:      key = "?"; break;
        }
        key += Theme_isSourceColoursEnabled() ? "\n1" : "\n0";
        const auto [it, added] = ids.try_emplace(std::move(key), static_cast<long>(ids.size()));
        return it->second;
    }

    std::shared_ptr<ListItemView> getView(int index) override {
        auto row = std::make_shared<ListRowView>(Fonts_getMedium());
        int inKind = 0;
        std::string value;
        switch (getKind(index, inKind)) {
            case RowKind::Continue: {
                const ContinueListeningEntry* entry = Podcast_getContinueListening(inKind);
                row->setText(entry->episode_title);
                value = entry->feed_title;
                break;
            }
            case RowKind::Show: {
                const PodcastFeed* feed = Podcast_getSubscription(inKind);
                row->setText(feed->title);
                if (feed->new_episode_count > 0) row->setIcon(getStarIcon(false), getStarIcon(true));
                value = std::to_string(feed->episode_count) + (feed->episode_count == 1 ? " episode" : " episodes");
                break;
            }
            case RowKind::Downloads: {
                int queue = 0;
                Podcast_getDownloadQueue(&queue);
                row->setText("Downloads (" + std::to_string(queue) + ")");
                break;
            }
            case RowKind::None: break;
        }
        // The detail panel of the new look carries the values
        if (!value.empty() && !Theme_isSourceColoursEnabled()) {
            auto text = std::make_shared<TextView>(Fonts_getTiny(), false);
            text->setText(value);
            text->setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
            row->setRightText(std::move(text));
        }
        return row;
    }

private:
    mutable std::unordered_map<std::string, long> ids;
};

class PodcastPageImpl final : public UiPageImpl<PodcastPage>, private ListScreenListener {
public:
    PodcastPageImpl() : UiPageImpl(PageKind::FullScreen) {
        screenView.getTitleView().setText("Podcasts");
        screenView.getTitleView().setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        emptyState.setMessage("No podcasts subscribed");
        emptyState.setLine("Press Y for options");
        screenView.setEmptyState(&emptyState);
        screenView.getButtonHintsView().setHints(kLeftHints, kRightHints);
    }

    std::span<const ButtonRequest> getButtonRequests() const override {
        return ListScreenController::getButtonRequests();
    }

    HelpId getHelpId() const override { return HELP_PODCAST_MENU; }

    /** The focused row of a continue or a show takes the accent of the podcasts, as the page does. */
    ThemeSource getSource() const override { return THEME_SOURCE_PODCAST; }

    void onEnter() override {
        Podcast_init();
        PodcastModule_autoRefresh();
        if (PodcastImport_isRunning()) Toast_show("Importing podcasts...", TOAST_DURATION);
        screenView.setAdapter(rows);
        list.start();
        shownSignature = getSignature();
        updatePanel(list.getSelectedIndex());
        updateHints();
        // A podcast that plays in the background opens its player at once
        if (Background_getActive() == BG_PODCAST && Podcast_isActive()) getNavigator().open(PodcastPlayingPage{});
    }

    void onExit() override {
        // An import that runs keeps the module alive, as a download does: it still adds shows
        if (Podcast_isActive() || Podcast_isDownloading() || PodcastImport_isRunning()) {
            Podcast_saveSubscriptions();
            Podcast_flushProgress();
            if (Podcast_isActive()) Background_setActive(BG_PODCAST);
        } else {
            Podcast_cleanup();
        }
    }

    void resume() override { reload(); }

    void onTick() override {
        Podcast_update();
        if (PodcastImport_takeFinished()) sayImportResult();
        if (Podcast_checkRefreshCompleted()) {
            Podcast_saveSubscriptions();
            reload();
        } else if (getSignature() != shownSignature) {
            // The queue of the downloads comes and goes, the shows change under a refresh
            reload();
        }
    }

    void layout(UiLayoutContext& context) override {
        // An empty list has nothing to show in a panel
        screenView.setSidePanel(Theme_isSourceColoursEnabled() && rows.getCount() > 0 ? &panel : nullptr);
        context.layoutChild(screenView, context.getMaxRect());
    }

    bool onButtonEvent(const ButtonEvent& event) override {
        if (list.onButtonEvent(event)) return true;
        if (event.isSingleButton(Button::A)) {
            open();
            return true;
        }
        if (event.isSingleButton(Button::B)) {
            close();
            return true;
        }
        if (event.isSingleButton(Button::Y)) {
            openOptions();
            return true;
        }
        return false;
    }

private:
    void onSelectionChanged(int index) override { updatePanel(index); }

    /** What the rows depend on, to see that the data changed under the cursor */
    std::string getSignature() const {
        int queue = 0;
        Podcast_getDownloadQueue(&queue);
        int newEpisodes = 0;
        for (int index = 0; index < Podcast_getSubscriptionCount(); index++) newEpisodes += Podcast_getSubscription(index)->new_episode_count;
        return std::to_string(Podcast_getContinueListeningCount()) + "/" + std::to_string(Podcast_getSubscriptionCount()) + "/" +
               std::to_string(queue > 0) + "/" + std::to_string(newEpisodes);
    }


    /** Says what the import of the files on the card did, then lists the shows again. */
    void sayImportResult() {
        const PodcastImportStatus status = PodcastImport_getStatus();
        std::string text = std::to_string(status.added) + (status.added == 1 ? " podcast added" : " podcasts added");
        if (status.duplicates > 0) text += ", " + std::to_string(status.duplicates) + " already subscribed";
        if (status.failed + status.invalid_files > 0) text += ", " + std::to_string(status.failed + status.invalid_files) + " failed, see Import from SD card";
        Toast_show(text.c_str(), TOAST_DURATION);
        Podcast_saveSubscriptions();
        reload();
    }

    /** Plays the episode of an entry of Continue Listening, which is found again by its guid. */
    void continueListening(int index) {
        const ContinueListeningEntry* entry = Podcast_getContinueListening(index);
        if (!entry) return;
        const int feed = Podcast_findFeedIndex(entry->feed_url);
        if (feed < 0) {
            Toast_show("Podcast not found", TOAST_DURATION);
            return;
        }
        const PodcastFeed* feedPtr = Podcast_getSubscription(feed);
        for (int episode = 0; feedPtr && episode < feedPtr->episode_count; episode++) {
            const PodcastEpisode* ep = Podcast_getEpisode(feed, episode);
            if (!ep || std::strcmp(ep->guid, entry->episode_guid) != 0) continue;
            if (Podcast_episodeFileExists(const_cast<PodcastFeed*>(feedPtr), episode)) {
                getNavigator().open(PodcastPlayingPage{feed, episode});
                return;
            }
            break;
        }
        Toast_show("Episode not available", TOAST_DURATION);
    }

    void open() {
        int inKind = 0;
        switch (rows.getKind(list.getSelectedIndex(), inKind)) {
            case RowKind::Continue:  continueListening(inKind); break;
            case RowKind::Show:      getNavigator().open(makePodcastEpisodesPage(inKind)); break;
            case RowKind::Downloads: getNavigator().open(PodcastQueuePage{}); break;
            case RowKind::None:      break;
        }
    }

    /** The actions of the list besides A: manage the podcasts, and unsubscribe the show under the cursor. */
    void openOptions() {
        int inKind = 0;
        const bool onShow = rows.getKind(list.getSelectedIndex(), inKind) == RowKind::Show;
        const PodcastFeed* feed = onShow ? Podcast_getSubscription(inKind) : nullptr;
        std::vector<std::string> labels = {"Search and top shows"};
        if (feed) labels.push_back("Unsubscribe");
        getNavigator().openForResult(
            ChoiceOptionsPage(THEME_SOURCE_PODCAST, "PODCASTS", feed ? "Show" : "Podcasts", feed ? feed->title : "Manage", labels),
            [this](const ChoiceResult& result) {
                if (result.index == 0) getNavigator().open(PodcastManagePage{});
                if (result.index == 1) unsubscribe();
            });
    }

    /** A with nothing under the cursor does nothing, thus the bar keeps no A for an empty list. */
    void updateHints() {
        const bool empty = rows.getCount() == 0;
        if (empty == hintsForEmpty && hintsSet) return;
        hintsSet       = true;
        hintsForEmpty  = empty;
        screenView.getButtonHintsView().setHints(kLeftHints, empty ? kEmptyRightHints : kRightHints);
    }

    void unsubscribe() {
        int inKind = 0;
        if (rows.getKind(list.getSelectedIndex(), inKind) != RowKind::Show) return;
        const PodcastFeed* feed = Podcast_getSubscription(inKind);
        if (!feed) return;
        // The list does not change while the dialog shows, thus the index names the show of the dialog
        getNavigator().openForResult(ConfirmDialogPage("Unsubscribe?", feed->title), [this, inKind](const bool& yes) {
            if (!yes) return;
            Podcast_unsubscribe(inKind);
            Toast_show("Unsubscribed", TOAST_DURATION);
            reload();
        });
    }

    void updatePanel(int index) {
        int inKind = 0;
        ShowPanelText text;
        switch (rows.getKind(index, inKind)) {
            case RowKind::Show: text = makeShowPanelText(inKind); break;
            case RowKind::Continue: {
                const ContinueListeningEntry* entry = Podcast_getContinueListening(inKind);
                const int feed                      = Podcast_findFeedIndex(entry->feed_url);
                if (feed >= 0) text = makeShowPanelText(feed);
                text.name   = entry->feed_title;
                text.line   = entry->episode_title;
                text.status = "Continue listening";
                break;
            }
            case RowKind::Downloads: {
                text.name = "Downloads";
                int queue = 0;
                Podcast_getDownloadQueue(&queue);
                text.line = std::to_string(queue) + (queue == 1 ? " episode" : " episodes");
                break;
            }
            case RowKind::None: return;
        }
        fillShowPanel(panel, text);
    }

    /** Reads the rows again. The cursor keeps its position, or goes to the last row. */
    void reload() {
        const int position = list.getSelectedIndex();
        shownSignature     = getSignature();
        list.notifyDataChanged();
        if (position >= 0) list.setSelectedIndex(position);
        updatePanel(list.getSelectedIndex());
        updateHints();
    }

    bool hintsSet      = false;
    bool hintsForEmpty = false;
    ShowsAdapter rows;
    std::string shownSignature;
    ListScreenView screenView{Fonts_getLarge()};
    EmptyStateView emptyState;
    DetailPanelView panel;
    ListScreenController list{screenView, getScheduler(), *this};
};

}  // namespace

std::unique_ptr<UiPageImplBase> PodcastPage::createPageImpl() const {
    return std::make_unique<PodcastPageImpl>();
}

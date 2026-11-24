#include "ui/podcast_queue_page.hpp"

#include <memory>
#include <string>
#include <unordered_map>

#include "podcast.h"
#include "toast.h"
#include "ui/page/list_screen_controller.hpp"
#include "ui/view/empty_state_view.hpp"
#include "ui/view/list_adapter.hpp"
#include "ui/view/list_row_view.hpp"
#include "ui/view/list_screen_view.hpp"
#include "ui/view/text_view.hpp"
#include "ui_fonts.h"
#include "ui_theme.h"

using namespace input;

namespace {

constexpr const char* kLeftHints[]  = {"START", "CONTROLS", nullptr};
constexpr const char* kRightHints[] = {"B", "BACK", "X", "REMOVE", nullptr};
constexpr const char* kEmptyHints[] = {"B", "BACK", nullptr};

/** The state of a download as the row says it */
std::string getStateText(const PodcastDownloadItem& item) {
    switch (item.status) {
        case PODCAST_DOWNLOAD_DOWNLOADING: return std::to_string(item.progress_percent) + " %";
        case PODCAST_DOWNLOAD_PENDING:     return "Queued";
        case PODCAST_DOWNLOAD_COMPLETE:    return "Done";
        case PODCAST_DOWNLOAD_FAILED:      return "Failed";
    }
    return {};
}

class QueueAdapter final : public ListAdapter {
public:
    int getCount() const override {
        int count = 0;
        Podcast_getDownloadQueue(&count);
        return count;
    }

    long getItemId(int index) const override {
        const PodcastDownloadItem* queue = Podcast_getDownloadQueue(nullptr);
        const PodcastDownloadItem& item  = queue[index];
        std::string key                  = std::string(item.episode_guid) + '\n' + getStateText(item);
        const auto [it, added]           = ids.try_emplace(std::move(key), static_cast<long>(ids.size()));
        return it->second;
    }

    std::shared_ptr<ListItemView> getView(int index) override {
        const PodcastDownloadItem* queue = Podcast_getDownloadQueue(nullptr);
        const PodcastDownloadItem& item  = queue[index];
        auto row                         = std::make_shared<ListRowView>(Fonts_getMedium());
        row->setText(item.episode_title);
        auto state = std::make_shared<TextView>(Fonts_getTiny(), false);
        state->setText(getStateText(item));
        state->setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
        row->setRightText(std::move(state));
        return row;
    }

private:
    mutable std::unordered_map<std::string, long> ids;
};

class PodcastQueuePageImpl final : public UiPageImpl<PodcastQueuePage>, private ListScreenListener {
public:
    PodcastQueuePageImpl() : UiPageImpl(PageKind::FullScreen) {
        screenView.getTitleView().setText("Downloads");
        screenView.getTitleView().setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        emptyState.setMessage("No downloads");
        screenView.setEmptyState(&emptyState);
        screenView.getButtonHintsView().setHints(kLeftHints, kRightHints);
    }

    std::span<const ButtonRequest> getButtonRequests() const override {
        return ListScreenController::getButtonRequests();
    }

    HelpId getHelpId() const override { return HELP_PODCAST_DOWNLOADS; }
    ThemeSource getSource() const override { return THEME_SOURCE_PODCAST; }

    void onEnter() override {
        screenView.setAdapter(rows);
        list.start();
        shownSignature = getSignature();
        updateHints();
    }

    void onTick() override {
        Podcast_update();
        const std::string signature = getSignature();
        if (signature != shownSignature) reload();
    }

    void layout(UiLayoutContext& context) override { context.layoutChild(screenView, context.getMaxRect()); }

    bool onButtonEvent(const ButtonEvent& event) override {
        if (list.onButtonEvent(event)) return true;
        if (event.isSingleButton(Button::B)) {
            close();
            return true;
        }
        if (event.isSingleButton(Button::X)) {
            removeSelected();
            return true;
        }
        return false;
    }

private:
    /** What the rows depend on: the order of the downloads and their states */
    static std::string getSignature() {
        int count                        = 0;
        const PodcastDownloadItem* queue = Podcast_getDownloadQueue(&count);
        std::string signature;
        for (int index = 0; index < count; index++) signature += std::string(queue[index].episode_guid) + getStateText(queue[index]) + '\n';
        return signature;
    }

    static std::string getGuid(int index) {
        int count                        = 0;
        const PodcastDownloadItem* queue = Podcast_getDownloadQueue(&count);
        return index >= 0 && index < count ? queue[index].episode_guid : std::string();
    }

    void removeSelected() {
        int count                  = 0;
        PodcastDownloadItem* queue = Podcast_getDownloadQueue(&count);
        const int index            = list.getSelectedIndex();
        if (index < 0 || index >= count) return;
        const PodcastDownloadItem& item = queue[index];
        Toast_show(Podcast_cancelEpisodeDownload(item.feed_url, item.episode_guid) == 0 ? "Download removed" : "Remove failed", TOAST_DURATION);
        reload();
    }

    /** Reads the queue again. The cursor stays on its download, or on its row when that one went. */
    void reload() {
        const int position     = list.getSelectedIndex();
        const std::string guid = getGuid(position);
        shownSignature         = getSignature();
        list.notifyDataChanged();
        int target = position;
        for (int index = 0; index < rows.getCount() && !guid.empty(); index++) {
            if (getGuid(index) == guid) target = index;
        }
        if (target >= 0) list.setSelectedIndex(std::min(target, rows.getCount() - 1));
        updateHints();
    }

    void updateHints() {
        screenView.getButtonHintsView().setHints(kLeftHints, rows.getCount() > 0 ? kRightHints : kEmptyHints);
    }

    QueueAdapter rows;
    std::string shownSignature;
    ListScreenView screenView{Fonts_getLarge()};
    EmptyStateView emptyState;
    ListScreenController list{screenView, getScheduler(), *this};
};

}  // namespace

std::unique_ptr<UiPageImplBase> PodcastQueuePage::createPageImpl() const {
    return std::make_unique<PodcastQueuePageImpl>();
}

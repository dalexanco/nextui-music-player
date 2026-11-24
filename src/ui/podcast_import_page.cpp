#include "ui/podcast_import_page.hpp"

#include <memory>
#include <string>
#include <unordered_map>

#include "podcast_import.h"
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
constexpr const char* kRightHints[] = {"B", "BACK", nullptr};

const char* statusLabel(PodcastImportItemStatus status) {
    switch (status) {
        case PODCAST_IMPORT_ITEM_ADDED:     return "Added";
        case PODCAST_IMPORT_ITEM_DUPLICATE: return "Already subscribed";
        case PODCAST_IMPORT_ITEM_FAILED:    return "Failed";
        case PODCAST_IMPORT_ITEM_PENDING:   break;
    }
    return "";
}

class ImportAdapter final : public ListAdapter {
public:
    int getCount() const override { return PodcastImport_getItemCount(); }

    long getItemId(int index) const override {
        PodcastImportItem item;
        if (!PodcastImport_getItem(index, &item)) return index;
        std::string key = std::to_string(index) + '\n' + item.url + '\n' + item.name + '\n' + std::to_string(item.status) +
                          (Theme_isSourceColoursEnabled() ? "\n1" : "\n0");
        const auto [it, added] = ids.try_emplace(std::move(key), static_cast<long>(ids.size()));
        return it->second;
    }

    std::shared_ptr<ListItemView> getView(int index) override {
        auto row = std::make_shared<ListRowView>(Fonts_getMedium());
        PodcastImportItem item;
        if (!PodcastImport_getItem(index, &item)) return row;
        row->setText(item.name);
        // A failure says why, in the place of the word that the others have
        std::string status = item.status == PODCAST_IMPORT_ITEM_FAILED && item.reason[0] ? item.reason : statusLabel(item.status);
        auto text          = std::make_shared<TextView>(Fonts_getTiny(), false);
        text->setText(status);
        text->setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
        row->setRightText(std::move(text));
        return row;
    }

private:
    mutable std::unordered_map<std::string, long> ids;
};

class PodcastImportPageImpl final : public UiPageImpl<PodcastImportPage>, private ListScreenListener {
public:
    PodcastImportPageImpl() : UiPageImpl(PageKind::FullScreen) {
        screenView.getTitleView().setText("Import Podcasts");
        screenView.getTitleView().setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        screenView.setEmptyState(&emptyState);
        screenView.getButtonHintsView().setHints(kLeftHints, kRightHints);
    }

    std::span<const ButtonRequest> getButtonRequests() const override {
        return ListScreenController::getButtonRequests();
    }

    HelpId getHelpId() const override { return HELP_PODCAST_MANAGE; }
    ThemeSource getSource() const override { return THEME_SOURCE_PODCAST; }

    void onEnter() override {
        // An import that the Podcasts page started carries on, with its results
        if (!PodcastImport_isRunning() && PodcastImport_countFiles(true) > 0) PodcastImport_start(true);
        screenView.setAdapter(rows);
        list.start();
        shownSignature = getSignature();
        refresh();
    }

    void onTick() override {
        PodcastImport_takeFinished();
        if (getSignature() != shownSignature) reload();
    }

    void layout(UiLayoutContext& context) override { context.layoutChild(screenView, context.getMaxRect()); }

    bool onButtonEvent(const ButtonEvent& event) override {
        if (list.onButtonEvent(event)) return true;
        if (event.isSingleButton(Button::B)) {
            close();
            return true;
        }
        return false;
    }

private:
    std::string getSignature() const {
        const PodcastImportStatus status = PodcastImport_getStatus();
        return std::to_string(status.state) + "/" + std::to_string(status.done) + "/" + std::to_string(status.total) + "/" +
               std::to_string(PodcastImport_getItemCount()) + "/" + status.current;
    }

    /** The title says where the import stands, and the empty state says why the list has nothing */
    void refresh() {
        const PodcastImportStatus status = PodcastImport_getStatus();
        std::string title                = "Import Podcasts";
        if (status.state == PODCAST_IMPORT_STATE_RUNNING) {
            title += " (" + std::to_string(status.done) + "/" + std::to_string(status.total) + ")";
            emptyState.setMessage(status.total > 0 ? "Importing..." : "Reading the files...");
            emptyState.setLine(status.current);
        } else if (status.state == PODCAST_IMPORT_STATE_DONE) {
            // Short, a longer title scrolls: the rows tell the rest
            const int failed = status.failed + status.invalid_files;
            title            = std::to_string(status.added) + " added";
            if (failed > 0) title += ", " + std::to_string(failed) + " failed";
            else if (status.duplicates > 0 && status.added == 0) title = std::to_string(status.duplicates) + " already subscribed";
            emptyState.setMessage("Nothing to import");
            emptyState.setLine("");
        } else {
            emptyState.setMessage("No file to import");
            emptyState.setLine("Drop .txt, .opml or .url files in Podcasts/Subscriptions");
        }
        screenView.getTitleView().setText(title);
    }

    void reload() {
        const int position = list.getSelectedIndex();
        shownSignature     = getSignature();
        refresh();
        list.notifyDataChanged();
        if (position >= 0 && position < rows.getCount()) list.setSelectedIndex(position);
    }

    ImportAdapter rows;
    std::string shownSignature;
    ListScreenView screenView{Fonts_getLarge()};
    EmptyStateView emptyState;
    ListScreenController list{screenView, getScheduler(), *this};
};

}  // namespace

std::unique_ptr<UiPageImplBase> PodcastImportPage::createPageImpl() const {
    return std::make_unique<PodcastImportPageImpl>();
}

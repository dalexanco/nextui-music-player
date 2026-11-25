#include "ui/podcast_manage_page.hpp"

#include <memory>
#include <string>

#include "podcast.h"
#include "podcast_import.h"
#include "toast.h"
#include "ui/page/list_screen_controller.hpp"
#include "ui/podcast_catalog_page.hpp"
#include "ui/podcast_import_page.hpp"
#include "ui/text_input_page.hpp"
#include "ui/view/list_adapter.hpp"
#include "ui/view/list_row_view.hpp"
#include "ui/view/list_screen_view.hpp"
#include "ui/wifi_dialog_page.hpp"
#include "ui_fonts.h"
#include "ui_theme.h"

using namespace input;

namespace {

constexpr const char* kLeftHints[]  = {"START", "CONTROLS", nullptr};
constexpr const char* kRightHints[] = {"B", "BACK", "A", "OPEN", nullptr};

/** The most bytes of a search */
constexpr size_t kMaxQuery = 255;

class ManageAdapter final : public ListAdapter {
public:
    static constexpr int kSearch = 0;
    static constexpr int kTopShows = 1;
    static constexpr int kImport = 2;

    int getCount() const override { return 3; }
    long getItemId(int index) const override { return index; }

    std::shared_ptr<ListItemView> getView(int index) override {
        auto row = std::make_shared<ListRowView>(Fonts_getMedium());
        row->setText(index == kSearch ? "Search" : index == kTopShows ? "Top Shows" : "Import from SD card");
        return row;
    }
};

class PodcastManagePageImpl final : public UiPageImpl<PodcastManagePage>, private ListScreenListener {
public:
    PodcastManagePageImpl() : UiPageImpl(PageKind::FullScreen) {
        screenView.getTitleView().setText("Manage Podcasts");
        screenView.getTitleView().setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        screenView.getButtonHintsView().setHints(kLeftHints, kRightHints);
    }

    std::span<const ButtonRequest> getButtonRequests() const override {
        return ListScreenController::getButtonRequests();
    }

    HelpId getHelpId() const override { return HELP_PODCAST_MANAGE; }
    ThemeSource getSource() const override { return THEME_SOURCE_PODCAST; }

    void onEnter() override {
        screenView.setAdapter(rows);
        list.start();
    }

    void onTick() override { Podcast_update(); }

    void layout(UiLayoutContext& context) override { context.layoutChild(screenView, context.getMaxRect()); }

    bool onButtonEvent(const ButtonEvent& event) override {
        if (list.onButtonEvent(event)) return true;
        if (event.isSingleButton(Button::A)) {
            choose(list.getSelectedIndex());
            return true;
        }
        if (event.isSingleButton(Button::B)) {
            close();
            return true;
        }
        return false;
    }

private:
    void choose(int index) {
        // All of them need the network: the import subscribes to feeds
        getNavigator().openForResult(WifiDialogPage{}, [this, index](const bool& connected) {
            if (!connected) {
                Toast_show("Internet connection required", TOAST_DURATION);
                return;
            }
            if (index == ManageAdapter::kSearch) {
                search();
            } else if (index == ManageAdapter::kImport) {
                PodcastImport_prepareFolder();
                getNavigator().open(PodcastImportPage{});
            } else {
                Podcast_loadCharts(nullptr);
                getNavigator().open(PodcastCatalogPage{PodcastCatalogKind::TopShows});
            }
        });
    }

    void search() {
        getNavigator().openForResult(TextInputPage("Search podcasts", kMaxQuery), [this](const std::string& query) {
            // An empty text is a cancel
            if (query.empty()) return;
            Podcast_startSearch(query.c_str());
            getNavigator().open(PodcastCatalogPage{PodcastCatalogKind::SearchResults});
        });
    }

    ManageAdapter rows;
    ListScreenView screenView{Fonts_getLarge()};
    ListScreenController list{screenView, getScheduler(), *this};
};

}  // namespace

std::unique_ptr<UiPageImplBase> PodcastManagePage::createPageImpl() const {
    return std::make_unique<PodcastManagePageImpl>();
}

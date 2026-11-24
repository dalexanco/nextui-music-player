#include "ui/library_page.hpp"

#include <iterator>
#include <memory>
#include <string>

#include "downloader.h"
#include "ui/downloader_page.hpp"
#include "ui/file_browser_page.hpp"
#include "ui/page/list_screen_controller.hpp"
#include "ui/playlists_page.hpp"
#include "ui/view/list_adapter.hpp"
#include "ui/view/list_row_view.hpp"
#include "ui/view/detail_panel_view.hpp"
#include "ui/view/list_screen_view.hpp"
#include "ui/wifi_dialog_page.hpp"
#include "ui/ytdlp_install_page.hpp"
#include "ui_fonts.h"
#include "ui_theme.h"

using namespace input;

namespace {

/** An item of Library */
enum class LibraryItem : int8_t {
    Files,
    Playlists,
    Downloader,
};

struct LibraryRow {
    LibraryItem item;
    const char* label;
    /** The line of the detail panel, which shows with the setting "Source colours" */
    const char* description;
};

constexpr LibraryRow kRows[] = {
    {LibraryItem::Files,      "Files",      "SD card /Music"     },
    {LibraryItem::Playlists,  "Playlists",  "Your M3U playlists" },
    {LibraryItem::Downloader, "Downloader", "Youtube and the web"},
};

constexpr int kRowCount = static_cast<int>(std::size(kRows));

constexpr const char* kLeftHints[]  = {"START", "CONTROLS", nullptr};
constexpr const char* kRightHints[] = {"B", "BACK", "A", "OPEN", nullptr};

/** The rows of Library */
class LibraryListAdapter final : public ListAdapter {
public:
    int getCount() const override { return kRowCount; }
    long getItemId(int index) const override { return index; }
    std::shared_ptr<ListItemView> getView(int index) override {
        auto row = std::make_shared<ListRowView>(Fonts_getMedium());
        row->setText(kRows[index].label);
        return row;
    }
};

class LibraryPageImpl final : public UiPageImpl<LibraryPage>, private ListScreenListener {
public:
    LibraryPageImpl() : UiPageImpl(PageKind::FullScreen) {
        screenView.getTitleView().setText("Library");
        screenView.getTitleView().setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        screenView.getButtonHintsView().setHints(kLeftHints, kRightHints);
    }

    std::span<const ButtonRequest> getButtonRequests() const override {
        return ListScreenController::getButtonRequests();
    }

    HelpId getHelpId() const override { return HELP_LIBRARY_MENU; }
    ThemeSource getSource() const override { return THEME_SOURCE_MUSIC; }

    void onEnter() override {
        screenView.setAdapter(rows);
        list.start();
    }

    void layout(UiLayoutContext& context) override {
        // The panel belongs to the source colours look, and the setting can change between frames
        screenView.setSidePanel(Theme_isSourceColoursEnabled() ? &panel : nullptr);
        context.layoutChild(screenView, context.getMaxRect());
    }

    bool onButtonEvent(const ButtonEvent& event) override {
        if (list.onButtonEvent(event)) return true;
        if (event.isSingleButton(Button::A)) {
            openItem(list.getSelectedIndex());
            return true;
        }
        if (event.isSingleButton(Button::B)) {
            close();
            return true;
        }
        return false;
    }

private:
    /** Fills the detail panel for the row at `index`: its art tile, its line, and its place in Library */
    void onSelectionChanged(int index) override {
        if (index < 0 || index >= kRowCount) return;
        panel.getArt().setFallback(kRows[index].label, THEME_SOURCE_MUSIC);
        panel.getInfo().setLine(kRows[index].description);
        panel.getInfo().setProgress((index + 1) * 100 / kRowCount);
        panel.getInfo().setPosition("Item " + std::to_string(index + 1) + " of " + std::to_string(kRowCount));
    }

    void openItem(int index) {
        if (index < 0 || index >= kRowCount) return;
        switch (kRows[index].item) {
            case LibraryItem::Files:      getNavigator().open(FileBrowserPage{}); break;
            case LibraryItem::Playlists:  getNavigator().open(PlaylistsPage{}); break;
            case LibraryItem::Downloader: openDownloader(); break;
        }
    }

    /** Waits for Wi-Fi, installs the helpers where they are missing, and opens the Downloader */
    void openDownloader() {
        Downloader_init();
        getNavigator().openForResult(WifiDialogPage{}, [this](const bool& connected) {
            if (!connected) {
                Downloader_cleanup();
                return;
            }
            if (Downloader_isAvailable()) {
                getNavigator().open(DownloaderPage{});
                return;
            }
            getNavigator().openForResult(YtdlpInstallPage{}, [this](const bool& installed) {
                if (installed && Downloader_init() == 0) {
                    getNavigator().open(DownloaderPage{});
                    return;
                }
                Downloader_cleanup();
            });
        });
    }

    LibraryListAdapter rows;
    ListScreenView screenView{Fonts_getLarge()};
    DetailPanelView panel;
    ListScreenController list{screenView, getScheduler(), *this};
};

}  // namespace

std::unique_ptr<UiPageImplBase> LibraryPage::createPageImpl() const {
    return std::make_unique<LibraryPageImpl>();
}

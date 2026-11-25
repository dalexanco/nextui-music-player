#include "ui/file_browser_page.hpp"

#include <sys/stat.h>
#include <unistd.h>

#include <cstdio>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

#include "background.h"
#include "browser.h"
#include "browser_nav.hpp"
#include "db.h"
#include "file_utils.h"
#include "filedb.h"
#include "music_session.h"
#include "toast.h"
#include "ui/add_to_playlist_page.hpp"
#include "ui/confirm_dialog_page.hpp"
#include "ui/now_playing_page.hpp"
#include "ui/choice_options_page.hpp"
#include "ui/page/list_screen_controller.hpp"
#include "ui/view/detail_panel_view.hpp"
#include "ui/view/empty_state_view.hpp"
#include "ui/view/list_adapter.hpp"
#include "ui/view/list_row_view.hpp"
#include "ui/view/list_screen_view.hpp"
#include "ui_fonts.h"
#include "ui_icons.h"
#include "ui_theme.h"

using namespace input;

namespace {

// A file has two actions besides A and B (add to a playlist, delete): they are behind Y OPTIONS. A
// folder has one, shown as it is, and ".." and Play All have none.
constexpr const char* kLeftHints[]        = {"START", "CONTROLS", nullptr};
constexpr const char* kFileLeftHints[]    = {"Y", "OPTIONS", nullptr};
constexpr const char* kFolderLeftHints[]  = {"Y", "PLAYLIST", nullptr};
constexpr const char* kRightHints[]       = {"B", "BACK", "A", "SELECT", nullptr};

/** How often the browser asks the database whether the folder changed, in milliseconds */
constexpr uint32_t kUpdateCheckMs = 100;

/** Where the browser was when its page closed last, thus the next page opens in the same folder */
struct Remembered {
    int dirId    = -1;
    int selected = 0;
    BrowserNav nav;
};
Remembered remembered;

/** Returns true for the ".." row, the way up to the parent folder. */
bool isParentRow(const FileEntry& entry) { return entry.is_dir && std::strcmp(entry.name, "..") == 0; }

/** The rows of the folder that the browser shows */
class BrowserListAdapter final : public ListAdapter {
public:
    BrowserContext& getContext() { return context; }

    int getCount() const override { return context.entry_count; }

    /** The ids of folders and of files are separate, thus the kind of the row is part of the id. */
    long getItemId(int index) const override {
        const FileEntry& entry = context.entries[index];
        return static_cast<long>(entry.db_id) * 4 + (entry.is_dir ? 1 : 0) + (entry.is_play_all ? 2 : 0);
    }

    const FileEntry* get(int index) const {
        return index >= 0 && index < context.entry_count ? &context.entries[index] : nullptr;
    }

    std::shared_ptr<ListItemView> getView(int index) override {
        const FileEntry& entry = context.entries[index];
        auto row               = std::make_shared<ListRowView>(Fonts_getMedium());
        char display[256];
        if (Icons_isLoaded()) {
            // With icons, the names are clean
            if (entry.is_dir || entry.is_play_all) {
                std::snprintf(display, sizeof(display), "%s", entry.name);
            } else {
                get_file_display_name(entry.name, display, sizeof(display));
            }
            SDL_Surface* normal   = nullptr;
            SDL_Surface* selected = nullptr;
            if (entry.is_dir) {
                normal   = Icons_getFolder(THEME_ROLE_PRIMARY, false);
                selected = Icons_getFolder(THEME_ROLE_PRIMARY, true);
            } else if (entry.is_play_all) {
                normal   = Icons_getPlayAll(THEME_ROLE_PRIMARY, false);
                selected = Icons_getPlayAll(THEME_ROLE_PRIMARY, true);
            } else {
                normal   = Icons_getForFormat(entry.format, THEME_ROLE_PRIMARY, false);
                selected = Icons_getForFormat(entry.format, THEME_ROLE_PRIMARY, true);
            }
            row->setIcon(normal, selected);
        } else if (entry.is_dir) {
            std::snprintf(display, sizeof(display), "[%s]", entry.name);
        } else if (entry.is_play_all) {
            std::snprintf(display, sizeof(display), "> %s", entry.name);
        } else {
            get_file_display_name(entry.name, display, sizeof(display));
        }
        row->setText(display);
        return row;
    }

private:
    BrowserContext context{};
};

class FileBrowserPageImpl final : public UiPageImpl<FileBrowserPage>, private ListScreenListener {
public:
    FileBrowserPageImpl() : UiPageImpl(PageKind::FullScreen) {
        screenView.getTitleView().setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        screenView.getButtonHintsView().setHints(kLeftHints, kRightHints);
        emptyState.setMessage("No music files found");
        emptyState.setLine("Add music to /Music on your SD card");
    }

    ~FileBrowserPageImpl() override { Browser_freeEntries(&rows.getContext()); }

    std::span<const ButtonRequest> getButtonRequests() const override {
        return ListScreenController::getButtonRequests();
    }

    HelpId getHelpId() const override { return HELP_BROWSER; }
    ThemeSource getSource() const override { return THEME_SOURCE_MUSIC; }

    void onEnter() override {
        mkdir(get_music_path(), 0755);
        nav = remembered.nav;
        loadDirectory(remembered.dirId >= 0 ? remembered.dirId : Db_getRootDirId());
        screenView.setAdapter(rows);
        list.start();
        if (remembered.dirId >= 0) list.setSelectedIndex(remembered.selected);
        showState();
        getScheduler().postRepeating([this] { refreshOnUpdate(); }, kUpdateCheckMs);
    }

    void onExit() override {
        remembered.dirId    = rows.getContext().dir_id;
        remembered.selected = list.getSelectedIndex() < 0 ? 0 : list.getSelectedIndex();
        remembered.nav      = nav;
    }

    void layout(UiLayoutContext& context) override {
        screenView.setSidePanel(panelShown ? &panel : nullptr);
        context.layoutChild(screenView, context.getMaxRect());
    }

    bool onButtonEvent(const ButtonEvent& event) override {
        if (list.onButtonEvent(event)) return true;
        if (event.isSingleButton(Button::A)) {
            activateSelected();
            return true;
        }
        if (event.isSingleButton(Button::B)) {
            goBack();
            return true;
        }
        if (event.isSingleButton(Button::Y)) {
            openOptions();
            return true;
        }
        return false;
    }

private:
    BrowserContext& browser() { return rows.getContext(); }

    void onSelectionChanged(int index) override {
        updatePanel(index);
        updateHints(index);
    }

    /** The left of the bar follows the row: its options, or START for a row that has none. */
    void updateHints(int index) {
        const FileEntry* entry = rows.get(index);
        const char* const* left = kLeftHints;
        if (entry && !entry->is_play_all && !isParentRow(*entry)) left = entry->is_dir ? kFolderLeftHints : kFileLeftHints;
        screenView.getButtonHintsView().setHints(left, kRightHints);
    }

    /** Y: the options of a file, or the playlist of a folder. ".." and Play All are not targets. */
    void openOptions() {
        const FileEntry* entry = rows.get(list.getSelectedIndex());
        if (!entry || entry->is_play_all || isParentRow(*entry)) return;
        if (entry->is_dir) {
            addToPlaylist();
            return;
        }
        char display[256];
        get_file_display_name(entry->name, display, sizeof(display));
        openActionOptions(getNavigator(), THEME_SOURCE_MUSIC, "FILE", "Music", display,
                          {{"Add to playlist", [this] { addToPlaylist(); }}, {"Delete file", [this] { confirmDelete(); }}});
    }

    /**
     * Fills the detail panel of an audio file: its name, the folder, and its place among the tracks of
     * the folder. A folder, "..", and Play All have no panel: there is nothing to show of them.
     */
    void updatePanel(int index) {
        const FileEntry* entry = rows.get(index);
        panelShown             = entry && !entry->is_dir && !entry->is_play_all;
        if (!panelShown) return;
        char name[256];
        get_file_display_name(entry->name, name, sizeof(name));
        panel.getArt().setFallback(THEME_SOURCE_MUSIC);
        panel.getInfo().setLine(name);
        int track = 0;
        for (int row = 0; row <= index; row++) {
            if (!rows.get(row)->is_dir && !rows.get(row)->is_play_all) track++;
        }
        panel.getInfo().setPosition("Track " + std::to_string(track) + " of " + std::to_string(browser().audio_count) + " in " + getFolderName());
    }

    /** Returns the name of the folder that the browser shows, or "Music" for the root. */
    std::string getFolderName() {
        const std::string path = browser().current_path;
        if (path.empty()) return "Music";
        const size_t slash = path.rfind('/');
        return slash == std::string::npos ? path : path.substr(slash + 1);
    }

    /** Loads the folder and starts its scan, which the update check shows when it ends. */
    void loadDirectory(int dirId) {
        Browser_loadDirectory(&browser(), dirId);
        FileDb_scanDir(dirId, false);
    }

    /** Sets the title (the path of the folder), and the empty state when no folder holds music. */
    void showState() {
        screenView.getTitleView().setText(std::string("/") + browser().current_path);
        screenView.setEmptyState(browser().root_no_music ? &emptyState : nullptr);
    }

    /** Reads the rows again after a change on the card, and keeps the cursor on its row. */
    void refreshOnUpdate() {
        if (!Browser_hasUpdate(&browser()) || !Browser_refresh(&browser())) return;
        list.notifyDataChanged();
        list.setSelectedIndex(browser().selected);
        showState();
    }

    /** Puts the rows of dirId on the screen with the cursor on `selected`, and tells the list. */
    void showDirectory(int selected) {
        list.notifyDataChanged();
        list.setSelectedIndex(selected);
        showState();
    }

    void activateSelected() {
        const FileEntry* entry = rows.get(list.getSelectedIndex());
        if (!entry) return;
        if (isParentRow(*entry)) {
            goUp();
        } else if (entry->is_dir) {
            nav.enter(browser().dir_id, list.getSelectedIndex());
            loadDirectory(entry->db_id);
            showDirectory(0);
        } else if (MusicSession_startFolder(browser().dir_id, entry->is_play_all ? 0 : entry->db_id)) {
            getNavigator().openForResult(NowPlayingPage{}, [this](const bool& queueEnded) {
                if (!queueEnded) return;
                // The queue ended: the browser starts again at the root
                nav.clear();
                loadDirectory(Db_getRootDirId());
                showDirectory(0);
            });
        }
    }

    void goBack() {
        if (browser().is_root) {
            close();
        } else {
            goUp();
        }
    }

    /**
     * Goes to the parent of the folder, which the ".." row names. The cursor goes to the row that the
     * user was on when they entered this folder, where the history knows it, and to the row of the
     * folder that they left otherwise.
     */
    void goUp() {
        BrowserContext& context = browser();
        const int childId       = context.dir_id;
        const int parentId      = context.is_root ? 0 : context.entries[0].db_id;
        int rememberedRow       = nav.leave(parentId);
        loadDirectory(parentId);
        // The browser shows the root when the parent is gone, thus the history is stale
        if (context.dir_id != parentId) {
            nav.clear();
            rememberedRow = -1;
        }
        std::vector<BrowserNavRow> navRows;
        navRows.reserve(static_cast<size_t>(context.entry_count));
        for (int index = 0; index < context.entry_count; index++) {
            navRows.push_back(BrowserNavRow{context.entries[index].is_dir, context.entries[index].db_id});
        }
        showDirectory(BrowserNav::chooseSelection(rememberedRow, childId, navRows));
    }

    void confirmDelete() {
        const FileEntry* entry = rows.get(list.getSelectedIndex());
        if (!entry || entry->is_dir || entry->is_play_all) return;
        char path[512];
        if (!getIndexedFilePath(entry->db_id, path, sizeof(path))) return;
        std::string target = path;
        getNavigator().openForResult(ConfirmDialogPage("Delete File?", entry->name), [this, target](const bool& yes) {
            if (!yes || unlink(target.c_str()) != 0) return;
            const int position = list.getSelectedIndex();
            loadDirectory(browser().dir_id);
            showDirectory(position);
        });
    }

    /** Writes the absolute path of an indexed file to `out`. Returns false when the file has no row. */
    static bool getIndexedFilePath(int fileId, char* out, size_t outSize) {
        DbFileResult* result = Db_getFile(fileId);
        if (!result) return false;
        const bool found = get_music_abspath(result->file.path, out, outSize);
        Db_freeResult(result);
        return found;
    }

    /** Y adds the file or the folder of the row. ".." and Play All are not targets. */
    void addToPlaylist() {
        const FileEntry* entry = rows.get(list.getSelectedIndex());
        if (!entry || entry->is_play_all || isParentRow(*entry)) return;
        getNavigator().open(entry->is_dir ? AddToPlaylistPage::forFolder(entry->db_id)
                                          : AddToPlaylistPage::forFile(entry->db_id));
    }

    BrowserListAdapter rows;
    bool panelShown = false;
    BrowserNav nav;
    ListScreenView screenView{Fonts_getLarge()};
    EmptyStateView emptyState;
    DetailPanelView panel;
    ListScreenController list{screenView, getScheduler(), *this};
};

}  // namespace

std::unique_ptr<UiPageImplBase> FileBrowserPage::createPageImpl() const {
    return std::make_unique<FileBrowserPageImpl>();
}

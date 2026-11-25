#ifndef __BROWSER_NAV_HPP__
#define __BROWSER_NAV_HPP__

#include <array>
#include <span>

/** What the navigation needs to know of a row of the browser. */
struct BrowserNavRow {
    bool isDir;
    int dbId;
};

/**
 * The navigation state of the file browser: where the cursor was in each folder that the user went
 * into, thus B (or the ".." row) puts the cursor back on the row that it left. It reads no
 * database and draws nothing, thus a host test covers it.
 */
class BrowserNav {
public:
    /** The depth that the history keeps. A deeper entry is not recorded, and its way up finds the folder row. */
    static constexpr int kDepth = 32;

    /** Forgets the history. */
    void clear() { top = 0; }

    /** Records that the user went from the folder `dirId`, with the cursor on `selected`, into a child. */
    void enter(int dirId, int selected);

    /**
     * Takes the cursor that the history holds for a way up to the folder `parentId`.
     * Returns: the remembered row, or -1 when the history has none for that folder. A history for
     * another folder is stale, such as after a resume, and is dropped.
     */
    int leave(int parentId);

    /**
     * Chooses the row of the cursor after a way up. `remembered` comes from leave(), or is -1 when
     * the browser is not in the parent that the user asked for: the history is then stale. Without
     * a remembered row the cursor goes to the folder `childId` that the user left, else to the
     * first row. The row stays inside the list.
     */
    static int chooseSelection(int remembered, int childId, std::span<const BrowserNavRow> rows);

private:
    struct Entry {
        int dirId;
        int selected;
    };

    std::array<Entry, kDepth> stack{};
    int top = 0;
};

#endif

#include "browser_nav.hpp"

void BrowserNav::enter(int dirId, int selected) {
    if (top >= kDepth) return;
    stack[static_cast<size_t>(top)] = Entry{dirId, selected};
    top++;
}

int BrowserNav::leave(int parentId) {
    if (top > 0 && stack[static_cast<size_t>(top - 1)].dirId == parentId) {
        top--;
        return stack[static_cast<size_t>(top)].selected;
    }
    top = 0;
    return -1;
}

int BrowserNav::chooseSelection(int remembered, int childId, std::span<const BrowserNavRow> rows) {
    int selected = remembered;
    if (selected < 0) {
        selected = 0;
        for (size_t index = 0; index < rows.size(); index++) {
            if (rows[index].isDir && rows[index].dbId == childId) {
                selected = static_cast<int>(index);
                break;
            }
        }
    }
    const int count = static_cast<int>(rows.size());
    if (selected >= count) selected = count > 0 ? count - 1 : 0;
    return selected;
}

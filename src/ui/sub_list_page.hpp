#ifndef __UI_SUB_LIST_PAGE_HPP__
#define __UI_SUB_LIST_PAGE_HPP__

#include <memory>
#include <string>
#include <variant>

#include "ui/page/ui_page.hpp"
#include "ui_theme.h"

/**
 * What a sub-list shows and does: the queue of the music, and later the stations, the episodes and
 * the chapters. The caller makes it and gives it to the page, which keeps it while it shows. The
 * page reads it again when the user presses nothing, thus it can follow a playback that moves.
 */
class SubListSource {
public:
    virtual ~SubListSource() = default;

    virtual int getCount() const = 0;

    /** Returns the text of the row at `index`: one line, which scrolls while the row is selected. */
    virtual std::string getRowText(int index) const = 0;

    /** Returns the value at the right of the row at `index`, or an empty text for none. */
    virtual std::string getRowValue([[maybe_unused]] int index) const { return {}; }

    /** Returns true for the row that plays now: it gets the playing indicator. */
    virtual bool isPlaying([[maybe_unused]] int index) const { return false; }

    /** Returns the row that the cursor starts on. */
    virtual int getInitialIndex() const { return 0; }

    /** Returns the message of the empty list. */
    virtual std::string getEmptyMessage() const { return "Nothing to show"; }

    /**
     * Called for A on the row at `index`.
     * Returns: true when the page closes
     */
    virtual bool onActivate(int index) = 0;
};

/**
 * The page of a sub-list, reached with X from a player or a list: a title and its kind in small
 * letters, full-width one-line rows with no art, an indicator on the row that plays, and the
 * button hints. A closes it when the source says so, and B closes it.
 */
class SubListPage final : public UiPage {
public:
    using Result = std::monostate;

    /**
     * `title` is the name of the owner (a playlist, a show), `kind` says what the rows are (QUEUE,
     * EPISODES), `verb` is the label of A, and `source` is the source whose accent the page takes.
     */
    SubListPage(std::string title, std::string kind, std::string verb, ThemeSource source,
                std::shared_ptr<SubListSource> rows)
        : title(std::move(title)), kind(std::move(kind)), verb(std::move(verb)), source(source), rows(std::move(rows)) {}

    std::unique_ptr<UiPageImplBase> createPageImpl() const override;

private:
    std::string title;
    std::string kind;
    std::string verb;
    ThemeSource source;
    std::shared_ptr<SubListSource> rows;
};

#endif

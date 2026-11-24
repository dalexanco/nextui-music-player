#ifndef __UI_SUB_LIST_PAGE_HPP__
#define __UI_SUB_LIST_PAGE_HPP__

#include <memory>
#include <string>
#include <variant>

#include <vector>

#include "help_screen.h"
#include "input/input_events.hpp"
#include "ui/page/ui_page.hpp"
#include "ui_theme.h"

/** The mark at the start of a row: a star for what is new, a check for what is done */
enum class SubListMark : uint8_t { None, New, Done };

/**
 * An action of the page besides A and B. The hint bar holds three actions at most (see the hint bar
 * rule of the plan): a source with one key shows it as `button label` at the left of the bar, and a
 * source with several keys shows `Y OPTIONS`, whose panel lists them by `label`. `button` is what
 * the source gets in onKey().
 */
struct SubListKey {
    input::Button button;
    std::string label;
};

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

    /** Returns the mark of the row at `index`. The indicator of the row that plays takes its place. */
    virtual SubListMark getRowMark([[maybe_unused]] int index) const { return SubListMark::None; }

    /**
     * Returns the label of A for the row at `index`, or an empty text for the label of the page.
     * The page reads it again when the cursor moves, and when the version changes.
     */
    virtual std::string getVerb([[maybe_unused]] int index) const { return {}; }

    /** Returns a number that changes when the rows change (a download that moves, a mark): the page then reads them again. */
    virtual long getVersion() const { return 0; }

    /** Called at each frame of the page, for a source that has work of its own (a refresh to follow). */
    virtual void onTick() {}

    /** Called each time the page reads the rows again, before it does. */
    virtual void onReload() {}

    /** Returns the label of the key in the options panel, for the row at `index`. The default is its `label`. */
    virtual std::string getKeyLabel(const SubListKey& key, [[maybe_unused]] int index) const { return key.label; }

    /** Returns the actions of the source besides A and B. */
    virtual std::vector<SubListKey> getKeys() const { return {}; }

    /**
     * Called for the key `button` of getKeys() on the row at `index` (-1 with no row). The page
     * reads the rows again after it.
     */
    virtual void onKey([[maybe_unused]] input::Button button, [[maybe_unused]] int index, [[maybe_unused]] PageNavigator& navigator) {}

    /** Returns the row that the cursor starts on. */
    virtual int getInitialIndex() const { return 0; }

    /** Returns the message of the empty list. */
    virtual std::string getEmptyMessage() const { return "Nothing to show"; }

    /**
     * Called for A on the row at `index`.
     * Returns: true when the page closes
     */
    virtual bool onActivate([[maybe_unused]] int index) { return false; }

    /** The same, for a source that opens a page: `navigator` is the one of the sub-list. */
    virtual bool onActivate(int index, [[maybe_unused]] PageNavigator& navigator) { return onActivate(index); }
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
                std::shared_ptr<SubListSource> rows, HelpId helpId = HELP_NONE)
        : title(std::move(title)),
          kind(std::move(kind)),
          verb(std::move(verb)),
          source(source),
          rows(std::move(rows)),
          helpId(helpId) {}

    std::unique_ptr<UiPageImplBase> createPageImpl() const override;

private:
    std::string title;
    std::string kind;
    std::string verb;
    ThemeSource source;
    std::shared_ptr<SubListSource> rows;
    HelpId helpId;
};

#endif

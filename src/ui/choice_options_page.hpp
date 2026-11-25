#ifndef __UI_CHOICE_OPTIONS_PAGE_HPP__
#define __UI_CHOICE_OPTIONS_PAGE_HPP__

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "ui/page/ui_page.hpp"
#include "ui_theme.h"

/** What the user chose in a ChoiceOptionsPage: the index of the row, or -1 when the page closed with B. */
struct ChoiceResult {
    int index = -1;
};

/**
 * The options of a list (Y): a panel at the right with rows that are actions, as the options of
 * the players. It is the page behind `Y OPTIONS` of the lists: the hint bar holds the action of the
 * list (A), B, and this one, and the other actions of the page are the rows here.
 * A chooses the row and closes the page, B closes it with no choice.
 */
class ChoiceOptionsPage final : public UiPage {
public:
    using Result = ChoiceResult;

    /**
     * `scope` is the small word at the right of the title of the panel (PODCASTS, EPISODE),
     * `meta` and `title` the text at the left (what the options are about), and `source` gives
     * the accent. Each row of `labels` is an action.
     */
    ChoiceOptionsPage(ThemeSource source, std::string scope, std::string meta, std::string title, std::vector<std::string> labels)
        : source(source), scope(std::move(scope)), meta(std::move(meta)), title(std::move(title)), labels(std::move(labels)) {}

    std::unique_ptr<UiPageImplBase> createPageImpl() const override;

private:
    ThemeSource source;
    std::string scope;
    std::string meta;
    std::string title;
    std::vector<std::string> labels;
};

/** A row of the options of a list, with what it does. */
struct ChoiceAction {
    std::string label;
    std::function<void()> run;
};

/**
 * Opens the options of a list (Y OPTIONS) with `actions` as rows, and runs the one that the user
 * chooses once the options page has closed. Nothing runs when B closes it.
 */
inline void openActionOptions(PageNavigator& navigator, ThemeSource source, std::string scope, std::string meta,
                              std::string title, std::vector<ChoiceAction> actions) {
    std::vector<std::string> labels;
    for (const ChoiceAction& action : actions) labels.push_back(action.label);
    auto chosen = std::make_shared<std::vector<ChoiceAction>>(std::move(actions));
    navigator.openForResult(ChoiceOptionsPage(source, std::move(scope), std::move(meta), std::move(title), std::move(labels)),
                            [chosen](const ChoiceResult& result) {
                                if (result.index >= 0 && result.index < static_cast<int>(chosen->size())) (*chosen)[static_cast<size_t>(result.index)].run();
                            });
}

#endif

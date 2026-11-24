#include "ui/controls_help_page.hpp"

#include <memory>
#include <vector>

#include "ui/view/dialog_view.hpp"
#include "ui/view/text_view.hpp"
#include "ui_fonts.h"
#include "ui_main.h"
#include "ui_theme.h"

// The NextUI headers declare C functions and have no guard of their own
extern "C" {
#include "defines.h"
#include "api.h"
}

using namespace input;

namespace {

/** The rows of a controls help: the button on the left, and its action on the right. */
class ControlsHelpView : public UiView {
public:
    /** Shows the rows of controls, which ends with a row of NULLs, in font. */
    ControlsHelpView(const ControlHelp* controls, TTF_Font* font);

    /** Returns the height of all rows, in pixels. */
    int getHeight() const;

    void layout(UiLayoutContext& context) override;

private:
    struct Row {
        std::unique_ptr<TextView> button;
        std::unique_ptr<TextView> action;
    };

    std::vector<Row> rows;
};

int getRowHeight() { return SCALE1(18); }

/** The width of the column of the buttons */
int getButtonWidth() { return SCALE1(75); }

ControlsHelpView::ControlsHelpView(const ControlHelp* controls, TTF_Font* font) {
    for (const ControlHelp* control = controls; control->button; control++) {
        Row row{
            .button = std::make_unique<TextView>(font, true),
            .action = std::make_unique<TextView>(font, true),
        };
        row.button->setText(control->button);
        row.button->setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        row.action->setText(control->action);
        row.action->setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
        rows.push_back(std::move(row));
    }
}

int ControlsHelpView::getHeight() const {
    return static_cast<int>(rows.size()) * getRowHeight();
}

void ControlsHelpView::layout(UiLayoutContext& context) {
    const int width = context.getMaxSize().w;
    int y           = 0;
    for (Row& row : rows) {
        context.layoutChild(*row.button, SDL_Rect{0, y, getButtonWidth(), getRowHeight()});
        context.layoutChild(*row.action, SDL_Rect{getButtonWidth(), y, width - getButtonWidth(), getRowHeight()});
        y += getRowHeight();
    }
    context.setSize(UiSize{width, getHeight()});
}

/**
 * The buttons that close Controls Help when pressed. START closes it on a short press or a long
 * press, thus START with a volume button does not close it.
 */
constexpr uint32_t kHelpCloseButtons =
    getButtonBit(Button::A) | getButtonBit(Button::B) | getButtonBit(Button::X) | getButtonBit(Button::Y) |
    getButtonBit(Button::Select) | getButtonBit(Button::DpadUp) |
    getButtonBit(Button::DpadDown) | getButtonBit(Button::DpadLeft) | getButtonBit(Button::DpadRight) |
    getButtonBit(Button::L1) | getButtonBit(Button::R1) | getButtonBit(Button::Menu);

/** Returns the source whose accent a help dialog takes: the one of the screen that it explains. */
ThemeSource getHelpSource(HelpId helpId) {
    switch (helpId) {
        case HELP_LIBRARY_MENU:
        case HELP_BROWSER:
        case HELP_PLAYER:
        case HELP_PLAYER_OPTIONS:
        case HELP_QUEUE:
        case HELP_PLAYLIST_LIST:
        case HELP_PLAYLIST_DETAIL:
        case HELP_DOWNLOADER_MENU:
        case HELP_DOWNLOADER_SEARCHING:
        case HELP_DOWNLOADER_RESULTS:
        case HELP_DOWNLOADER_QUEUE:
        case HELP_DOWNLOADER_YTDLP:         return THEME_SOURCE_MUSIC;
        case HELP_RADIO_LIST:
        case HELP_RADIO_PLAYING:
        case HELP_RADIO_MANAGE:
        case HELP_RADIO_BROWSE:
        case HELP_RADIO_STATIONS:            return THEME_SOURCE_RADIO;
        case HELP_PODCAST_MENU:
        case HELP_PODCAST_MANAGE:
        case HELP_PODCAST_TOP_SHOWS:
        case HELP_PODCAST_SEARCH_RESULTS:
        case HELP_PODCAST_EPISODES:
        case HELP_PODCAST_DOWNLOADS:
        case HELP_PODCAST_PLAYING:          return THEME_SOURCE_PODCAST;
        case HELP_DEFAULT:
        case HELP_NONE:
        case HELP_MAIN_MENU:
        case HELP_KEYBOARD:
        case HELP_SETTINGS:
        case HELP_ABOUT:                    break;
    }
    return THEME_SOURCE_NEUTRAL;
}

class ControlsHelpPageImpl final : public UiPageImpl<ControlsHelpPage> {
public:
    explicit ControlsHelpPageImpl(HelpId helpId)
        : UiPageImpl(PageKind::Dialog),
          helpId(helpId),
          dialog(SCALE1(240), DialogView::Alignment::Left, Fonts_getMedium(), Fonts_getSmall()),
          rows(get_controls_help(helpId, &title), Fonts_getSmall()) {
        dialog.setTitle(title);
        dialog.setContent(&rows);
        dialog.setHint("Press any button to close");
    }

    void layout(UiLayoutContext& context) override {
        // With the source colours, the border takes the accent of the screen that the help explains
        if (Theme_isSourceColoursEnabled()) {
            dialog.setEdgeColor(Theme_getAccent(getHelpSource(helpId)));
        } else {
            dialog.clearEdgeColor();
        }
        context.layoutChild(dialog, context.getMaxRect());
    }

    bool onRawEvent(const RawEvent& event) override {
        if (const auto* typedEvent = std::get_if<event::ButtonPressed>(&event.kind)) {
            if (getButtonBit(typedEvent->button) & kHelpCloseButtons) close();
        }
        // A media key goes on to the earphone buttons of the global handler
        return isButtonEvent(event);
    }

    bool onButtonEvent(const ButtonEvent& event) override {
        if (event.isSingleButton(Button::Start)) close();
        return true;
    }

private:
    HelpId helpId;
    const char* title = nullptr;
    DialogView dialog;
    ControlsHelpView rows;
};

}  // namespace

std::unique_ptr<UiPageImplBase> ControlsHelpPage::createPageImpl() const {
    return std::make_unique<ControlsHelpPageImpl>(helpId);
}

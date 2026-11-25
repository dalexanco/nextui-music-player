#include "ui/choice_options_page.hpp"

#include <variant>

#include "ui/page/list_screen_controller.hpp"
#include "ui/view/art_fade_view.hpp"
#include "ui/view/box_view.hpp"
#include "ui/view/options_header_view.hpp"
#include "ui/view/options_panel_view.hpp"
#include "ui/view/screen_view.hpp"
#include "ui_fonts.h"
#include "ui_theme.h"

using namespace input;

namespace {

constexpr const char* kRightHints[] = {"B", "CLOSE", "A", "CHOOSE", nullptr};

class ChoiceOptionsPageImpl final : public UiPageImpl<ChoiceOptionsPage> {
public:
    ChoiceOptionsPageImpl(ThemeSource source, const std::string& scope, const std::string& meta, const std::string& title,
                          const std::vector<std::string>& labels)
        : UiPageImpl(PageKind::FullScreen), source(source), labels(labels) {
        screenView.getButtonHintsView().setHints(nullptr, kRightHints);
        // The dim over the list that shows behind the panel
        SDL_Color dim = Theme_getColor(THEME_ROLE_PAGE_BACKGROUND, false);
        dim.a         = 158;
        dimView.setColor(dim);
        artView.setSource(source);
        header.setMeta(meta);
        header.setTitle(title);
        panel.setScope(scope);
    }

    std::span<const ButtonRequest> getButtonRequests() const override {
        return ListScreenController::getButtonRequests();
    }

    HelpId getHelpId() const override { return HELP_PLAYER_OPTIONS; }
    ThemeSource getSource() const override { return source; }

    void onEnter() override {
        std::vector<OptionRow> rows;
        for (const std::string& label : labels) rows.push_back({label, ""});
        panel.setRows(rows);
    }

    void layout(UiLayoutContext& context) override {
        const UiSize screen = context.getMaxSize();
        context.layoutChild(artView, SDL_Rect{screen.w - screen.w * 620 / 1024, 0, screen.w * 620 / 1024, screen.h});
        context.layoutChild(dimView, context.getMaxRect());
        context.layoutChild(screenView, context.getMaxRect());
        context.layoutChild(header, context.getMaxRect());
        context.layoutChild(panel, context.getMaxRect());
    }

    bool onButtonEvent(const ButtonEvent& event) override {
        const auto* shortPress = std::get_if<event::ShortPress>(&event.kind);
        const auto* repeat     = std::get_if<event::Repeat>(&event.kind);
        if (!shortPress && !repeat) return false;
        const Button button = shortPress ? shortPress->button : repeat->button;
        switch (button) {
            case Button::DpadUp:   panel.moveSelection(-1); return true;
            case Button::DpadDown: panel.moveSelection(1); return true;
            case Button::A:
                if (shortPress) finish(ChoiceResult{panel.getSelectedIndex()});
                return true;
            case Button::B:
                if (shortPress) close();
                return true;
            default: return false;
        }
    }

private:
    ThemeSource source;
    std::vector<std::string> labels;
    ArtFadeView artView;
    BoxView dimView;
    ScreenView screenView{Fonts_getLarge()};
    OptionsHeaderView header;
    OptionsPanelView panel;
};

}  // namespace

std::unique_ptr<UiPageImplBase> ChoiceOptionsPage::createPageImpl() const {
    return std::make_unique<ChoiceOptionsPageImpl>(source, scope, meta, title, labels);
}

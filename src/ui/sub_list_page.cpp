#include "ui/sub_list_page.hpp"

#include <memory>
#include <string>
#include <vector>

#include "ui/page/list_screen_controller.hpp"
#include "ui/view/empty_state_view.hpp"
#include "ui/view/list_adapter.hpp"
#include "ui/view/list_row_view.hpp"
#include "ui/view/list_screen_view.hpp"
#include "ui/view/text_view.hpp"
#include "ui_fonts.h"
#include "ui_theme.h"

// The NextUI headers declare C functions and have no guard of their own
extern "C" {
#include "defines.h"
#include "api.h"
}

using namespace input;

namespace {

constexpr const char* kLeftHints[] = {"START", "CONTROLS", nullptr};

/** How often the page asks the source which row plays, in milliseconds */
constexpr uint32_t kPlayingCheckMs = 500;

/** The side of the surface of the playing indicator, in pixels before the scale of the screen */
constexpr int kIndicatorSide = 24;

/**
 * The indicator of the row that plays: three bars of different heights, in the colors of a row
 * that is selected or not. The surfaces belong to the program, and the icon views only read them.
 */
class PlayingIndicator {
public:
    static PlayingIndicator& get() {
        static PlayingIndicator instance;
        return instance;
    }

    SDL_Surface* getSurface(bool selected) {
        const SDL_Color wanted = Theme_getColor(THEME_ROLE_PRIMARY, selected);
        Slot& slot             = slots[selected ? 1 : 0];
        if (slot.surface && slot.color.r == wanted.r && slot.color.g == wanted.g && slot.color.b == wanted.b) {
            return slot.surface;
        }
        // An older surface can sit in a row view, thus it stays alive: the program frees it at the end
        slot.color   = wanted;
        slot.surface = draw(wanted);
        return slot.surface;
    }

private:
    struct Slot {
        SDL_Surface* surface = nullptr;
        SDL_Color color{};
    };

    static SDL_Surface* draw(SDL_Color color) {
        SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormat(0, kIndicatorSide, kIndicatorSide, 32, SDL_PIXELFORMAT_ARGB8888);
        if (!surface) return nullptr;
        SDL_FillRect(surface, nullptr, SDL_MapRGBA(surface->format, color.r, color.g, color.b, 0));
        const Uint32 ink       = SDL_MapRGBA(surface->format, color.r, color.g, color.b, color.a);
        const int heights[]    = {12, 20, 16};
        const int barWidth     = 5;
        const int gap          = 3;
        const int groupWidth   = 3 * barWidth + 2 * gap;
        const int left         = (kIndicatorSide - groupWidth) / 2;
        for (int bar = 0; bar < 3; bar++) {
            SDL_Rect rect{left + bar * (barWidth + gap), (kIndicatorSide + 20) / 2 - heights[bar], barWidth, heights[bar]};
            SDL_FillRect(surface, &rect, ink);
        }
        return surface;
    }

    Slot slots[2];
};

/** The rows of a source. Each row keeps its view, thus the indicator moves with no new row. */
class SubListAdapter final : public ListAdapter {
public:
    explicit SubListAdapter(std::shared_ptr<SubListSource> source) : source(std::move(source)) {}

    SubListSource& getSource() { return *source; }

    int getCount() const override { return source->getCount(); }
    long getItemId(int index) const override { return index; }

    std::shared_ptr<ListItemView> getView(int index) override {
        auto row = std::make_shared<ListRowView>(Fonts_getMedium());
        row->setText(source->getRowText(index));
        const std::string value = source->getRowValue(index);
        if (!value.empty()) {
            auto text = std::make_shared<TextView>(Fonts_getTiny(), false);
            text->setText(value);
            text->setColor(Theme_getColor(THEME_ROLE_PRIMARY, false));
            row->setRightText(std::move(text));
        }
        setPlaying(*row, source->isPlaying(index));
        views.resize(static_cast<size_t>(source->getCount()));
        if (index >= 0 && index < static_cast<int>(views.size())) views[static_cast<size_t>(index)] = row;
        return row;
    }

    /** Puts the indicator on the row that plays now. Returns true when it moved. */
    bool refreshPlaying() {
        int playing = -1;
        for (int index = 0; index < source->getCount(); index++) {
            if (source->isPlaying(index)) {
                playing = index;
                break;
            }
        }
        if (playing == playingIndex) return false;
        for (const int index : {playingIndex, playing}) {
            if (index < 0 || index >= static_cast<int>(views.size())) continue;
            if (auto row = views[static_cast<size_t>(index)].lock()) setPlaying(*row, index == playing);
        }
        playingIndex = playing;
        return true;
    }

private:
    void setPlaying(ListRowView& row, bool playing) {
        if (playing) {
            row.setIcon(PlayingIndicator::get().getSurface(false), PlayingIndicator::get().getSurface(true));
        } else {
            row.setIcon(nullptr, nullptr);
        }
    }

    std::shared_ptr<SubListSource> source;
    std::vector<std::weak_ptr<ListRowView>> views;
    int playingIndex = -1;
};

class SubListPageImpl final : public UiPageImpl<SubListPage>, private ListScreenListener {
public:
    SubListPageImpl(std::string title, std::string kind, std::string verb, ThemeSource source,
                    std::shared_ptr<SubListSource> rows, HelpId helpId)
        : UiPageImpl(PageKind::FullScreen), source(source), helpId(helpId), rows(std::move(rows)) {
        screenView.getTitleView().setText(title);
        screenView.getTitleView().setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        kindView.setText(kind);
        kindView.setColor(Theme_getColor(THEME_ROLE_SECONDARY, false));
        rightHints[0] = "B";
        rightHints[1] = "BACK";
        rightHints[2] = "A";
        this->verb    = std::move(verb);
        rightHints[3] = this->verb.c_str();
        rightHints[4] = nullptr;
        screenView.getButtonHintsView().setHints(kLeftHints, rightHints);
        emptyState.setMessage(this->rows.getSource().getEmptyMessage());
        screenView.setEmptyState(&emptyState);
    }

    std::span<const ButtonRequest> getButtonRequests() const override {
        return ListScreenController::getButtonRequests();
    }

    ThemeSource getSource() const override { return source; }
    HelpId getHelpId() const override { return helpId; }

    void onEnter() override {
        screenView.setAdapter(rows);
        list.start();
        list.setSelectedIndex(rows.getSource().getInitialIndex());
        getScheduler().postRepeating([this] { rows.refreshPlaying(); }, kPlayingCheckMs);
    }

    void layout(UiLayoutContext& context) override {
        context.layoutChild(screenView, context.getMaxRect());
        // The kind sits after the title, at its foot, and gives way when the title takes the room
        const UiSize screen       = context.getMaxSize();
        const int padding         = SCALE1(PADDING);
        const int titleX          = padding + SCALE1(BUTTON_PADDING);
        UiLayoutContext measuring = context.createMeasuringContext();
        const UiSize titleSize    = measuring.layoutChild(screenView.getTitleView(), SDL_Rect{0, 0, screen.w / 2, SCALE1(PILL_SIZE)}).getSize();
        const UiSize kindSize     = measuring.layoutChild(kindView, SDL_Rect{0, 0, screen.w / 2, SCALE1(PILL_SIZE)}).getSize();
        const int x               = titleX + titleSize.w + SCALE1(12);
        // The status pill takes the right of the row
        if (x + kindSize.w > screen.w - SCALE1(PADDING * 2 + 120)) return;
        const int titleY = padding + (SCALE1(PILL_SIZE) - titleSize.h) / 2;
        context.layoutChild(kindView, SDL_Rect{x, titleY + titleSize.h - kindSize.h - SCALE1(4), kindSize.w, kindSize.h});
    }

    bool onButtonEvent(const ButtonEvent& event) override {
        if (list.onButtonEvent(event)) return true;
        if (event.isSingleButton(Button::A)) {
            const int index = list.getSelectedIndex();
            if (index >= 0 && rows.getSource().onActivate(index)) close();
            return true;
        }
        if (event.isSingleButton(Button::B)) {
            close();
            return true;
        }
        return false;
    }

private:
    ThemeSource source;
    HelpId helpId;
    std::string verb;
    const char* rightHints[5] = {};
    SubListAdapter rows;
    ListScreenView screenView{Fonts_getLarge()};
    EmptyStateView emptyState;
    TextView kindView{Fonts_getSmall(), true};
    ListScreenController list{screenView, getScheduler(), *this};
};

}  // namespace

std::unique_ptr<UiPageImplBase> SubListPage::createPageImpl() const {
    return std::make_unique<SubListPageImpl>(title, kind, verb, source, rows, helpId);
}

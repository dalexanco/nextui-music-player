#include "ui/resume_page.hpp"

#include "music_session.h"
#include "resume.h"
#include "ui/now_playing_page.hpp"

namespace {

/**
 * Starts the music of the Resume record and opens the player over itself. It has no view: it closes
 * when the player returns, or at once when the record cannot play.
 */
class ResumePageImpl final : public UiPageImpl<ResumePage> {
public:
    ResumePageImpl() : UiPageImpl(PageKind::FullScreen) {}

    void onEnter() override {
        const ResumeState* resume = Resume_getState();
        if (!resume || !MusicSession_startResume(resume)) {
            close();
            return;
        }
        started = true;
        getNavigator().open(NowPlayingPage{});
    }

    /** The player closed */
    void resume() override {
        if (started) close();
    }

    void layout(UiLayoutContext&) override {}

private:
    bool started = false;
};

}  // namespace

std::unique_ptr<UiPageImplBase> ResumePage::createPageImpl() const {
    return std::make_unique<ResumePageImpl>();
}

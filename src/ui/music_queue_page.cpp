#include "ui/music_queue_page.hpp"

#include "music_session.h"
#include "player.h"

namespace {

/** Returns the name of a track with no audio extension, which the names of a folder queue keep. */
std::string getTrackLabel(const char* name) {
    std::string label = name;
    const size_t dot  = label.rfind('.');
    if (dot != std::string::npos && dot > 0 && Player_detectFormat(label.c_str()) != AUDIO_FORMAT_UNKNOWN) {
        label.resize(dot);
    }
    return label;
}

/** The tracks of the queue of the music session. A plays the track, and the page closes. */
class MusicQueueSource final : public SubListSource {
public:
    int getCount() const override { return MusicSession_getTrackCount(); }

    std::string getRowText(int index) const override {
        const PlaylistTrack* track = MusicSession_getTrack(index);
        return track ? getTrackLabel(track->name) : std::string();
    }

    bool isPlaying(int index) const override { return index == MusicSession_getCurrentIndex(); }

    int getInitialIndex() const override {
        const int current = MusicSession_getCurrentIndex();
        return current < 0 ? 0 : current;
    }

    std::string getEmptyMessage() const override { return "The queue is empty"; }

    bool onActivate(int index) override { return MusicSession_playTrack(index); }
};

}  // namespace

SubListPage makeMusicQueuePage() {
    return SubListPage("Music", "QUEUE", "PLAY", THEME_SOURCE_MUSIC, std::make_shared<MusicQueueSource>(), HELP_QUEUE);
}

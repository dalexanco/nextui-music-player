#include "radio_player_state.hpp"

#include <algorithm>
#include <cstdio>

RadioView getRadioView(const RadioViewInput& input) {
    if (input.waitingToStart) return RadioView::Connecting;
    switch (input.state) {
        case RADIO_STATE_CONNECTING: return RadioView::Connecting;
        case RADIO_STATE_BUFFERING:
            if (input.bufferLevel >= kRadioLowBuffer) return RadioView::Live;
            return input.wasHealthy ? RadioView::Rebuffering : RadioView::Connecting;
        case RADIO_STATE_PLAYING: return RadioView::Live;
        case RADIO_STATE_ERROR:   return RadioView::Offline;
        case RADIO_STATE_STOPPED: return RadioView::Paused;
    }
    return RadioView::Paused;
}

bool isRadioStreamHealthy(RadioState state, float bufferLevel) {
    return state == RADIO_STATE_PLAYING || (state == RADIO_STATE_BUFFERING && bufferLevel >= kRadioLowBuffer);
}

const char* getRadioChipText(RadioView view) {
    switch (view) {
        case RadioView::Live:        return "LIVE";
        case RadioView::Connecting:  return "CONNECTING";
        case RadioView::Rebuffering: return "RECONNECTING";
        case RadioView::Paused:      return "PAUSED";
        case RadioView::Offline:     return "OFFLINE";
    }
    return "";
}

const char* getRadioActionLabel(RadioView view) {
    switch (view) {
        case RadioView::Live:        return "STOP";
        case RadioView::Connecting:
        case RadioView::Rebuffering: return "CANCEL";
        case RadioView::Paused:      return "LISTEN";
        case RadioView::Offline:     return "RETRY";
    }
    return "";
}

std::string getRadioStatusLine(RadioView view, const char* errorMessage) {
    switch (view) {
        case RadioView::Connecting:  return "Connecting to the stream...";
        case RadioView::Rebuffering: return "Stream interrupted, buffering again";
        case RadioView::Offline:
            return errorMessage && errorMessage[0] ? errorMessage : "Stream unavailable, check the network";
        case RadioView::Paused: return "Stopped";
        case RadioView::Live:   break;
    }
    return {};
}

int getRadioBufferPercent(float bufferLevel) {
    return std::clamp(static_cast<int>(bufferLevel * 100.0f + 0.5f), 0, 100);
}

std::string formatListening(uint32_t milliseconds) {
    const uint32_t minutes = milliseconds / 60000;
    if (minutes == 0) return "Just started";
    char text[48];
    if (minutes >= 60) {
        std::snprintf(text, sizeof(text), "Listening for %u h %02u", minutes / 60, minutes % 60);
    } else {
        std::snprintf(text, sizeof(text), "Listening for %u min", minutes);
    }
    return text;
}

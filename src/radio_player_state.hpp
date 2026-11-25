#ifndef __RADIO_PLAYER_STATE_HPP__
#define __RADIO_PLAYER_STATE_HPP__

#include <cstdint>
#include <string>

#include "radio.h"

/*
 * What the radio player says of the stream, as pure functions of the state of the radio, thus a
 * host test checks them with no stream. The radio keeps its own state machine, and the page only
 * reads it.
 */

/** The look of the player: the chip of the state, the row of the progress and the verb of A */
enum class RadioView : uint8_t {
    /** The stream plays: the LIVE chip */
    Live,
    /** A station connects, or its first buffer fills */
    Connecting,
    /** The stream that played ran low on sound and fills its buffer again */
    Rebuffering,
    /** The user stopped the station */
    Paused,
    /** The stream failed: the station is offline until the user retries */
    Offline,
};

/** What the page knows of the radio at one moment */
struct RadioViewInput {
    RadioState state;
    /** A station waits for its screen, before its stream starts */
    bool waitingToStart;
    /** The buffer level from 0 to 1 */
    float bufferLevel;
    /** The stream of this station played with a healthy buffer since it started */
    bool wasHealthy;
};

/** The buffer level under which a stream that plays has too little sound */
constexpr float kRadioLowBuffer = 0.5f;

/** Returns the look for the state of the radio. */
RadioView getRadioView(const RadioViewInput& input);

/** Returns true while the stream plays with enough sound. A stream that played and ran low is not. */
bool isRadioStreamHealthy(RadioState state, float bufferLevel);

/** Returns the word of the chip of the look: "LIVE", "CONNECTING", "RECONNECTING", "PAUSED", "OFFLINE". */
const char* getRadioChipText(RadioView view);

/** Returns the label of A for the look: STOP, CANCEL, LISTEN, RETRY. */
const char* getRadioActionLabel(RadioView view);

/** Returns the line under the station name for a look that is not live, or "" for the live one. */
std::string getRadioStatusLine(RadioView view, const char* errorMessage);

/** Returns the buffer level as a percent from 0 to 100. */
int getRadioBufferPercent(float bufferLevel);

/** Returns "Listening for 12 min", "Listening for 1 h 05", or "Just started" under a minute. */
std::string formatListening(uint32_t milliseconds);

#endif

#ifndef __RADIO_FAVOURITES_H__
#define __RADIO_FAVOURITES_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdio.h>

#include "radio.h"

// The stations that the user marked as favourite, by URL. The set is kept apart from the station
// list: the list file keeps its format, and a station that is renamed keeps its mark.

typedef struct {
    char urls[RADIO_MAX_STATIONS][RADIO_MAX_URL];
    int count;
} RadioFavourites;

// Returns true when `url` is a favourite.
bool RadioFavourites_has(const RadioFavourites* set, const char* url);

// Marks or unmarks `url`. Returns false when the mark does not fit: the set is full.
bool RadioFavourites_set(RadioFavourites* set, const char* url, bool favourite);

// Flips the mark of `url`. Returns the new state of the mark.
bool RadioFavourites_toggle(RadioFavourites* set, const char* url);

// Reads one URL per line from `file`, in place of the content of the set. Returns the count.
int RadioFavourites_read(RadioFavourites* set, FILE* file);

// Writes one URL per line to `file`.
void RadioFavourites_write(const RadioFavourites* set, FILE* file);

// The set of the radio, in the user data of the pak. Loads at the first call. A change saves.
bool RadioFavourites_isFavourite(const char* url);
bool RadioFavourites_toggleFavourite(const char* url);
void RadioFavourites_forget(const char* url);

#ifdef __cplusplus
}
#endif

#endif

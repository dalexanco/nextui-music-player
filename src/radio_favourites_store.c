#include <stdio.h>

#include "file_utils.h"
#include "radio_favourites.h"

// The set of the radio, kept in `radio/favourites.txt` of the user data

static RadioFavourites favourites;
static bool loaded = false;

static void load(void) {
    if (loaded) return;
    loaded = true;
    char path[512];
    const int length = userdata_snpath("radio/favourites.txt", path, sizeof(path));
    if (length < 0 || (size_t)length >= sizeof(path)) return;
    FILE* file = fopen(path, "r");
    if (!file) return;
    RadioFavourites_read(&favourites, file);
    fclose(file);
}

static void save(void) {
    if (!userdata_mkdir("radio")) return;
    char path[512];
    const int length = userdata_snpath("radio/favourites.txt", path, sizeof(path));
    if (length < 0 || (size_t)length >= sizeof(path)) return;
    FILE* file = fopen(path, "w");
    if (!file) return;
    RadioFavourites_write(&favourites, file);
    fclose(file);
}

bool RadioFavourites_isFavourite(const char* url) {
    load();
    return RadioFavourites_has(&favourites, url);
}

bool RadioFavourites_toggleFavourite(const char* url) {
    load();
    const bool now = RadioFavourites_toggle(&favourites, url);
    save();
    return now;
}

void RadioFavourites_forget(const char* url) {
    load();
    if (!RadioFavourites_has(&favourites, url)) return;
    RadioFavourites_set(&favourites, url, false);
    save();
}

#include "radio_favourites.h"

#include <string.h>

static int find_url(const RadioFavourites* set, const char* url) {
    for (int i = 0; i < set->count; i++) {
        if (strcmp(set->urls[i], url) == 0) return i;
    }
    return -1;
}

bool RadioFavourites_has(const RadioFavourites* set, const char* url) {
    return url && url[0] && find_url(set, url) >= 0;
}

bool RadioFavourites_set(RadioFavourites* set, const char* url, bool favourite) {
    if (!url || !url[0] || strlen(url) >= RADIO_MAX_URL) return false;
    int index = find_url(set, url);
    if (!favourite) {
        if (index < 0) return true;
        memmove(set->urls[index], set->urls[index + 1], (size_t)(set->count - index - 1) * RADIO_MAX_URL);
        set->count--;
        return true;
    }
    if (index >= 0) return true;
    if (set->count >= RADIO_MAX_STATIONS) return false;
    strcpy(set->urls[set->count], url);
    set->count++;
    return true;
}

bool RadioFavourites_toggle(RadioFavourites* set, const char* url) {
    const bool now = !RadioFavourites_has(set, url);
    if (!RadioFavourites_set(set, url, now)) return !now;
    return now;
}

int RadioFavourites_read(RadioFavourites* set, FILE* file) {
    set->count = 0;
    char line[RADIO_MAX_URL + 8];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\r\n")] = '\0';
        if (line[0]) RadioFavourites_set(set, line, true);
    }
    return set->count;
}

void RadioFavourites_write(const RadioFavourites* set, FILE* file) {
    for (int i = 0; i < set->count; i++) fprintf(file, "%s\n", set->urls[i]);
}

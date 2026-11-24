#include "podcast_import_parse.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

#include "yxml.h"

static bool is_space(char c) {
    return c == ' ' || c == '\t' || c == '\r' || c == '\n';
}

static void trim_in_place(char* text) {
    size_t length = strlen(text);
    while (length > 0 && is_space(text[length - 1])) text[--length] = '\0';
    size_t lead = 0;
    while (text[lead] && is_space(text[lead])) lead++;
    if (lead) memmove(text, text + lead, strlen(text + lead) + 1);
}

static bool has_prefix(const char* text, const char* prefix) {
    return strncasecmp(text, prefix, strlen(prefix)) == 0;
}

PodcastImportFormat podcast_import_format_of(const char* filename) {
    if (!filename || !filename[0] || filename[0] == '.') return PODCAST_IMPORT_FORMAT_NONE;
    const char* dot = strrchr(filename, '.');
    if (!dot) return PODCAST_IMPORT_FORMAT_NONE;
    if (strcasecmp(dot, ".txt") == 0) return PODCAST_IMPORT_FORMAT_TEXT;
    if (strcasecmp(dot, ".opml") == 0) return PODCAST_IMPORT_FORMAT_OPML;
    if (strcasecmp(dot, ".url") == 0) return PODCAST_IMPORT_FORMAT_URL;
    return PODCAST_IMPORT_FORMAT_NONE;
}

bool podcast_import_is_failure_report(const char* filename) {
    if (!filename) return false;
    static const char suffix[] = ".failed.txt";
    size_t length = strlen(filename);
    return length > sizeof(suffix) - 1 && strcasecmp(filename + length - (sizeof(suffix) - 1), suffix) == 0;
}

bool podcast_import_normalize_url(const char* text, char* out, size_t out_size) {
    if (!text || !out || out_size < PODCAST_MAX_URL) return false;
    out[0] = '\0';

    char url[PODCAST_MAX_URL + 16];
    size_t length = strlen(text);
    if (length >= sizeof(url)) return false;
    memcpy(url, text, length + 1);
    trim_in_place(url);

    // feed:https://host/x, feed://host/x, itpc://host/x and pcast://host/x name the same feed
    const char* rest = url;
    if (has_prefix(rest, "feed:") && strncmp(rest + 5, "//", 2) != 0) rest += 5;
    const char* scheme_end = strstr(rest, "://");
    if (!scheme_end) return false;

    char scheme[8];
    size_t scheme_length = (size_t)(scheme_end - rest);
    if (scheme_length == 0 || scheme_length >= sizeof(scheme)) return false;
    for (size_t i = 0; i < scheme_length; i++) scheme[i] = (char)tolower((unsigned char)rest[i]);
    scheme[scheme_length] = '\0';

    const char* canonical;
    if (strcmp(scheme, "http") == 0) {
        canonical = "http";
    } else if (strcmp(scheme, "https") == 0 || strcmp(scheme, "feed") == 0 || strcmp(scheme, "itpc") == 0 ||
               strcmp(scheme, "pcast") == 0) {
        canonical = "https";
    } else {
        return false;
    }

    const char* after = scheme_end + 3;
    // The fragment is never sent to the server
    size_t body_length = strcspn(after, "#");
    if (body_length == 0) return false;

    // The host ends at the first / ? or end, and is case-insensitive
    size_t host_length = strcspn(after, "/?#");
    if (host_length == 0 || host_length > body_length) return false;

    for (size_t i = 0; i < body_length; i++) {
        unsigned char c = (unsigned char)after[i];
        if (c <= ' ' || c == 0x7f || c == '"' || c == '\'' || c == '`' || c == '\\') return false;
    }

    size_t needed = strlen(canonical) + 3 + body_length + 1;
    if (needed > out_size) return false;

    size_t at = 0;
    for (const char* c = canonical; *c; c++) out[at++] = *c;
    out[at++] = ':';
    out[at++] = '/';
    out[at++] = '/';
    for (size_t i = 0; i < body_length; i++) {
        out[at++] = i < host_length ? (char)tolower((unsigned char)after[i]) : after[i];
    }
    out[at] = '\0';
    return true;
}

// Adds an entry unless the URL is bad, a duplicate, or there is no room.
static void add_entry(const char* raw_url, const char* title, PodcastImportEntry* out, int max,
                      PodcastImportResult* result) {
    char url[PODCAST_MAX_URL];
    if (!podcast_import_normalize_url(raw_url, url, sizeof(url))) {
        result->invalid++;
        return;
    }
    for (int i = 0; i < result->count; i++) {
        if (strcmp(out[i].url, url) == 0) {
            result->duplicates++;
            return;
        }
    }
    if (result->count >= max) {
        result->truncated = true;
        return;
    }
    PodcastImportEntry* entry = &out[result->count++];
    memset(entry, 0, sizeof(*entry));
    memcpy(entry->url, url, strlen(url) + 1);
    if (title) {
        strncpy(entry->title, title, sizeof(entry->title) - 1);
        trim_in_place(entry->title);
    }
}

// One feed URL per line, then an optional title. # starts a comment line.
static void parse_text(const char* data, size_t length, PodcastImportEntry* out, int max,
                       PodcastImportResult* result) {
    size_t at = 0;
    // A text editor of Windows writes a byte order mark
    if (length >= 3 && (unsigned char)data[0] == 0xEF && (unsigned char)data[1] == 0xBB &&
        (unsigned char)data[2] == 0xBF) {
        at = 3;
    }
    while (at < length) {
        size_t end = at;
        while (end < length && data[end] != '\n') end++;

        char line[PODCAST_MAX_URL + PODCAST_MAX_TITLE + 2];
        size_t line_length = end - at;
        if (line_length >= sizeof(line)) line_length = sizeof(line) - 1;
        memcpy(line, data + at, line_length);
        line[line_length] = '\0';
        at = end + 1;

        trim_in_place(line);
        if (line[0] == '\0' || line[0] == '#') continue;

        char* title = line;
        while (*title && !is_space(*title)) title++;
        if (*title) *title++ = '\0';
        add_entry(line, title, out, max, result);
    }
}

// An internet shortcut of Windows: [InternetShortcut] then URL=...
static void parse_url_file(const char* data, size_t length, PodcastImportEntry* out, int max,
                           PodcastImportResult* result) {
    size_t at = 0;
    bool found = false;
    while (at < length) {
        size_t end = at;
        while (end < length && data[end] != '\n') end++;

        char line[PODCAST_MAX_URL + 16];
        size_t line_length = end - at;
        if (line_length >= sizeof(line)) line_length = sizeof(line) - 1;
        memcpy(line, data + at, line_length);
        line[line_length] = '\0';
        at = end + 1;

        trim_in_place(line);
        if (has_prefix(line, "URL=")) {
            add_entry(line + 4, NULL, out, max, result);
            found = true;
            break;
        }
    }
    if (!found) result->invalid++;
}

// What an <outline> element has said so far
typedef struct {
    bool active;
    char url[PODCAST_MAX_URL];
    char text[PODCAST_MAX_TITLE];
    char title[PODCAST_MAX_TITLE];
} Outline;

static void append_bounded(char* dest, size_t dest_size, const char* src) {
    size_t length = strlen(dest);
    size_t room = dest_size - length - 1;
    size_t take = strlen(src);
    if (take > room) take = room;
    memcpy(dest + length, src, take);
    dest[length + take] = '\0';
}

static void flush_outline(Outline* outline, PodcastImportEntry* out, int max, PodcastImportResult* result) {
    if (outline->active && outline->url[0]) {
        add_entry(outline->url, outline->title[0] ? outline->title : outline->text, out, max, result);
    }
    outline->active = false;
}

// Each <outline xmlUrl="..."> is a feed, at any depth: a podcast app groups them in outlines
// of their own. An outline with no xmlUrl is a group, and holds no feed itself.
static void parse_opml(const char* data, size_t length, PodcastImportEntry* out, int max,
                       PodcastImportResult* result) {
    yxml_t* parser = (yxml_t*)malloc(sizeof(yxml_t));
    char* stack = (char*)malloc(4096);
    Outline* outline = (Outline*)calloc(1, sizeof(Outline));
    if (!parser || !stack || !outline) {
        free(parser);
        free(stack);
        free(outline);
        return;
    }
    yxml_init(parser, stack, 4096);

    char attr[32] = "";
    char value[PODCAST_MAX_URL] = "";
    bool is_outline = false;

    for (size_t i = 0; i < length; i++) {
        yxml_ret_t r = yxml_parse(parser, data[i]);
        if (r < 0) break;  // a malformed file gives what it gave before the fault
        switch (r) {
            case YXML_ELEMSTART:
                flush_outline(outline, out, max, result);
                is_outline = strcmp(parser->elem, "outline") == 0;
                if (is_outline) {
                    memset(outline, 0, sizeof(*outline));
                    outline->active = true;
                }
                break;
            case YXML_ELEMEND:
                flush_outline(outline, out, max, result);
                is_outline = false;
                break;
            case YXML_ATTRSTART:
                strncpy(attr, parser->attr, sizeof(attr) - 1);
                attr[sizeof(attr) - 1] = '\0';
                value[0] = '\0';
                break;
            case YXML_ATTRVAL:
                append_bounded(value, sizeof(value), parser->data);
                break;
            case YXML_ATTREND:
                if (!is_outline || !outline->active) break;
                if (strcasecmp(attr, "xmlUrl") == 0) {
                    strncpy(outline->url, value, sizeof(outline->url) - 1);
                } else if (strcasecmp(attr, "text") == 0) {
                    strncpy(outline->text, value, sizeof(outline->text) - 1);
                } else if (strcasecmp(attr, "title") == 0) {
                    strncpy(outline->title, value, sizeof(outline->title) - 1);
                }
                break;
            default:
                break;
        }
    }
    flush_outline(outline, out, max, result);

    free(parser);
    free(stack);
    free(outline);
}

void podcast_import_parse(PodcastImportFormat format, const char* data, size_t length,
                          PodcastImportEntry* out, int max, PodcastImportResult* result) {
    if (!result) return;
    memset(result, 0, sizeof(*result));
    if (!data || !out || max <= 0) return;

    switch (format) {
        case PODCAST_IMPORT_FORMAT_TEXT: parse_text(data, length, out, max, result); break;
        case PODCAST_IMPORT_FORMAT_OPML: parse_opml(data, length, out, max, result); break;
        case PODCAST_IMPORT_FORMAT_URL:  parse_url_file(data, length, out, max, result); break;
        case PODCAST_IMPORT_FORMAT_NONE: break;
    }
}

#ifndef __PODCAST_IMPORT_PARSE_H__
#define __PODCAST_IMPORT_PARSE_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "podcast.h"

// Reads the files that the user drops on the SD card to subscribe to podcasts: a list of feed
// URLs (.txt), an OPML export of a podcast app (.opml), or an internet shortcut (.url). Pure
// text in, entries out: the network and the files are in podcast_import.c.

// The most entries that one file gives. A larger OPML is cut, and the cut is reported.
#define PODCAST_IMPORT_MAX_ENTRIES 200

// The largest file that is read, in bytes. A larger one is refused whole.
#define PODCAST_IMPORT_MAX_FILE_SIZE (512 * 1024)

typedef enum {
    PODCAST_IMPORT_FORMAT_NONE = 0,  // not a file to import
    PODCAST_IMPORT_FORMAT_TEXT,      // .txt: one feed URL per line
    PODCAST_IMPORT_FORMAT_OPML,      // .opml
    PODCAST_IMPORT_FORMAT_URL        // .url: internet shortcut
} PodcastImportFormat;

typedef struct {
    char url[PODCAST_MAX_URL];
    char title[PODCAST_MAX_TITLE];  // what the file calls the show, or empty
} PodcastImportEntry;

typedef struct {
    int count;       // entries written
    int invalid;     // lines or outlines that name no usable URL
    int duplicates;  // the same URL as an earlier entry of the file
    bool truncated;  // more entries than the caller had room for
} PodcastImportResult;

// The format of a file by its name, case-insensitive. A hidden file, and a name with another
// extension, are NONE.
PodcastImportFormat podcast_import_format_of(const char* filename);

// True for the report that an import leaves beside the files that failed: "<name>.failed.txt".
bool podcast_import_is_failure_report(const char* filename);

// Cleans a URL a user wrote: trims it, maps feed:, itpc: and pcast: to https:, drops the
// fragment, lower-cases the scheme and host. Only http and https URLs with a host come out.
// out holds PODCAST_MAX_URL bytes at least. Returns false for anything else.
bool podcast_import_normalize_url(const char* text, char* out, size_t out_size);

// Parses the content of a file of the given format. Returns the entries in `out` (room for
// `max`), and counts in `result`.
void podcast_import_parse(PodcastImportFormat format, const char* data, size_t length,
                          PodcastImportEntry* out, int max, PodcastImportResult* result);

#ifdef __cplusplus
}
#endif

#endif

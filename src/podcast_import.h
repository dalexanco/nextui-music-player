#ifndef __PODCAST_IMPORT_H__
#define __PODCAST_IMPORT_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "podcast.h"
#include "podcast_import_parse.h"

// Subscribes to the podcasts that the user dropped on the SD card, in
// <SD>/Podcasts/Subscriptions/: .txt (one feed URL per line), .opml (the export of a podcast
// app) and .url (an internet shortcut). The work runs on a thread of its own, and the page
// reads the progress.
//
// A file that was read is moved to imported/. Feeds that failed go to "<name>.failed.txt", a
// file in the same format with the reason on a comment line above each URL, which a later
// import reads again. A file that holds no feed is renamed "<name>.invalid".

#define PODCAST_IMPORT_FOLDER "Subscriptions"
#define PODCAST_IMPORT_DONE_FOLDER "imported"

// The most results that the page lists; further feeds are still imported, and only counted
#define PODCAST_IMPORT_MAX_RESULTS 256

typedef enum {
    PODCAST_IMPORT_STATE_IDLE = 0,  // nothing ran yet in this session
    PODCAST_IMPORT_STATE_RUNNING,
    PODCAST_IMPORT_STATE_DONE       // the last import ended, its results stay until the next one
} PodcastImportState;

typedef enum {
    PODCAST_IMPORT_ITEM_PENDING = 0,
    PODCAST_IMPORT_ITEM_ADDED,
    PODCAST_IMPORT_ITEM_DUPLICATE,  // already subscribed
    PODCAST_IMPORT_ITEM_FAILED
} PodcastImportItemStatus;

typedef struct {
    char name[PODCAST_MAX_TITLE];    // the title that the feed gave, else the file's, else the URL
    char url[PODCAST_MAX_URL];
    char reason[96];                 // for a failure
    PodcastImportItemStatus status;
} PodcastImportItem;

typedef struct {
    PodcastImportState state;
    int files;          // files of this import
    int total;          // feeds found in them
    int done;           // feeds handled
    int added;
    int duplicates;
    int failed;
    int invalid_files;  // files that hold no feed, or are too large to read
    bool truncated;     // a file had more feeds than the cap
    char current[PODCAST_MAX_TITLE];  // the feed under way
} PodcastImportStatus;

// The folder that the user drops files in, as a path on the card.
const char* PodcastImport_getFolder(void);

// Creates the folder, and a README.md in it that says what goes there, when they are missing.
void PodcastImport_prepareFolder(void);

// How many files wait in the folder. A report of failures counts only when include_reports is
// true: the import that runs by itself leaves them alone, the one that the user asks for takes them.
int PodcastImport_countFiles(bool include_reports);

// Starts the import on its own thread. Returns 0 when it started, -1 when there is nothing to
// import, one runs already, or the thread failed. The podcasts must be initialized.
int PodcastImport_start(bool include_reports);

bool PodcastImport_isRunning(void);

// A copy of the progress, safe to call from any thread.
PodcastImportStatus PodcastImport_getStatus(void);

// The result at `index`, the order of the files then of the feeds in them. False past the end.
bool PodcastImport_getItem(int index, PodcastImportItem* out);
int PodcastImport_getItemCount(void);

// True once after each import ends, for the page to say the result and to read the list again.
bool PodcastImport_takeFinished(void);

#ifdef __cplusplus
}
#endif

#endif

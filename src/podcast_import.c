#include "podcast_import.h"

#include <dirent.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/stat.h>
#include <unistd.h>

#include "defines.h"
#include "api.h"
#include "debug.h"
#include "file_utils.h"

#define PODCAST_IMPORT_MAX_FILES 16
#define PODCAST_IMPORT_PATH SDCARD_PATH "/Podcasts/" PODCAST_IMPORT_FOLDER

static pthread_mutex_t import_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_t import_thread;
static volatile bool import_running = false;
static bool import_finished_flag = false;
static bool import_with_reports = false;

static PodcastImportStatus import_status;
static PodcastImportItem import_items[PODCAST_IMPORT_MAX_RESULTS];
static int import_item_count = 0;

const char* PodcastImport_getFolder(void) {
    return PODCAST_IMPORT_PATH;
}

void PodcastImport_prepareFolder(void) {
    struct stat st;
    if (stat(PODCAST_IMPORT_PATH, &st) == 0) return;
    if (!mkdir_p(PODCAST_IMPORT_PATH)) return;

    FILE* fp = fopen(PODCAST_IMPORT_PATH "/README.md", "w");
    if (!fp) return;
    fputs("# Podcasts to subscribe to\n"
          "\n"
          "Drop a file here, open Podcasts on the device, and it subscribes.\n"
          "\n"
          "- `.txt`: one feed address per line. `#` starts a comment. A title after the address is optional.\n"
          "- `.opml`: the export of a podcast app, many shows at once.\n"
          "- `.url`: an internet shortcut to one feed.\n"
          "\n"
          "A file that was read moves to `imported/`. Feeds that failed are listed in\n"
          "`<name>.failed.txt`, with the reason: fix them, and import again from\n"
          "Podcasts > Y > Import from SD card.\n"
          "\n"
          "Example, `bestioles.txt`:\n"
          "\n"
          "    # France Inter\n"
          "    https://radiofrance-podcast.net/podcast09/podcast_a80ecbd5-df3d-4c9d-bee7-4e3d9efc1974.xml Bestioles\n",
          fp);
    fclose(fp);
}

static int compare_names(const void* a, const void* b) {
    return strcasecmp((const char*)a, (const char*)b);
}

// Collects the names of the files to import, sorted. Returns how many.
static int list_files(bool include_reports, char names[][256], int max) {
    DIR* dir = opendir(PODCAST_IMPORT_PATH);
    if (!dir) return 0;

    int count = 0;
    struct dirent* entry;
    while ((entry = readdir(dir)) != NULL && count < max) {
        if (podcast_import_format_of(entry->d_name) == PODCAST_IMPORT_FORMAT_NONE) continue;
        if (!include_reports && podcast_import_is_failure_report(entry->d_name)) continue;

        char path[512];
        snprintf(path, sizeof(path), "%s/%s", PODCAST_IMPORT_PATH, entry->d_name);
        struct stat st;
        if (stat(path, &st) != 0 || !S_ISREG(st.st_mode)) continue;

        snprintf(names[count++], 256, "%s", entry->d_name);
    }
    closedir(dir);
    qsort(names, (size_t)count, 256, compare_names);
    return count;
}

int PodcastImport_countFiles(bool include_reports) {
    static char names[PODCAST_IMPORT_MAX_FILES][256];
    static pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_mutex_lock(&count_mutex);
    int count = list_files(include_reports, names, PODCAST_IMPORT_MAX_FILES);
    pthread_mutex_unlock(&count_mutex);
    return count;
}

// ---------------------------------------------------------------------------
// The results the page reads
// ---------------------------------------------------------------------------

static void set_current(const char* text) {
    pthread_mutex_lock(&import_mutex);
    snprintf(import_status.current, sizeof(import_status.current), "%s", text ? text : "");
    pthread_mutex_unlock(&import_mutex);
}

// Records the outcome of one feed. The counters always move, the list stops at its cap.
static void record(const char* name, const char* url, PodcastImportItemStatus status, const char* reason) {
    pthread_mutex_lock(&import_mutex);
    if (import_item_count < PODCAST_IMPORT_MAX_RESULTS) {
        PodcastImportItem* item = &import_items[import_item_count++];
        memset(item, 0, sizeof(*item));
        snprintf(item->name, sizeof(item->name), "%s", name && name[0] ? name : url);
        snprintf(item->url, sizeof(item->url), "%s", url);
        snprintf(item->reason, sizeof(item->reason), "%s", reason ? reason : "");
        item->status = status;
    }
    import_status.done++;
    if (status == PODCAST_IMPORT_ITEM_ADDED) import_status.added++;
    if (status == PODCAST_IMPORT_ITEM_DUPLICATE) import_status.duplicates++;
    if (status == PODCAST_IMPORT_ITEM_FAILED) import_status.failed++;
    pthread_mutex_unlock(&import_mutex);
}

// A file that holds no feed, or cannot be read: one failed line for the page, no feed counted
static void record_bad_file(const char* filename, const char* reason) {
    pthread_mutex_lock(&import_mutex);
    if (import_item_count < PODCAST_IMPORT_MAX_RESULTS) {
        PodcastImportItem* item = &import_items[import_item_count++];
        memset(item, 0, sizeof(*item));
        snprintf(item->name, sizeof(item->name), "%s", filename);
        snprintf(item->reason, sizeof(item->reason), "%s", reason);
        item->status = PODCAST_IMPORT_ITEM_FAILED;
    }
    import_status.invalid_files++;
    pthread_mutex_unlock(&import_mutex);
}

// ---------------------------------------------------------------------------
// Files
// ---------------------------------------------------------------------------

// Reads a whole file, NUL-terminated. NULL when it is missing or larger than the cap.
static char* read_file(const char* path, size_t* length) {
    struct stat st;
    if (stat(path, &st) != 0 || st.st_size > PODCAST_IMPORT_MAX_FILE_SIZE) return NULL;
    FILE* fp = fopen(path, "rb");
    if (!fp) return NULL;
    char* data = (char*)malloc((size_t)st.st_size + 1);
    if (!data) {
        fclose(fp);
        return NULL;
    }
    *length = fread(data, 1, (size_t)st.st_size, fp);
    data[*length] = '\0';
    fclose(fp);
    return data;
}

// "name.txt" -> "name", and "name.failed.txt" -> "name"
static void base_name(const char* filename, char* out, size_t out_size) {
    snprintf(out, out_size, "%s", filename);
    char* dot = strrchr(out, '.');
    if (dot) *dot = '\0';
    size_t length = strlen(out);
    if (length > 7 && strcasecmp(out + length - 7, ".failed") == 0) out[length - 7] = '\0';
}

// Moves the file that was read to imported/, under a free name. Returns false when it stays.
static bool move_to_done(const char* filename) {
    char folder[512];
    snprintf(folder, sizeof(folder), "%s/%s", PODCAST_IMPORT_PATH, PODCAST_IMPORT_DONE_FOLDER);
    if (!mkdir_p(folder)) return false;

    char from[512];
    snprintf(from, sizeof(from), "%s/%s", PODCAST_IMPORT_PATH, filename);

    char to[768];
    snprintf(to, sizeof(to), "%s/%s", folder, filename);
    for (int n = 2; access(to, F_OK) == 0 && n < 1000; n++) {
        char base[256];
        snprintf(base, sizeof(base), "%s", filename);
        const char* ext = strrchr(filename, '.');
        if (ext) base[ext - filename] = '\0';
        snprintf(to, sizeof(to), "%s/%s-%d%s", folder, base, n, ext ? ext : "");
    }
    return rename(from, to) == 0;
}

// Leaves the feeds that failed in "<name>.failed.txt", in the format that an import reads
static void write_failure_report(const char* filename, const PodcastImportEntry* entries,
                                 const char reasons[][96], const bool* failed, int count) {
    char base[256];
    base_name(filename, base, sizeof(base));

    char path[512];
    snprintf(path, sizeof(path), "%s/%s.failed.txt", PODCAST_IMPORT_PATH, base);
    FILE* fp = fopen(path, "w");
    if (!fp) return;
    fprintf(fp, "# Feeds of %s that could not be imported.\n"
                "# Fix the address or remove the line, then use Import from SD card again.\n",
            filename);
    for (int i = 0; i < count; i++) {
        if (!failed[i]) continue;
        fprintf(fp, "\n# %s\n%s%s%s\n", reasons[i], entries[i].url, entries[i].title[0] ? " " : "",
                entries[i].title);
    }
    fclose(fp);
}

// ---------------------------------------------------------------------------
// The thread
// ---------------------------------------------------------------------------

typedef struct {
    char name[256];
    PodcastImportEntry* entries;
    int count;
} ImportFile;

static void import_one(const PodcastImportEntry* entry, char* reason_out, size_t reason_size,
                       PodcastImportItemStatus* status_out, char* name_out, size_t name_size) {
    snprintf(name_out, name_size, "%s", entry->title[0] ? entry->title : entry->url);

    if (Podcast_isSubscribed(entry->url)) {
        *status_out = PODCAST_IMPORT_ITEM_DUPLICATE;
        reason_out[0] = '\0';
        return;
    }
    if (Podcast_subscribe(entry->url) == 0) {
        int index = Podcast_findFeedIndex(entry->url);
        PodcastFeed* feed = index >= 0 ? Podcast_getSubscription(index) : NULL;
        if (feed && feed->title[0]) snprintf(name_out, name_size, "%s", feed->title);
        *status_out = PODCAST_IMPORT_ITEM_ADDED;
        reason_out[0] = '\0';
        return;
    }
    *status_out = PODCAST_IMPORT_ITEM_FAILED;
    snprintf(reason_out, reason_size, "%s", Podcast_getError()[0] ? Podcast_getError() : "Could not subscribe");
}

static void* import_thread_func(void* arg) {
    (void)arg;
    static char names[PODCAST_IMPORT_MAX_FILES][256];
    int file_count = list_files(import_with_reports, names, PODCAST_IMPORT_MAX_FILES);

    ImportFile files[PODCAST_IMPORT_MAX_FILES];
    int parsed = 0;

    // Read every file first, thus the page knows the total
    for (int f = 0; f < file_count; f++) {
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", PODCAST_IMPORT_PATH, names[f]);
        size_t length = 0;
        char* data = read_file(path, &length);
        if (!data) {
            LOG_error("[PodcastImport] Cannot read %s\n", path);
            record_bad_file(names[f], "Cannot read the file, or over 512 KB");
            char invalid[600];
            snprintf(invalid, sizeof(invalid), "%s.invalid", path);
            rename(path, invalid);
            continue;
        }

        PodcastImportEntry* entries = (PodcastImportEntry*)malloc(sizeof(PodcastImportEntry) * PODCAST_IMPORT_MAX_ENTRIES);
        if (!entries) {
            free(data);
            continue;
        }
        PodcastImportResult parse_result;
        podcast_import_parse(podcast_import_format_of(names[f]), data, length, entries,
                             PODCAST_IMPORT_MAX_ENTRIES, &parse_result);
        free(data);

        if (parse_result.count == 0) {
            record_bad_file(names[f], "No podcast address found");
            char invalid[600];
            snprintf(invalid, sizeof(invalid), "%s.invalid", path);
            rename(path, invalid);
            free(entries);
            continue;
        }

        snprintf(files[parsed].name, sizeof(files[parsed].name), "%s", names[f]);
        files[parsed].entries = entries;
        files[parsed].count = parse_result.count;
        parsed++;

        pthread_mutex_lock(&import_mutex);
        import_status.files++;
        import_status.total += parse_result.count;
        if (parse_result.truncated) import_status.truncated = true;
        pthread_mutex_unlock(&import_mutex);
    }

    for (int f = 0; f < parsed; f++) {
        ImportFile* file = &files[f];
        char (*reasons)[96] = (char (*)[96])calloc((size_t)file->count, 96);
        bool* failed = (bool*)calloc((size_t)file->count, sizeof(bool));
        bool any_failed = false;

        for (int i = 0; i < file->count && reasons && failed; i++) {
            const PodcastImportEntry* entry = &file->entries[i];
            set_current(entry->title[0] ? entry->title : entry->url);

            char reason[96];
            char name[PODCAST_MAX_TITLE];
            PodcastImportItemStatus status;
            import_one(entry, reason, sizeof(reason), &status, name, sizeof(name));
            record(name, entry->url, status, reason);
            if (status == PODCAST_IMPORT_ITEM_FAILED) {
                failed[i] = true;
                any_failed = true;
                snprintf(reasons[i], 96, "%s", reason);
            }
        }

        // The file is read: out of the way first, so the report may take the name of a former one
        move_to_done(file->name);
        if (any_failed) write_failure_report(file->name, file->entries, (const char (*)[96])reasons, failed, file->count);

        free(reasons);
        free(failed);
        free(file->entries);
    }

    set_current("");
    Podcast_saveSubscriptions();

    pthread_mutex_lock(&import_mutex);
    import_status.state = PODCAST_IMPORT_STATE_DONE;
    import_finished_flag = true;
    pthread_mutex_unlock(&import_mutex);
    import_running = false;

    // The feeds that were there before may be stale: the refresh skips the ones just added
    Podcast_startRefreshAll();
    return NULL;
}

int PodcastImport_start(bool include_reports) {
    if (import_running) return -1;
    if (PodcastImport_countFiles(include_reports) == 0) return -1;

    pthread_mutex_lock(&import_mutex);
    memset(&import_status, 0, sizeof(import_status));
    import_status.state = PODCAST_IMPORT_STATE_RUNNING;
    import_item_count = 0;
    import_finished_flag = false;
    pthread_mutex_unlock(&import_mutex);

    import_with_reports = include_reports;
    import_running = true;
    if (pthread_create(&import_thread, NULL, import_thread_func, NULL) != 0) {
        import_running = false;
        pthread_mutex_lock(&import_mutex);
        import_status.state = PODCAST_IMPORT_STATE_IDLE;
        pthread_mutex_unlock(&import_mutex);
        return -1;
    }
    pthread_detach(import_thread);
    return 0;
}

bool PodcastImport_isRunning(void) {
    return import_running;
}

PodcastImportStatus PodcastImport_getStatus(void) {
    pthread_mutex_lock(&import_mutex);
    PodcastImportStatus copy = import_status;
    pthread_mutex_unlock(&import_mutex);
    return copy;
}

int PodcastImport_getItemCount(void) {
    pthread_mutex_lock(&import_mutex);
    int count = import_item_count;
    pthread_mutex_unlock(&import_mutex);
    return count;
}

bool PodcastImport_getItem(int index, PodcastImportItem* out) {
    bool found = false;
    pthread_mutex_lock(&import_mutex);
    if (index >= 0 && index < import_item_count && out) {
        *out = import_items[index];
        found = true;
    }
    pthread_mutex_unlock(&import_mutex);
    return found;
}

bool PodcastImport_takeFinished(void) {
    pthread_mutex_lock(&import_mutex);
    bool finished = import_finished_flag;
    import_finished_flag = false;
    pthread_mutex_unlock(&import_mutex);
    return finished;
}

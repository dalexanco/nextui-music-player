#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"
#include "podcast_import_parse.h"

#ifndef TEST_FIXTURE_DIR
#define TEST_FIXTURE_DIR "fixtures"
#endif

#define BESTIOLES "https://radiofrance-podcast.net/podcast09/podcast_a80ecbd5-df3d-4c9d-bee7-4e3d9efc1974.xml"
#define ODYSSEES "https://radiofrance-podcast.net/podcast09/podcast_c361798b-d6e3-4282-ba0a-ebb051b9e424.xml"

static PodcastImportEntry entries[PODCAST_IMPORT_MAX_ENTRIES];
static PodcastImportResult result;

static void parse(PodcastImportFormat format, const char* text) {
    memset(entries, 0, sizeof(entries));
    podcast_import_parse(format, text, strlen(text), entries, PODCAST_IMPORT_MAX_ENTRIES, &result);
}

TEST(format_of_a_file_name) {
    CHECK_EQ_INT(podcast_import_format_of("feeds.txt"), PODCAST_IMPORT_FORMAT_TEXT);
    CHECK_EQ_INT(podcast_import_format_of("Feeds.TXT"), PODCAST_IMPORT_FORMAT_TEXT);
    CHECK_EQ_INT(podcast_import_format_of("export.opml"), PODCAST_IMPORT_FORMAT_OPML);
    CHECK_EQ_INT(podcast_import_format_of("Les bestioles.url"), PODCAST_IMPORT_FORMAT_URL);
    CHECK_EQ_INT(podcast_import_format_of("feed.xml"), PODCAST_IMPORT_FORMAT_NONE);
    CHECK_EQ_INT(podcast_import_format_of("README"), PODCAST_IMPORT_FORMAT_NONE);
    CHECK_EQ_INT(podcast_import_format_of(".hidden.txt"), PODCAST_IMPORT_FORMAT_NONE);
    CHECK_EQ_INT(podcast_import_format_of("._feeds.txt"), PODCAST_IMPORT_FORMAT_NONE);
    CHECK_EQ_INT(podcast_import_format_of(NULL), PODCAST_IMPORT_FORMAT_NONE);
}

TEST(failure_report_name) {
    CHECK(podcast_import_is_failure_report("feeds.failed.txt"));
    CHECK(podcast_import_is_failure_report("FEEDS.FAILED.TXT"));
    CHECK(!podcast_import_is_failure_report("feeds.txt"));
    CHECK(!podcast_import_is_failure_report(".failed.txt"));
    CHECK(!podcast_import_is_failure_report(NULL));
}

TEST(normalize_url) {
    char out[PODCAST_MAX_URL];
    CHECK(podcast_import_normalize_url("  https://Example.COM/Feed.xml?X=1#frag \r\n", out, sizeof(out)));
    CHECK(strcmp(out, "https://example.com/Feed.xml?X=1") == 0);
    CHECK(podcast_import_normalize_url("feed://example.com/rss", out, sizeof(out)));
    CHECK(strcmp(out, "https://example.com/rss") == 0);
    CHECK(podcast_import_normalize_url("feed:https://example.com/rss", out, sizeof(out)));
    CHECK(strcmp(out, "https://example.com/rss") == 0);
    CHECK(podcast_import_normalize_url("itpc://example.com/rss", out, sizeof(out)));
    CHECK(strcmp(out, "https://example.com/rss") == 0);
    CHECK(podcast_import_normalize_url("pcast://example.com/rss", out, sizeof(out)));
    CHECK(strcmp(out, "https://example.com/rss") == 0);
    CHECK(podcast_import_normalize_url("http://example.com:8080", out, sizeof(out)));
    CHECK(strcmp(out, "http://example.com:8080") == 0);
    CHECK(!podcast_import_normalize_url("ftp://example.com/rss", out, sizeof(out)));
    CHECK(!podcast_import_normalize_url("example.com/rss", out, sizeof(out)));
    CHECK(!podcast_import_normalize_url("https://", out, sizeof(out)));
    CHECK(!podcast_import_normalize_url("https:///path", out, sizeof(out)));
    CHECK(!podcast_import_normalize_url("https://exa mple.com/rss", out, sizeof(out)));
    CHECK(!podcast_import_normalize_url("https://example.com/\"quoted\"", out, sizeof(out)));
    CHECK(!podcast_import_normalize_url("", out, sizeof(out)));
    CHECK(!podcast_import_normalize_url(NULL, out, sizeof(out)));
    char tiny[8];
    CHECK(!podcast_import_normalize_url("https://example.com/rss", tiny, sizeof(tiny)));
}

TEST(normalize_url_too_long) {
    char url[PODCAST_MAX_URL + 64] = "https://example.com/";
    memset(url + strlen(url), 'a', PODCAST_MAX_URL);
    url[strlen("https://example.com/") + PODCAST_MAX_URL] = '\0';
    char out[PODCAST_MAX_URL];
    CHECK(!podcast_import_normalize_url(url, out, sizeof(out)));
}

TEST(text_file_with_comments_and_titles) {
    parse(PODCAST_IMPORT_FORMAT_TEXT,
          "# My podcasts\r\n"
          "\r\n"
          BESTIOLES "   Les bestioles (France Inter)\r\n"
          "  " ODYSSEES "\n"
          "   # indented comment\n"
          "feed://example.com/rss\n");
    CHECK_EQ_INT(result.count, 3);
    CHECK_EQ_INT(result.invalid, 0);
    CHECK(strcmp(entries[0].url, BESTIOLES) == 0);
    CHECK(strcmp(entries[0].title, "Les bestioles (France Inter)") == 0);
    CHECK(strcmp(entries[1].url, ODYSSEES) == 0);
    CHECK(entries[1].title[0] == '\0');
    CHECK(strcmp(entries[2].url, "https://example.com/rss") == 0);
}

TEST(text_file_with_bom_and_no_final_newline) {
    parse(PODCAST_IMPORT_FORMAT_TEXT, "\xEF\xBB\xBF" BESTIOLES);
    CHECK_EQ_INT(result.count, 1);
    CHECK(strcmp(entries[0].url, BESTIOLES) == 0);
}

TEST(text_file_counts_duplicates_and_invalid_lines) {
    parse(PODCAST_IMPORT_FORMAT_TEXT,
          BESTIOLES "\n"
          "HTTPS://RADIOFRANCE-PODCAST.NET/podcast09/podcast_a80ecbd5-df3d-4c9d-bee7-4e3d9efc1974.xml\n"
          "Les bestioles\n"
          "mailto:someone@example.com\n"
          ODYSSEES "\n");
    CHECK_EQ_INT(result.count, 2);
    CHECK_EQ_INT(result.duplicates, 1);
    CHECK_EQ_INT(result.invalid, 2);
}

TEST(text_file_empty) {
    parse(PODCAST_IMPORT_FORMAT_TEXT, "");
    CHECK_EQ_INT(result.count, 0);
    parse(PODCAST_IMPORT_FORMAT_TEXT, "\n\n# only comments\n");
    CHECK_EQ_INT(result.count, 0);
    CHECK_EQ_INT(result.invalid, 0);
}

TEST(text_file_is_cut_at_the_room) {
    char* big = (char*)malloc(300 * 64);
    big[0] = '\0';
    for (int i = 0; i < 300; i++) {
        char line[64];
        snprintf(line, sizeof(line), "https://example.com/feed%d.xml\n", i);
        strcat(big, line);
    }
    memset(entries, 0, sizeof(entries));
    podcast_import_parse(PODCAST_IMPORT_FORMAT_TEXT, big, strlen(big), entries, PODCAST_IMPORT_MAX_ENTRIES, &result);
    free(big);
    CHECK_EQ_INT(result.count, PODCAST_IMPORT_MAX_ENTRIES);
    CHECK(result.truncated);
}

TEST(url_shortcut) {
    parse(PODCAST_IMPORT_FORMAT_URL,
          "[InternetShortcut]\r\nURL=" BESTIOLES "\r\nIconIndex=0\r\n");
    CHECK_EQ_INT(result.count, 1);
    CHECK(strcmp(entries[0].url, BESTIOLES) == 0);

    parse(PODCAST_IMPORT_FORMAT_URL, "[InternetShortcut]\nIconIndex=0\n");
    CHECK_EQ_INT(result.count, 0);
    CHECK_EQ_INT(result.invalid, 1);
}

static char* load_fixture(const char* name, size_t* length) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", TEST_FIXTURE_DIR, name);
    FILE* fp = fopen(path, "rb");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* data = (char*)malloc((size_t)size + 1);
    if (data) {
        *length = fread(data, 1, (size_t)size, fp);
        data[*length] = '\0';
    }
    fclose(fp);
    return data;
}

TEST(opml_with_nested_outlines_and_entities) {
    size_t length = 0;
    char* data = load_fixture("podcast_import_nested.opml", &length);
    CHECK(data != NULL);
    if (!data) return;
    memset(entries, 0, sizeof(entries));
    podcast_import_parse(PODCAST_IMPORT_FORMAT_OPML, data, length, entries, PODCAST_IMPORT_MAX_ENTRIES, &result);
    free(data);

    // Bestioles and Les Odyssées (two levels down), the show with the entities; the repeat of
    // Bestioles counts as a duplicate, the web page as nothing, and the ftp feed as invalid
    CHECK_EQ_INT(result.count, 3);
    CHECK_EQ_INT(result.duplicates, 1);
    CHECK_EQ_INT(result.invalid, 1);
    CHECK(strcmp(entries[0].url, BESTIOLES) == 0);
    CHECK(strcmp(entries[0].title, "Bestioles") == 0);
    CHECK(strcmp(entries[1].url, ODYSSEES) == 0);
    CHECK(strcmp(entries[1].title, "Les Odyssées") == 0);
    CHECK(strcmp(entries[2].url, "https://example.com/feed?a=1&b=2") == 0);
    CHECK(strcmp(entries[2].title, "Tom & Jerry's \"Show\"") == 0);
}

TEST(opml_malformed_keeps_what_came_before) {
    parse(PODCAST_IMPORT_FORMAT_OPML,
          "<opml><body><outline text=\"A\" xmlUrl=\"https://example.com/a\"/>"
          "<outline text=\"B\" xmlUrl=\"https://example.com/b\"/><<<broken");
    CHECK_EQ_INT(result.count, 2);
    parse(PODCAST_IMPORT_FORMAT_OPML, "not xml at all");
    CHECK_EQ_INT(result.count, 0);
    parse(PODCAST_IMPORT_FORMAT_OPML, "");
    CHECK_EQ_INT(result.count, 0);
}

TEST(opml_attribute_case_and_title_fallback) {
    parse(PODCAST_IMPORT_FORMAT_OPML,
          "<opml><body><outline TEXT=\"Short\" title=\"Long title\" XMLURL=\"https://example.com/a\"/>"
          "<outline xmlUrl=\"https://example.com/b\"/></body></opml>");
    CHECK_EQ_INT(result.count, 2);
    CHECK(strcmp(entries[0].title, "Long title") == 0);
    CHECK(entries[1].title[0] == '\0');
}

int main(void) {
    RUN(format_of_a_file_name);
    RUN(failure_report_name);
    RUN(normalize_url);
    RUN(normalize_url_too_long);
    RUN(text_file_with_comments_and_titles);
    RUN(text_file_with_bom_and_no_final_newline);
    RUN(text_file_counts_duplicates_and_invalid_lines);
    RUN(text_file_empty);
    RUN(text_file_is_cut_at_the_room);
    RUN(url_shortcut);
    RUN(opml_with_nested_outlines_and_entities);
    RUN(opml_malformed_keeps_what_came_before);
    RUN(opml_attribute_case_and_title_fallback);
    return test_summary();
}

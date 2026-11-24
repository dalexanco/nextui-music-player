#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "test.h"
#include "podcast.h"

#ifndef TEST_FIXTURE_DIR
#define TEST_FIXTURE_DIR "fixtures"
#endif

// Loads a fixture whole, NUL-terminated. The caller frees it.
static char* load_fixture(const char* name, int* len) {
    char path[512];
    snprintf(path, sizeof(path), "%s/%s", TEST_FIXTURE_DIR, name);
    FILE* fp = fopen(path, "rb");
    if (!fp) {
        printf("    cannot open %s\n", path);
        return NULL;
    }
    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* data = (char*)malloc((size_t)size + 1);
    if (data && fread(data, 1, (size_t)size, fp) != (size_t)size) {
        free(data);
        data = NULL;
    }
    fclose(fp);
    if (data) {
        data[size] = '\0';
        *len = (int)size;
    }
    return data;
}

static PodcastFeed feed;
static PodcastEpisode episodes[16];
static int count;

static int parse_fixture(const char* name) {
    int len = 0;
    char* xml = load_fixture(name, &len);
    if (!xml) return -99;
    memset(&feed, 0, sizeof(feed));
    memset(episodes, 0, sizeof(episodes));
    count = 0;
    int rc = podcast_rss_parse_with_episodes(xml, len, &feed, episodes, 16, &count);
    free(xml);
    return rc;
}

TEST(bestioles_feed_level) {
    CHECK_EQ_INT(parse_fixture("radiofrance_bestioles.xml"), 0);
    CHECK(strcmp(feed.title, "Bestioles") == 0);
    CHECK(strstr(feed.artwork_url, "1400x1400_sc_carre-bestioles") != NULL);
    CHECK(strcmp(feed.author, "France Inter") == 0);
    CHECK_EQ_INT(count, 4);
}

TEST(bestioles_episodes) {
    parse_fixture("radiofrance_bestioles.xml");
    CHECK(strcmp(episodes[0].title, "Le panda roux, le carnivore végétarien !") == 0);
    CHECK(strncmp(episodes[0].url, "https://proxycast.radiofrance.fr/", 33) == 0);
    CHECK(strstr(episodes[0].url, ".m4a") != NULL);
    CHECK(strcmp(episodes[0].guid, "df995d4b-03bf-448f-baa9-95306d3e1519") == 0);
    CHECK(episodes[0].pub_date > 0);
}

// itunes:duration is hh:mm:ss on every Radio France item
TEST(bestioles_duration_hh_mm_ss) {
    parse_fixture("radiofrance_bestioles.xml");
    CHECK_EQ_INT(episodes[0].duration_sec, 11 * 60 + 49);
    CHECK_EQ_INT(episodes[3].duration_sec, 12);
}

// The oldest item points straight at media.radiofrance-podcast.net, as an mp3
TEST(bestioles_direct_mp3_enclosure) {
    parse_fixture("radiofrance_bestioles.xml");
    CHECK(strncmp(episodes[3].url, "https://media.radiofrance-podcast.net/", 38) == 0);
}

TEST(odyssees) {
    CHECK_EQ_INT(parse_fixture("radiofrance_odyssees.xml"), 0);
    CHECK(strcmp(feed.title, "Les Odyssées") == 0);
    CHECK_EQ_INT(count, 3);
    CHECK_EQ_INT(episodes[0].duration_sec, 12 * 60 + 8);
}

TEST(rejects_empty_and_garbage) {
    PodcastFeed f;
    memset(&f, 0, sizeof(f));
    CHECK_EQ_INT(podcast_rss_parse("", 0, &f), -1);
    memset(&f, 0, sizeof(f));
    const char* junk = "<html><body>not a feed</body></html>";
    CHECK_EQ_INT(podcast_rss_parse(junk, (int)strlen(junk), &f), -1);
}

int main(void) {
    RUN(bestioles_feed_level);
    RUN(bestioles_episodes);
    RUN(bestioles_duration_hh_mm_ss);
    RUN(bestioles_direct_mp3_enclosure);
    RUN(odyssees);
    RUN(rejects_empty_and_garbage);
    return test_summary();
}

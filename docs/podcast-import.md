# Adding a podcast from a file on the SD card

The user wants to add a podcast by dropping a file on the SD card, in particular the Radio France
shows (France Inter « Bestioles »). This page records what was measured (Part 1), the format that
was chosen and why (Part 2), and how the import works (Part 3).

## 1. Research

Measured on 2026-11-24 with `curl` from a Mac, and again with the CA bundle of the pak
(`--cacert res/cacert.pem`, the flags of `wget_fetch.c`).

### The feeds

| Show | Feed URL | Items | Size |
| --- | --- | --- | --- |
| Bestioles (France Inter, with the Muséum), 5-7 years | `https://radiofrance-podcast.net/podcast09/podcast_a80ecbd5-df3d-4c9d-bee7-4e3d9efc1974.xml` | 145 | 379 KB |
| Les Odyssées (France Inter) | `https://radiofrance-podcast.net/podcast09/podcast_c361798b-d6e3-4282-ba0a-ebb051b9e424.xml` | 204 | 589 KB |
| La Science CQFD (France Culture) | `https://radiofrance-podcast.net/podcast09/rss_14312.xml` | 185 | 601 KB |

The show is called **Bestioles** in the feed, and not « Les bestioles ».

How they were found:

- Radio France no longer prints the RSS link on the page of a show, and its feed list
  (`https://www.radiofrance.fr/rss`) is a JavaScript page that `curl` cannot read. Its linuxfr
  thread, [« radio france supprime les liens rss de ses émissions »](https://linuxfr.org/users/micromy/journaux/radio-france-supprime-les-liens-rss-des-ses-emissions),
  says the link is in the `⁝` menu at the top right of the page of the show.
- The feed directory [feeder.co](https://feeder.co/discover/235494f7e5/radiofrance-fr) lists the
  `podcast_<uuid>.xml` feeds of the site. The first two rows come from there, then `curl` checked
  that they answer.
- `rss_<number>.xml` is the old form of the address, still served: it answers `301` with a
  relative `Location: /podcast09/podcast_<uuid>.xml`. The user can keep either form.
- The `<itunes:new-feed-url>` of a feed names an address that includes another uuid
  (`/podcast09/<uuid>/podcast_….xml`). It answers `200` too, and the app does not follow it: it
  keeps the address that it was given.

### Apple's directory does not give these feeds

The iTunes search finds the shows and answers with an id, but **without a `feedUrl`**:

- Bestioles: id `1562100547`, 145 episodes.
- Les Odyssées: id `1469858852`. Affaires sensibles: id `912451024`.

`Podcast_subscribeFromItunes()` then fails with « Failed to lookup podcast », and the search page
cannot subscribe to a result with no `feedUrl`. This is why the shows need a feed address from
another source, and why this import exists.

### HTTP behaviour

| | Feed | Enclosure |
| --- | --- | --- |
| Scheme | HTTPS only; the certificate verifies with `res/cacert.pem` | HTTPS |
| Redirects | none (the `rss_N.xml` form: one `301`) | `proxycast.radiofrance.fr/<uuid>/<show>/<episode>/<file>.m4a` answers `302` to `media.radiofrance-podcast.net/podcast09/<file>.m4a?provider=public&cu=<show uuid>` |
| Content-Type | `application/xml; charset=utf-8` | `audio/mp4`, `Accept-Ranges: bytes`, a `Content-Length` |
| Caching | `Cache-Control: max-age=66844` (18 h), served by Akamai | `max-age=2592000` (30 days) |
| Compression | none, `Content-Length` known | |

No tracking host (podtrac, chtbl) sits between the feed and the audio: one hop through a Radio
France proxy, nothing else.

### What the feed holds

- `<rss>` 2.0 with the namespaces `itunes`, `pa` (Podcast Addict), `podcastRF` and `googleplay`.
  UTF-8, no CDATA: the descriptions are escaped (`&lt;a href="…"&gt;`), and begin with
  `durée : 00:11:49  - Bestioles - …`.
- `<itunes:duration>` is always `hh:mm:ss` (`00:11:49`), never a count of seconds.
- `<pubDate>` is RFC 2822 with a numeric zone: `Fri, 28 Aug 2026 04:42:00 +0200`.
- Artwork: `<image><url>` and `<itunes:image href>`, the same 1400×1400 JPEG of 150 KB.
- Enclosures: `type="audio/x-m4a"` and a `.m4a` file for 144 of the 145 items of Bestioles. The
  oldest item, a promotion of the Radio France app, is an `.mp3` on `media.radiofrance-podcast.net`.
- The longest `<description>` of the three feeds is 1298 characters.
- `<itunes:author>` is on the channel (`France Inter`), not a plain `<author>`.

### What the app did with them

Run on the trimmed feeds in `test/fixtures/` (`test/test_podcast_rss.c`):

| Point | Before | Now |
| --- | --- | --- |
| Redirects (`301`, `302`) | followed, `curl -L` | unchanged |
| HTTPS | verified against the bundle | unchanged |
| `hh:mm:ss` durations | read right | unchanged, tested |
| Feed buffer (5 MB) and the 2000 episodes | enough (601 KB, 204) | unchanged |
| **The file of an episode** | **always `<title>.mp3`**: the player picks its decoder by extension, so every `.m4a` of Radio France would be refused by the MP3 decoder after a complete download | **extension of the enclosure** (`podcast_enclosure_extension`) |
| `<itunes:author>` of the channel | not read, the author stayed empty | read |
| A page with a `<title>` and no episode | subscribed, with an empty list | refused: « No episodes in the feed » |
| `pubDate` zone | ignored (read as local time): two hours at most | unchanged |
| Descriptions over 1023 bytes | cut, possibly inside a UTF-8 character; the HTML stays as text | unchanged |
| Search of the Radio France shows | cannot subscribe (no `feedUrl`) | the import is the way |

## 2. The format

### Candidates

| | (a) text file of URLs | (b) OPML | (c) a saved `.rss`/`.xml` | (d) `.url` shortcut |
| --- | --- | --- | --- | --- |
| Many feeds at once | yes | yes | no, one | no, one |
| Made on a computer | any editor | export of every podcast app | « Save as » of a browser | drag the link from the browser |
| Made on a phone | paste in a notes app, share as a file | export, share | awkward | no (iOS and Android do not write `.url`) |
| Typo risk | the user types or pastes the address; no title to check | none: the app wrote it | none | none when dragged |
| Duplicates | seen by the parser | seen by the parser | | |
| Unreachable feed | per line, reported | per outline, reported | whole file | whole file |
| What it carries | the address that the user chose | the address, the title, the groups | **the episodes, not the address**: a saved feed has no URL to refresh from, only a `rel="self"` link when the show wrote one | the address |
| Privacy | the file lists what the user listens to, and stays on their card | same | same | same |

(c) is rejected: a copy of the XML is a snapshot, and the app needs the **address** to fetch new
episodes. It would also have to guess the address from `atom:link rel="self"` or
`itunes:new-feed-url`, and Radio France sets the second to another uuid than the one of the page.

### Recommendation

A **folder on the card**, `Podcasts/Subscriptions/`, that takes:

1. **`.txt`** (primary for a person): one address per line, `#` for a comment, an optional title
   after the address. It is the only format that a person writes by hand in ten seconds on any
   device, and the one that this page can document with an example.
2. **`.opml`** (primary for a migration): the standard export of every podcast app, many shows,
   groups and entities handled.
3. **`.url`**: a bonus, since the shortcut of a browser is a file that people already have.

Why this shape:

- **Ease**: nothing to type on the handheld. The user copies a file or a text with a computer or a
  phone, and opens `Podcasts`. The text file is the format of the example, so the first import is
  copy, paste, save.
- **Typos**: the parser takes `feed://`, `itpc://`, `pcast://` and `feed:https://` (what the
  « Subscribe » links of the sites give), trims, drops the fragment, lower-cases the scheme and the
  host, and refuses anything that is not an `http(s)` address. A line that is no address is counted,
  and a feed that does not answer shows its reason: the file is never silently half-read.
- **Duplicates**: the same address twice in a file counts once (the scheme and the host are
  compared in lower case). An address that is already subscribed shows as « Already subscribed ».
  The same show under two *different* addresses (the `rss_N.xml` form and its `podcast_<uuid>.xml`
  target) is **not** seen: it would take the feed title, which is known only after the fetch. It
  subscribes twice, and the user can unsubscribe one.
- **Unreachable feeds**: the import goes on with the next feed. The file that was read moves to
  `imported/`, and the feeds that failed are written to **`<name>.failed.txt`**, in the same text
  format, with the reason above each address. Nothing is lost: the user fixes the address and
  presses `Import from SD card`, which also reads the reports. The automatic import leaves the
  reports alone, so a dead address is not fetched at each visit.
- **Redirects**: `curl -L` follows them, and the address that the user wrote is the one that the app
  keeps (it does not follow `new-feed-url`).
- **Limits**: 200 feeds per file (the rest is reported as cut), 512 KB per file, 50 subscriptions in
  all (the existing cap), 16 files per run. A file with no feed, or too large, is renamed
  `<name>.invalid` and shows as a failure.
- **Privacy**: the files stay on the card. The only requests are to the feeds that the user listed,
  with the same user agent as the other transfers (`NextUI-Music-Player`).

### Example: Bestioles

`bestioles.txt`:

```
# France Inter, 5-7 years. The title after the address is optional.
https://radiofrance-podcast.net/podcast09/podcast_a80ecbd5-df3d-4c9d-bee7-4e3d9efc1974.xml Bestioles
```

The same in OPML, `radiofrance.opml`:

```xml
<?xml version="1.0" encoding="UTF-8"?>
<opml version="2.0">
  <head><title>Radio France</title></head>
  <body>
    <outline type="rss" text="Bestioles"
             xmlUrl="https://radiofrance-podcast.net/podcast09/podcast_a80ecbd5-df3d-4c9d-bee7-4e3d9efc1974.xml"/>
    <outline type="rss" text="Les Odyssées"
             xmlUrl="https://radiofrance-podcast.net/podcast09/podcast_c361798b-d6e3-4282-ba0a-ebb051b9e424.xml"/>
  </body>
</opml>
```

And `bestioles.url`:

```
[InternetShortcut]
URL=https://radiofrance-podcast.net/podcast09/podcast_a80ecbd5-df3d-4c9d-bee7-4e3d9efc1974.xml
```

## 3. How it works

| File | Role |
| --- | --- |
| `src/podcast_import_parse.c` | Pure C: format by file name, URL normalization, the three parsers. Tested on the host. |
| `src/podcast_import.c` | The folder, the thread, `Podcast_subscribe()` per feed, the moves, the failure report. |
| `src/ui/podcast_import_page.cpp` | The page: a title with the progress, a row per feed (added, already subscribed, or the reason). |
| `src/ui/podcast_manage_page.cpp` | The row « Import from SD card ». |
| `src/module_podcast.c` | `PodcastModule_autoRefresh()`: creates the folder, then starts the import if files wait and Wi-Fi is on, else refreshes as before. |

- **Triggers**: (1) the Podcasts page opens and Wi-Fi is on: the import starts by itself if the
  folder holds a file, the page says « Importing podcasts… » then the count (« 2 podcasts added, 1
  failed, see Import from SD card »); (2) `Y` → `Search and top shows` → `Import from SD card`
  (after the Wi-Fi dialog) shows the progress and the result of each feed, and reads the reports.
- **No blocking**: the work is on a thread, as the search and the refresh are. The page polls
  `PodcastImport_getStatus()`. Leaving the page does not stop the import: the module keeps itself
  initialized while it runs (as for a download), and the import ends with a refresh of the shows.
- **Folder**: `<SD>/Podcasts/Subscriptions/`, created with a `README.md` when the user opens
  Podcasts. Hidden files (`._x.txt` of macOS) are ignored.

### Tests

- `test/test_podcast_import_parse.c`: file names, URL normalization (schemes, case, fragments,
  length), the text format (comments, BOM, CRLF, titles, duplicates, cap), the `.url` file, OPML
  (nested outlines, entities, case of attributes, malformed input).
  Fixture: `test/fixtures/podcast_import_nested.opml`.
- `test/test_podcast_rss.c`: the RSS parser on the trimmed Radio France feeds
  `test/fixtures/radiofrance_bestioles.xml` (the first three items and the oldest, an `.mp3`) and
  `radiofrance_odyssees.xml`, and `podcast_enclosure_extension`.

Run them with `make -C test bin/test_podcast_rss bin/test_podcast_import_parse` and execute the
binaries in `test/bin/`.

### Not done

- The same show under two addresses is not merged (see above).
- Descriptions are shown with their HTML (`<a href>`), as the feeds give them.
- No import of a `.rss`/`.xml` file, and no `.webloc`.

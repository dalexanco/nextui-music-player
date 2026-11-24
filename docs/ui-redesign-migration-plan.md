# UI redesign — iterative migration plan

From the current UI on `main` to the target designed in the canvas
(<https://claude.ai/artifact/DLebS2R726BgcjQZTvGA6W>, private; boards named in brackets below).

- **Baseline:** `origin/main` at `0a46e04` (page architecture: `ScreenView`, `ListScreenView`, `PageNavigator`).
- **Scope:** Music, Radio and Podcast first. Audiobook last (it lives in PR #31 and is rebased on the result).
- **Evidence:** the audit in `docs/audit/` (49 device screenshots) is the "before". Each step re-takes the
  same screenshots as the "after".

## Direction (fork-first)

- Work lands on the fork branch `feat/ui-redesign` (based on `origin/main` `0a46e04`), phases 0 to 3 on that one branch,
  one commit per step. No coordination with upstream is planned; upstream changes are merged in when convenient.
- Legacy C code met on the way (`ui_*.c`, `module_*.c` drawing and screen flow) is migrated to C++ pages the way the rest
  of the app was (`UiPageImpl`, `ScreenView`, `ListScreenView`). Business logic (`radio.c`, `podcast*.c`, `player.c`, ...)
  stays in C unless a step says otherwise.
- A painful migration (a big legacy module) is split into dedicated sub-tasks, each with its own worker.

## 1. Target in one page

- **Source colours:** one accent per source (Music magenta, Radio blue, Podcasts green, Audiobook amber, neutral grey
  for Settings). The accent drives the status pill, the button-hint pills and the background gradient.
- **Soft background:** two diffuse accent halos over a dark base. No hard gradient.
- **Big white title** (as the *Shortcuts* screen of the launcher), with a side margin aligned on the row text.
- **Lists:** NextUI pill rows, 90 px. The selected pill extends over the right-hand value, the long title scrolls
  horizontally. No two-line rows. Rows of Music / Podcast / Audiobook lists carry no durations.
- **Detail panel on the right** for lists, in 1/3 of the width: square art, then the same block everywhere:
  line (artist / author / source), progress bar, position (`Track 2 of 15`), `elapsed / total`.
- **Sub-list (X):** same page for queue, chapters, episodes, stations. No art, full-width rows.
- **Player:** art fills the right side with a fade, text on the left, chips, progress row, hints `Y OPTIONS | X <sub-list>  A <verb>`.
  No spectrum.
- **Options (Y):** right-hand panel. Already approved as is.
- **States:** empty, no network, stream error, connecting [`tpl-radio-connexion`].

Common key map across sources: `A` primary verb, `B` back, `X` sub-list, `Y` options, `START` help.

## 2. What exists today (to verify at the start of each step)

- **Already native pages** (C++, `UiPageImpl`, `ListScreenView`): main menu, library, radio station list, settings,
  about, errors, dialogs, update / yt-dlp installers.
- **Still legacy C modules wrapped by `LegacyModulePageImpl`:** file browser (`ui_music.c`, `module_file_browser.c`),
  playlists (`ui_playlist.c`), now playing music (`ui_music_playing.c`), now playing podcast
  (`ui_podcast_playing.c`), podcast browser (`ui_podcast.c`, 1748 lines), radio player (`ui_radio_playing.c`),
  radio management, downloader, resume.
- **Theme:** `ui_theme.c` resolves `ThemeRole` colours from the active NextUI theme; page background is a flat fill
  in `page_stack_loop.cpp`.
- **Reusable views:** `ProgressBarView`, `BitmapView`, `BoxView`, `TextView`, `EmptyStateView`, `ScrollIndicatorView`.

## 3. Rules for every step

- One PR per step, mergeable alone; the app stays fully usable between steps.
- A new look is **opt-in until the end of its phase**: setting *Source colours* (off = today's look, pixel-identical).
  The switch is removed in the final step.
- Validation on the Brick: `./dev build brick tsps`, `./dev install device`, then the screenshots of the screens touched,
  taken with the `--test-control` channel (one command per line). Compare with `docs/audit/screens/`.
- Unit tests for any logic extracted from a legacy module (`test/test_*.cpp`).
- Bugs found while migrating a screen are fixed in a separate commit of the same PR.

## 4. Phases and steps

### Phase 0 — Foundations (no screen is migrated yet)

> **Phase 0 status: code done and built for tg5040 / tg5050; not yet validated on the Brick** (device unreachable when the phase ended).
> `docs/redesign/phase0/` only holds host renders of the gradient and of the art tile, made with the real render functions.
> To do on the device: both values of the setting on the main menu, Settings, Library (panel) and the radio list; frame time; legacy pages with the gradient behind.

**0.1 Source accent in the theme** *(smallest change)*
- Add a `Source` enum (Music, Radio, Podcast, Audiobook, Neutral) and `Theme_getAccent(Source)` with the accent / deep
  colours from the canvas. Add `ThemeRole` entries only if a role is needed (`THEME_ROLE_ACCENT`).
- Files: `ui_theme.h/.c`, `ui_theme_utils.c`.
- Validation: nothing visible; unit test on the colour table.
- **Status: done** — `ThemeSource` enum (`THEME_SOURCE_*`), `Theme_getAccent()` / `Theme_getDeep()` in `ui_theme_utils.c`; test in `test_ui_theme.c`.

**0.2 Soft gradient background**
- A page declares its `Source`; `page_stack_loop.cpp` paints the two-halo gradient (pre-rendered once per source,
  blitted each frame) instead of the flat fill. Setting *Source colours* off keeps the flat fill.
- Files: `page_stack_loop.cpp`, `ui/page/ui_page.hpp` (`getSource()`), `settings_page.cpp`, settings storage.
- Validation: main menu and settings screenshots, both settings values. Frame time not worse (check `frame_state`).
- Risk: gradient blit cost on tg5040 — pre-render to a cached surface.
- **Status: done** — `BackgroundView` (layer 1, cached image per palette), `UiPageImplBase::getSource()`, `PageStack::setBackgroundPalette()`, setting `SETTING_SOURCE_COLOURS` ("Source colours" row in Settings, default off). Tests: `test_source_gradient`, `test_page_stack`. Frame time not measured (no device available).

**0.3 Accent on the status and hint pills**
- `StatusGroupView` and `ButtonHintsView` take the accent of the page source instead of the theme colour.
- Files: `view/status_group_view.cpp`, `view/button_hints_view.cpp`, `view/screen_view.cpp`.
- Validation: any native page; sizes unchanged (the existing pills already match the canvas).
- **Status: done** — done through the `AccentPills` RAII (`ui/view/accent_pills.hpp`), which swaps the platform pill colour around the draw; `StatusGroupView` and `ButtonHintsView` render again when the accent changes. Native pages only (legacy modules keep their pills).

**0.4 Large white title**
- `TitleTextView` uses the large title font in primary colour, with the left margin of the rows.
- Files: `view/title_text_view.cpp`, `view/screen_view.cpp`, `ui_fonts.c` (font size constant).
- Validation: all native pages (title width vs status pill; long titles scroll).
- Risk: this touches every page; keep it behind the same setting.
- **Status: done** — `Fonts_getHeading()` (20 pt, about 60 px on the Brick), `TitleTextView::setHeading()`, applied by `ScreenView::layout()` while the setting is on.

**0.5 Extended-pill row**
- `ListRowView`: when selected, the pill grows over the right-hand view, and the text keeps its horizontal scroll;
  muted `THEME_ROLE_SECONDARY` colour for the right value of unselected rows.
- Files: `view/list_row_view.cpp/.hpp`, `view/list_view.cpp`.
- Validation: settings page (values on the right), radio list.
- **Status: done** — `ListRowView::setRightText()`: the pill takes the whole row and the band goes away; the right text is muted when not selected. Used by Settings and the radio list. No host test (the view tests link the desktop objects).

**0.6 Detail panel view**
- New `DetailPanelView` = `ArtTileView` (square, side = 1/3 of the width, `BitmapView` with generated gradient tile and
  title as fallback) + `InfoBlockView` (line, `ProgressBarView`, position, elapsed / total).
- `ListScreenView::setSidePanel(UiView*)` reserves the right third and shrinks the list.
- Files: new `view/detail_panel_view.*`, `view/art_tile_view.*`; edit `view/list_screen_view.*`; tests for the layout maths.
- Validation: a throw-away page in the *About* debug entry, or the library page behind the setting.
- **Status: done** — `DetailPanelView`, `ArtTileView`, `InfoBlockView`, `ScreenView::setSidePanel()` (also on `ListScreenView`), pure geometry in `detail_panel_layout.*` (`test_detail_panel_layout`). Wired in the Library page behind the setting with demo content (row title tile, description, position in the list).

### Phase 1 — Music

**1.1 Main menu** — native already. Per-entry accent, preview panel of the focused entry, *Resume* row on one line.
- Files: `main_menu_page.cpp`. Board: `Main`. Validation: screenshots 01–02 of the audit.
- **Status: done (not validated on device)** — `getSource()` returns the source of the focused entry (it is read at each layout, so no new API was needed: the background, pills and tile follow the cursor); preview `DetailPanelView` per entry (Now Playing: track, artist, progress; Resume: track and position). The Resume row was already one line.

**1.2 Library** — native already. List with the detail panel (art, artist, position).
- Files: `library_page.cpp`. Board: `tpl-liste-musique`.
- **Status: done (not validated on device)** — the panel lines are real: count of playlists, last track played (Resume record), downloader description. Library rows are menu entries, not tracks, hence no artist / progress of a track.

**1.3 Playlists** — wrap-free native page replacing `ui_playlist.c` (130 lines, the smallest legacy module).
- Files: new `ui/playlists_page` rewrite, `module_playlist.c` kept as logic only. Board: `tpl-liste`.
- Validation: audit screens 16–19.
- **Status: done (built, not validated on device)** — `PlaylistsPage` (list + panel, Y new, X delete) and the new `PlaylistTracksPage` (A plays through `NowPlayingPage`, X removes) are native; `module_playlist.*` and `ui_playlist.*` are deleted (no logic was left in them: it all lived in the module loop). New reusable pieces: `TextInputPage` (on-screen keyboard as a legacy page with a `std::string` result), `ListRowView::setIcon()` + `IconView` (format icons in rows).

**1.4 File browser** — native list page replacing `ui_music.c`; directory rows and audio rows; panel shows tags + art.
- Files: `file_browser_page.cpp`, `browser.c` kept as logic. Validation: audit screens 04–06, 15.
- Risk: the biggest behaviour surface of Music (selection memory, multi-select for playlists). Extract the
  navigation state into a testable class first (own commit).
- **Status 1.4a: done** — `BrowserNav` (`src/browser_nav.*`, history of the cursor per folder and the choice of the row after a way up), tested by `test_browser_nav` (host, 11 tests pass).

**1.5 Queue (X)** — new native sub-list page [`tpl-sousliste`], reached from the player. First use of the shared
`SubListPage` (title + kind, full-width rows, playing indicator).
- **Status: done (built, not validated on device; not reachable yet)** — `SubListPage` + `SubListSource` (`ui/sub_list_page.*`: title, small kind label after the title, one-line full-width rows, optional right value, playing indicator that follows the source every 500 ms, `A` verb configurable, empty message) and `makeMusicQueuePage()` (`ui/music_queue_page.*`, backed by new `MusicSession_getTrack/getCurrentIndex/playTrack`). The legacy player cannot open a page, so the queue is wired to `X` in 1.6.

**1.6 Music player** — native page replacing `ui_music_playing.c` (429 lines): art on the right with fade, text, chips,
progress row. Spectrum removed here. Lyrics stay a full-page mode until 1.8.
- Files: new `ui/music_playing_page.*`, `now_playing_page.cpp`. Board: `tpl-lecteur`.
- Validation: audit screens 07–12, 20, 21; screen-off behaviour unchanged.
- **Status: done, with deviations (built, NOT validated on device — screen-off, key lock, seek scan and art fade need a check on the Brick)** —
  `MusicPlayerPage` (`ui/music_player_page.cpp`, opened by `NowPlayingPage` while the music plays; `ResumePage` now starts the session and opens it).
  `module_music_playing.*`, `ui_music_playing.*` and their test are deleted. Layout maths and time texts are in `ui/music_player_layout.*` (`test_music_player_layout`, 13 tests).
  Deviations, on purpose: (1) the new layout is the only layout of the player (a native rewrite cannot keep the legacy one); *Source colours* off only drops the gradient and the accent pills;
  (2) the spectrum is removed (its code, `spectrum.c`, stays until 5.1; the `L2/L3` spectrum keys are gone); (3) `Y` toggles shuffle in this step and becomes the options panel in 1.7,
  repeat is not reachable between 1.6 and 1.7; (4) the legacy title marquee on the animation layer is replaced by `TextView` scrolling; (5) the lyric window is inline, as before.
  Behaviour kept: A pause, B back (saves the position or stops), L1/R1 and Up/Down previous/next, R2/R3 lyrics, hold Left/Right seek scan (`ModuleCommon_updateSeekScan`),
  Select tap and the inactivity timeout go through the wake hint to a dark screen (`ModuleCommon_*ScreenOffHint`), any press wakes, SELECT + A closes the hint, USB media keys through the page stack.
  `X` opens the queue sub-list (1.5).

**1.7 Options panel (Y)** — `OptionsPanelView` (right panel, title + scope, rows with `‹ value ›`) and the music options
(shuffle, repeat, lyrics, sleep timer, add to playlist). Replaces the START help for per-track settings. Board: `tpl-options`.
- **Status: done (built, not validated on device)** — `OptionsPanelView` (`ui/view/options_panel_view.*`, geometry in `options_panel_layout.*`, `test_options_panel_layout`) with `OptionRow {label, value}`, `‹ value ›` on the selected row, reusable by the radio and podcast players (`setScope`, `setRows`, `setValue`, `moveSelection`);
  `MusicOptionsPage` (`Y` from the player: shuffle, repeat, lyrics, sleep timer 15/30/45/60 min, add to playlist through `AddToPlaylistPage`).
  New logic: `sleep_timer.*` (`test_sleep_timer`, 8 tests) ticked by `MusicSession_tick`, so it also runs with the screen dark; `MusicSession_toggleLyrics()`.
  The page is full-screen (art, dim, panel), not a `Dialog`, so the hint pills of the player do not show under its own.

**1.8 Lyrics and help** — lyrics view in the new look; controls help page restyled.
- **Status: done, reduced (built, not validated on device)** — the lyric window stays inline in the player (a full-page lyrics mode was not built: no board asks for it): the current line is larger and in the primary colour, the others secondary.
  Help: the player table is rewritten for the new key map (X queue, Y options, no spectrum), new `HELP_PLAYER_OPTIONS` and `HELP_QUEUE` tables (`SubListPage` takes a `HelpId`), and the help dialog border takes the accent of the screen it explains while *Source colours* is on (`DialogView::setEdgeColor`).

### Phase 2 — Radio (closest to existing: the list is already a page)

**2.1 Station list** — `radio_page.cpp` with the panel (logo tile, genre · bitrate, favourite state). Board: `tpl-liste` (radio).
- **Status: done (built, not validated on device)** — the list takes a `DetailPanelView` while *Source colours* is on: generated tile with the name, slogan (or name), `genre · bitrate`, `Favourite` mark; rows carry no genre on the right in the new look (old look unchanged). Shared by the next steps: `ui/radio_station_panel.*` (`StationPanelText`, `makeStationPanelText`, `fillStationPanel`, `getStationBitrate`; the bitrate is known only for the station that plays) and the new favourites set `radio_favourites.*` (pure, tested by `test_radio_favourites`) with its store `radio_favourites_store.c` (`radio/favourites.txt`, by URL, so `stations.txt` keeps its format).
**2.2 Station management** — native page replacing the legacy radio management [`tpl-radio-gestion`]: favourites,
`X` favourite, `Y` options (rename, delete).
- **Status: done (built, not validated on device)** — `RadioManagePage` is native: the stations of the user with a star (`getStarIcon()`, `ui/view/star_icon.*`) on the favourites, the detail panel, `A` listens (through the Wi-Fi check), `X` toggles the favourite, `Y` opens `RadioOptionsPage` (full page with an `OptionsPanelView`, scope STATION: Favourite on/off, Rename, Remove, Add stations; it returns a `RadioStationAction`). Rename goes through `TextInputPage` and `cleanStationName()` (`radio_station_name.*`, `test_radio_station_name`; the `|` of the station file cannot reach a name). The favourites are the set of 2.1. Deviation: *Add stations* still opens the country directory of the legacy module, now only wrapped by `RadioBrowsePage` (`module_radio_manage.c` / `ui_radio.c` stay until 2.3).
**2.3 Search / add station** — native page with the search field, results, panel [`tpl-radio-recherche`]; uses the
existing keyboard (`ui_keyboard.c`) and the station directory code in `radio.c`.
- **Status: done, with deviations (built, not validated on device)** — `RadioSearchPage` (`ui/radio_search_page.*`) replaces the country browser: `module_radio_manage.*`, `ui_radio.*` and the temporary `RadioBrowsePage` are deleted, so is the `HELP_RADIO_HELP` screen (the manual `stations.txt` instructions are gone with it). Search is a pure function over the curated directory of `radio_curated.c` (`radio_search.*`, `test_radio_search`: words in the name, genre, slogan or country, case folded, sorted by name). `Y` (or `A` on the first row) opens `TextInputPage`; `X` cycles the country filter; `A` adds the station (already added: toast; the row then shows *Added*). Panel: tile, slogan, `genre · country`, *New* / *Added*. Deviations: (1) the search field is the first row of the list, not a pill fixed above it (the list screen has no header slot); (2) an empty keyboard result is a cancel, a single space clears the search; (3) the directory is the local `stations/*.json` only, as before: there is no online directory.
**2.4 Radio player + states** — native replacement of `ui_radio_playing.c` (521 lines) with the connecting /
reconnecting / offline states [`tpl-radio-connexion`]. Live chip instead of a progress bar.
- Validation: audit screens 39–43; unplug network and check the offline state.
- Risk: stream logic (`radio.c`, 1968 lines) must stay untouched; the page only observes its state.
- **Status: done, with deviations (built, NOT validated on device — screen-off, key lock, the start of a station after its "connecting" frame, the sleep timer and the art fade need a check on the Brick)** —
  `RadioPlayingPage` (`ui/radio_playing_page.cpp`) is native and one template (`RadioPlayerPageImpl<Page>`) serves both `RadioPlayingPage` (result: the URL shown last, for the station list) and `NowPlayingPage` (`createRadioPlayerPageImpl()`, picked for `BG_RADIO`).
  `module_radio_playing.*` and `ui_radio_playing.*` are deleted; `radio.c`, `icy.c` and the stream code are untouched. Layout reuses `layoutPlayer()` and `ArtFadeView` of the music player, plus the new shared `ChipView` (`ui/view/chip_view.*`, moved out of the music page).
  Looks (`radio_player_state.*`, `test_radio_player_state`): LIVE (chip + "Listening for N min"), CONNECTING / RECONNECTING (chip + buffer bar + "Buffer N %"), PAUSED, OFFLINE (error text in the error colour, `A` = RETRY). `X` opens the stations sub-list (`makeRadioStationsPage()`, a `SubListPage`), `Y` the options (`RadioOptionsPage::forPlayer()`: favourite, sleep timer), Up/R1 and Down/L1 step the station, `A` stops / cancels / listens / retries, `B` leaves the station playing in the background, Select tap and the inactivity timeout go through the wake hint to a dark screen as in the music player.
  The station starts one frame after the connecting screen was laid out (`Radio_play()` blocks the thread for the whole connection).
  New logic: `radio_sleep.*` (sleep timer of the radio on the `sleep_timer.*` countdown, ticked by the page and by `Background_tick()` for `BG_RADIO`, so it also runs with the player closed).
  Deviations, on purpose: (1) the spectrum is gone from the radio too (`L2/L3` keys removed); (2) the radio has no retry counter ("attempt 2 / 3" of the board): `radio.c` ends a failed stream in `RADIO_STATE_ERROR` and exposes no retry count, so RECONNECTING means "the stream ran low on sound and fills its buffer again"; (3) the old triangular album-art background is replaced by the art fade of the music player (`Radio_getAlbumArt()` while no fetch runs, else the generated tile); (4) the offline art is not greyed.
  Fix in the same commit: the help dialog of the radio screens kept the podcast accent after 2.3 removed the radio help screen (`controls_help_page.cpp`).

> **Phase 2 status: code done and built for tg5040 / tg5050; not validated on the Brick** (device unreachable). Host tests added: `test_radio_favourites`, `test_radio_station_name`, `test_radio_search`, `test_radio_player_state`.

### Phase 3 — Podcasts (largest legacy module, last of the three)

**3.1 Shows list** — subscriptions list with the detail panel (show art, last episode, update date).
**3.2 Episode list (X-style)** — shared `SubListPage`: episodes with *new / listened* markers and no duration column.
**3.3 Podcast player** — native replacement of `ui_podcast_playing.c` (575 lines): speed and sleep timer go to the options panel.
**3.4 Search, top shows, management** — replace the remaining screens of `ui_podcast.c` (1748 lines) one by one;
the file is deleted when the last one moves.
- Risk: size. Split 3.4 per screen, each its own PR. Keep download / RSS code untouched.

### Phase 4 — Audiobook (PR #31)

**4.1 Rebase `feat/audiobook`** on the migrated base; the Audiobook list and player use `DetailPanelView`, the shared player
and `SubListPage` (chapters).
**4.2 Audit bugs** (found in `docs/audit/README.md`), each with a test where possible:
- `A` of Select+A wakes the screen **and** pauses.
- The `A PLAY` hint disappears (the bar shows only two pairs).
- The help text is truncated (`Mark Finished/Unfini…`).
- The chapter title is oversized in the player.
- The author is empty for MP3 books.

### Phase 5 — Clean-up

**5.1** Remove the *Source colours* switch (new look is the only one), the legacy `ui_*.c` drawing code that nothing calls,
and the unused `ui_utils.c` helpers. Update `README.md` screenshots and `docs/audit` ("after" set).
**5.2** Empty and error states for the remaining pages (`EmptyStateView` restyled) — board `tpl-etats`.

## 5. Order and dependencies

```
0.1 → 0.2 → 0.3 → 0.4 → 0.5 → 0.6
                                 ↓
        1.1  1.2  →  1.3 → 1.4 → 1.5 → 1.6 → 1.7 → 1.8
                                 ↓
                  2.1 → 2.2 → 2.3 → 2.4        (needs 0.6, 1.5, 1.7)
                                 ↓
                  3.1 → 3.2 → 3.3 → 3.4        (needs 1.5, 1.6, 1.7)
                                 ↓
                         4.1 → 4.2 → 5.1 → 5.2
```

- 0.x can ship in any order after 0.1, except 0.6 after 0.5.
- 1.1 and 1.2 only need Phase 0; they are the first visible result (smallest risk).
- Phase 2 can start in parallel with 1.3+ once 0.6 and the shared sub-list exist.

## 6. Open points

- Source colours as a user setting or fixed? (Plan assumes a setting until 5.1.)
- Does the NextUI theme (user themes) override the accent, or is the accent always the source colour?
- Podcasts: keep the speed / sleep timer in the options panel only, or also on a hint key?
- Spectrum: confirmed removed; check nobody relies on its setting before deleting the code in 5.1.
- Art for stations and podcasts without a logo: generated gradient tile with the title (as in the canvas) — to confirm on device.

# NextUI Music Player

A C/SDL2 music, radio, and podcast player packaged as a NextUI "pak" (TOOL type)
for TrimUI handhelds. Fork of `mohammadsyuhada/nextui-music-player`, now
maintained at `nborodikhin/nextui-music-player`.

Target platforms: `tg5040` (TrimUI Smart Pro / Brick), `tg5050` (Smart Pro S),
plus a `desktop` target for local dev only (never shipped).

## Build

The Makefile references sibling directories (`../../all/common`,
`../../$(PLATFORM)/platform`, `../../$(PLATFORM)/libmsettings`), so this repo
**must** live inside a checked-out NextUI workspace at
`NextUI/workspace/nextui-music-player`. Building it standalone will not work.

```sh
sh build-tg5040.sh          # cross-compile in Docker toolchain + adb push
sh build-tg5050.sh          # same for tg5050 (stages libfdk-aac first)
sh build-desktop.sh         # native host build + run (macOS/Linux)
sh build-desktop.sh --build # build only

# manual
sh run-docker.sh /bin/sh -c 'cd nextui-music-player/src && make PLATFORM=tg5040'
```

`run-docker.sh` mounts the **parent** workspace dir at `/root/workspace` and
uses `ghcr.io/loveretro/${PLATFORM}-toolchain`.

Output: `bin/<platform>/musicplayer.elf` (gitignored). Objects go to
`src/build/<platform>/` (gitignored).

Releases: pushing a `v*` tag triggers `.github/workflows/release.yml`, which
checks out NextUI, builds both platforms, strips `src/ .git .github/ README.md`
and zips the rest into `Music.Player.pak.zip`.

Version bumps go through `python3 update_version.py` — it is the single entry
point and keeps `pak.json`, `state/app_version.txt`, `src/selfupdate.h`, and
`src/qr_code_data.h` in sync. Don't hand-edit those.

## Architecture

Three layers, by filename prefix in `src/`:

1. **Core / engine** (no prefix) — `player.c` (92K, the audio engine: decode,
   mix, resample, output), `radio*.c` (streaming, HLS, Icecast metadata,
   curated lists), `podcast*.c` (RSS, Apple Podcasts search), `playlist*.c`,
   `downloader.c` (yt-dlp driver), `http_download.c` / `wget_fetch.c`,
   `album_art.c`, `lyrics.c`, `spectrum.c`, `settings.c`, `resume.c`,
   `selfupdate.c`.
2. **`module_*.c`** — screen controllers. Each exposes a blocking
   `XModule_run(SDL_Surface*)` returning `ModuleExitReason`
   (`MODULE_EXIT_TO_MENU` / `MODULE_EXIT_QUIT`). `module_common.c` holds shared
   global input handling (START dialogs, volume, power/backlight, toasts) that
   every module calls each frame via `ModuleCommon_handleGlobalInput`.
3. **`ui_*.c`** — pure rendering + widgets for the corresponding module.
   `ui_utils.c` / `ui_utils.h` is the shared widget toolkit.

`musicplayer.c` is a thin `main()`: init subsystems, then a loop of
`MenuModule_run()` → dispatch to the selected module → repeat.

`background.c` tracks which of `BG_MUSIC` / `BG_RADIO` / `BG_PODCAST` owns
playback while the user is elsewhere in the UI; the main menu's "Now Playing"
slot routes back into the owning module through `Background_getActive()`.

### Vendored dependencies

`src/include/` (~12M) holds full vendored source for libopus, libogg, opusfile,
mbedTLS, yxml, parson. These are compiled into `libopus.a` and `libmbedtls.a`
by the same Makefile. `src/audio/` holds the single-header decoders
(`dr_mp3.h`, `dr_flac.h`, `dr_wav.h`, `stb_vorbis.h`, `minimp4.h`) plus
`kiss_fft` for the spectrum visualizer. AAC/M4A comes from the shared
`libfdk-aac` (prebuilt copy in `bin/tg5050/`; stubbed out on desktop via
`fdk_aac_desktop_stub.c`).

### Notable constraints

- `config_readonly_shim.h` is force-included into NextUI's `all/common/config.c`
  only. NextUI's `CFG_init` rewrites the global theme file as a side effect of
  reading it; the shim fails write-mode `fopen()` in that one translation unit
  so this pak never mutates global NextUI settings. Don't remove it.
- `EXT_SRC` files build into `$(OBJ_DIR)/ext/` on purpose — a plain pattern rule
  lets `../../` collapse out of the path and share objects between tg5040 and
  tg5050 builds, which silently mixes platform headers.
- `launch.sh` saves and restores the CPU governor/freq via `trap ... EXIT`, so a
  crash doesn't leave the device pinned.

## Directory layout

```
src/          all C sources (flat), Makefile, vendored libs under include/
src/audio/    single-header codecs + kiss_fft
bin/<plat>/   built .elf (gitignored) + libfdk-aac for tg5050
bin/          yt-dlp (34M), wget, keyboard — runtime helper binaries
res/          font.ttf (3.5M) + PNG icons
stations/     curated radio station lists, one JSON per country (USA/MLA/SNG/JPN)
state/        runtime state shipped in the pak (app_version, yt-dlp_version, queue)
```

Runtime user data lives on the device at
`.userdata/shared/music-player/` (e.g. `radio/stations.txt`); music in
`./Music`, podcasts in `./Podcasts`.

## Why the repo is large (162M)

`.git` alone is **107M**. The working tree is 55M. Causes, largest first:

| Path | History cost | Note |
|---|---|---|
| `bin/yt-dlp` | 171 MB over 5 revisions | 34M binary, replaced wholesale on each update — incompressible |
| `bins/ffmpeg` | 49 MB | 51M binary, added and later removed; still in history |
| `musicplayer.elf` (old root path) | 39 MB over 32 revisions | compiled binaries were committed |
| `bin/tg5040/musicplayer.elf` | 14 MB over 13 revisions | now gitignored |
| `bin/tg5050/musicplayer.elf` | 12 MB over 11 revisions | now gitignored |
| `res/font.ttf` | 3.5 MB | one revision, legitimately shipped |

Binaries are only gitignored as of a later commit, so the accumulated blobs stay
in history. Shrinking it requires a history rewrite (`git filter-repo`) plus a
force push — coordinate before doing that. `bin/yt-dlp` still has to be tracked
for the pak to ship, so future updates keep adding ~35M each.

## Features (for context)

Library playback (WAV/MP3/OGG/FLAC/M4A/AAC/OPUS) with file browser, shuffle,
repeat, playlists, spectrum visualizer, album art auto-download, lyrics
download; YouTube Music search/download via bundled `yt-dlp`; online radio
(Shoutcast/Icecast + HLS, HTTPS via mbedTLS) with presets and curated
per-country lists; Apple Podcasts search, subscriptions, and episode downloads;
Bluetooth/USB-C output with media-key control; in-app self-update.

# Sniper 87, reconstructed

A C reconstruction of the 1996 Windows game *Sniper* (version 87), the
predecessor of SubSpace. It builds a playable Win32 client from the portable
game code, plus a native SDL2 client for Linux that is a work in progress.

This is reconstructed source, not the original source text. Behavior was
checked against the original executable, but full equivalence has not been
established.

## You need your own copy of the game

This repository contains no game assets: no bitmaps, sounds, dialogs, icons,
levels, help file or executable. The build generates them from your own copy
of the original archive, `sniper87.zip`:

| File in the archive | SHA-256 |
| --- | --- |
| `SNIPER.EXE` | `8aadc97250639528bde00149ad67365e603e991ee606196e59b0b89d156033d0` |

`tools/extract_assets.py` checks the archive, reads every resource out of
`SNIPER.EXE` and writes `build/assets/`, which is never committed:

- `sniper_assets.c`: every resource payload as C arrays, declared in
  `include/sniper_assets.h`
- `sniper_assets.rc`: the same payloads for the Win32 resource compiler
- `files/`: the data files the game reads at runtime (`LOCAL.LVL`,
  `SNIPER.HLP`, `ARENA.LST`, `45538382.LVL`, `LICENSE.TXT`) and `SNIPER.EXE`
  for the comparison tests

The extractor uses only the Python standard library and refuses any other
version of the game.

## Build

Requirements: GNU make 4.3 or newer, Python 3, and the `i686-w64-mingw32`
GCC/windres toolchain for Windows, or SDL2 for Linux.

```sh
make assets SNIPER_ZIP=/path/to/sniper87.zip
make app            # Win32 client: platform/build/sniper87-recreated.exe
make native-linux   # SDL2 client:  linux/build/native/sniper87-native
make test           # level decoding and checksum check
```

`app`, `test` and `native-linux` regenerate the assets when they are missing.

## Run

**Windows or Wine:** run `platform/build/sniper87-recreated.exe` from
`platform/build/`, where the build copies the game's data files.

- It needs DirectDraw and DirectSound. Built-in help needs a WinHelp viewer.
- The MinGW build links against the UCRT runtime, so it does not run on
  Windows 95.
- The map editor (Ctrl+Shift+Play Game) edits `LOCAL.LVL` in place and saves
  it when it exits, so keep a copy.

**Linux (work in progress):** run `linux/run-native.sh`. It plays the offline
session with map and ship rendering only: arrow keys fly, Shift boosts and
Escape quits. It has no sound, HUD, chat, radar, menus, editor or online play
yet. For the full game on Linux, run the Win32 client under Wine. See
`linux/README.md` for headless runs.

## Online play

`server/` contains a local server that the original client can connect to.
See `server/README.md`.

## What works

- **Offline play:** flight, weapons, collisions, pickups, chat, radar,
  statistics, the help panels, spectator mode, audio and the hidden level
  editor.
- **Online client:** connection, login, file download, gameplay, scores,
  squad and voice messages.

## Source layout

```
src/        portable game logic and network protocol
platform/   Win32 adapters: DirectDraw, DirectSound, sockets, dialogs
linux/      SDL2 client for the portable code (work in progress)
include/    sniper_assets.h (the API for generated assets)
tools/      extract_assets.py
server/     independent local server for the original client (see server/README.md)
tests/      comparison tests, session tests and runtime probes
```

## Tests

`make test` checks level decoding.

`make native-test` runs a differential suite. Each test emulates instruction
ranges of the original `SNIPER.EXE` with Unicorn and compares the results with
the reconstructed code. It needs Python with `pefile` and `unicorn`, plus the
assets from `make assets`.

The suite still assumes the maintainer's checkout layout and some prebuilt test
libraries, so it does not yet run in a fresh clone of this repository.

## Legal

*Sniper* and its assets belong to their original rights holders. This
repository distributes only reconstructed source code and tools; you must
supply your own copy of the game.

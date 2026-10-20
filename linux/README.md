# Native Linux layer (work in progress)

**Status:** work in progress. The client runs the offline session with map and
ship rendering and keyboard flight. Sound, the HUD and text panels, chat,
radar, menus, the editor and online play are not implemented yet; use the
Win32 client under Wine for those.

This directory contains a native Linux client for the recovered C gameplay modules.
The `native-linux.mk` target links the recovered offline session and gameplay
code directly against SDL2; it does not
replace the Win32 build and does not alter `src/` or
`platform/`. SDL2 supplies the Linux window, keyboard events, clock, and
software presentation.

Dependencies on Arch Linux:

```sh
sudo pacman -S --needed base-devel sdl2
```

Run:

```sh
make -C linux
./linux/build/sniper87-linux
```

Run the native reconstructed client from the repository root:

```sh
make native-linux
./linux/run-native.sh
```

The launcher changes to `platform/build` so the recovered `LOCAL.LVL` is loaded.
Ship and tile bitmaps are compiled in from the generated `build/assets/sniper_assets.c`
(`make assets SNIPER_ZIP=/path/to/sniper87.zip`); no asset files are read at runtime.
Arrow keys fly, Shift boosts, and Escape exits.

Controls: arrow keys fly, Shift boosts, Escape quits. For a finite headless
run, pass an explicit frame count:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./linux/build/sniper87-linux --frames 120
```

For a deterministic input smoke test, run:

```sh
SDL_VIDEODRIVER=dummy SDL_AUDIODRIVER=dummy ./linux/build/sniper87-linux --frames 120 --demo-input
```

The demo script applies thrust, turns in both directions, and then boosts; the
final position, velocity, and heading in the summary should be nonzero.

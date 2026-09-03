# W100h

W100h is a small retro-styled music player built around the AY-3-8910/PT3 audio path originally developed for Mode256.

Current bootstrap baseline:

- C++23 and SDL3 3.4.10;
- Ayumi AY-3-8910 synthesis at 48 kHz stereo;
- PT3 playback with embedded-loop handling at a 50 Hz sequencer rate;
- 02TS two-chip TurboSound support;
- 320x160 pixel-native UI framebuffer with nearest-neighbor presentation;
- no game-oriented AYFX/SFX path and no Mode256 graphics-profile laboratory.

The current bootstrap build starts the bundled `Pator - August Melancholy.pt3` track. File associations, playlist/library scanning, transport controls, and selectable themes are intentionally left for later patches.

## Build

```bash
./m
```

Other build commands:

```bash
./m release
./m clean
./m rebuild
```

CMake fetches pinned SDL3, Ayumi, and Volutar pt3player dependencies on the first configure.

## Run

```bash
./r
```

Temporary integer window scaling is available from the command line:

```bash
./r --scale 4
```

Press `Esc` to quit.

## Configuration

The bootstrap configuration lives in `config/default.ini`:

```ini
[window]
scale=3
vsync=true

[audio]
enabled=true
master_volume=80
music_enabled=true
music_volume=60
```

The music path deliberately preserves the proven Mode256 behavior. Only game-specific AYFX/SFX functionality was removed.

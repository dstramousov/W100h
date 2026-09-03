# Versions

## v0.0.0 -> v0.0.1

- init repo

## v0.0.1 -> v0.0.2

- Bootstrap W100h from the Mode256 v0.0.15 music stack while preserving PT3, 02TS TurboSound, dual-AY synthesis, 50 Hz sequencing, and SDL3 audio output.
- Remove game-only AYFX/SFX and the old Mode256 graphics laboratory; add a minimal 320x160 pixel-native single-window player shell with the built-in 5x7 font.
- Rename build/runtime identity to W100h and repair the `a` archive helper plus `appr` commit/push helper, including correct handling of untracked files.

## v0.0.2 -> v0.0.3

- Replace bundled-track autoplay with real startup/library behavior: bare launch scans `~/Music` and stays silent, while an explicit PT3 file starts immediately and seeds a playlist from its directory.
- Add deterministic recursive PT3 library scanning plus keyboard transport for play/pause, restart, stop, previous, and next without changing the proven PT3/AY/TurboSound synthesis path.
- Replace the bootstrap README with the fixed Russian project overview and UI concept board for the three selected themes; document stable usage, hotkeys, and repository layout.

## v0.0.3 -> v0.0.4

- Accept the Vortex Tracker II 1.0 PT3 header in addition to the classic ProTracker 3.x header, including mixed-header 02TS TurboSound containers.
- Add regression coverage for a two-chip 02TS payload whose second embedded PT3 module uses the Vortex Tracker II header.
- Suppress repeated identical playback-error log messages for the same track while keeping the UI in LOAD ERROR state.

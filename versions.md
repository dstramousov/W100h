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

## v0.0.4 -> v0.0.5

- Replace the bootstrap shell with the first real W100h cassette-deck front panel at 480x240 logical resolution and a default 960x480 window, using the approved dark-blue titanium/Spectrum visual direction.
- Add the transparent cassette window with embedded handwritten `Spring of 75 / Яхта, парус ....` label, animated reel teeth during playback, live track/status/time display, and keyboard-labelled transport controls.
- Make the on-screen transport buttons mouse-clickable and the front-panel volume knob functional through the mouse wheel while preserving the existing PT3/02TS/dual-AY audio path; reserve aligned AY channel-meter slots for the next telemetry patch.

## v0.0.5 -> v0.0.6

- Replace the procedural front-panel artwork with the approved 960x480 raster skin derived from the W100h dark-blue titanium cassette-player concept while keeping the actual window size at 960x480 by default.
- Keep live controls separate from the skin: render track/status/time, volume pointer, transport state, aligned six-channel AY placeholders, and animated cassette reel cores as runtime overlays on top of the static body.
- Move the renderer to the skin-native 960x480 framebuffer, copy the new BMP skin/reel assets during the build, preserve existing mouse/keyboard behavior and PT3/02TS audio, and leave README.md unchanged.

## v0.0.6 -> v0.0.7

- Replace the oversized circular reel overlays with compact square-toothed cassette spindle cores that animate only the real engagement teeth over the existing reel artwork.
- Add live dual-AY front-panel telemetry for channels 1A/1B/1C and 2A/2B/2C, with envelope-aware levels plus small aggregate NOISE/ENV indicators; inactive second-chip meters remain dark for normal single-AY PT3 tracks.
- Make the volume control visibly interactive with a moving green neon position marker and mouse drag in addition to wheel control, and suppress the upstream PT3 decoder's stray `Number of positions` stdout diagnostics during track setup.

## v0.0.7 -> v0.0.8

- Move cassette motion from the small synthetic inner spindle to the real large outer engagement ring already present in the approved skin, using 12-frame left/right reel animation while leaving the central hub static.
- Update the cassette label to the approved exact wording `Spring '75` / `Ялта парус........`, rendered in darker, slightly heavier and more relaxed handwriting.
- Keep the existing live AY meters, transport, volume interaction, PT3/02TS audio path, and fixed README unchanged.

## v0.0.8 -> v0.0.9

- Add a real PT3 transport timeline by reading the pinned decoder's current/loop positions through a narrow bridge, then show current/total time plus a green dotted progress bar across the live display.
- Reassign Left/Right to seek the active track by -10/+10 seconds, Up/Down to master volume, and Ctrl+Left/Ctrl+Right to previous/next composition while keeping the front-panel PREV/NEXT buttons as playlist controls.
- Implement seek reconstruction with AY synthesis advancement so tracker/envelope state remains coherent after jumps, preserve first-pass duration across Stop, and keep README.md unchanged.

## v0.0.9 -> v0.0.10

- Add per-user Linux desktop integration for W100h with a stable `org.w100h.player` SDL/desktop application ID, a dedicated scalable cassette-player icon, and a `.desktop` entry that passes one selected PT3 file directly to the existing autoplay command-line path.
- Register `audio/x-pt3` through shared-mime-info and make W100h the default PT3 handler so Nautilus double-click opens and immediately plays the selected module without sudo or hard-coded machine paths.
- Add safe optional Midnight Commander integration that inserts/removes one managed `.pt3` Enter rule in the user's `mc.ext.ini`, plus a short standalone Linux integration guide while keeping the fixed README unchanged.

## v0.0.10 -> v0.0.11

- Rebuild cassette reel animation around a fixed pivot with 24 frames and rotate only the photographed outer engagement annulus, eliminating the visible orbital wobble while keeping the hub and cassette body static.
- Restore a subtle rounded lower chassis edge, lift the keyboard legends away from the window boundary, and remove the clipped footer text that made the bottom of the player look cut off.
- Split NOISE and ENVELOPE telemetry per AY chip and center compact N/E lamps under each chip's middle B channel so single-AY and TurboSound activity is displayed symmetrically and truthfully; keep README.md unchanged.

## v0.0.11 -> v0.0.12

- Rebuild the cassette reel strip so every animation frame uses the exact same fixed center and diameter, eliminating residual orbital wobble.
- Animate only the six pale cassette engagement teeth around each hub; keep the outer reel ring, central dark hub, cassette body, and surrounding artwork completely static.
- Preserve the existing transport, live AY meters, volume control, Linux integration, and fixed README unchanged.

## v0.0.12 -> v0.0.13

- Rebuild both reel strips from the approved cassette skin around one exact fixed 104x104 pivot, rotating the pale outer plastic engagement annulus without moving the hub center.
- Make left and right reel animation use the same frame order and phase so both cassette hubs rotate together in the same physical direction during playback.
- Keep the inner dark spindle, reel diameter, transport, meters, volume, Linux integration, and fixed README unchanged.

## v0.0.13 -> v0.0.14

- Replace the pre-rendered reel animation strip with mathematically centered six-tooth cassette engagement hubs, so both reels rotate continuously around immutable pivots without per-frame crop or center drift.
- Keep the cassette body, outer reel rings, and dark central spindles static; animate only the pale plastic engagement teeth in the same direction and phase on both reels.
- Remove the obsolete reel-frame asset and preserve the existing transport, meters, volume, Linux integration, and fixed README unchanged.

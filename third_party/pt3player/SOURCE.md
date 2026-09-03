# Volutar pt3player source

W100h uses the PT3 decoder core from `Volutar/pt3player`.

- Upstream: `https://github.com/Volutar/pt3player`
- Pinned commit: `aaa5c321466d955b8a96509216a2e5cbc860d7b3`
- Files compiled by W100h: `pt3player.c`, `pt3player.h`
- License: MIT (see `LICENSE`)

The Windows audio frontend from the upstream project is not used. W100h only uses
its PT3 register sequencer; Ayumi and SDL3 remain responsible for synthesis and audio
output inside W100h.

# Mega Man X5 Recompiled — v0.0.1-alpha

The first public cut. Mega Man X5 boots from the real PlayStation BIOS and
**plays** as a native Windows program — no emulator behind it — on the
[PSXRecomp](https://github.com/mstan/psxrecomp) framework, the same one behind
[TombaRecomp](https://github.com/mstan/TombaRecomp) and
[MegaManX6Recomp](https://github.com/mstan/MegaManX6Recomp). The game's MIPS code
is machine-translated ahead of time into native C and compiled into a real
Windows executable that runs on a faithful simulation of the PS1 hardware plus
the recompiled PS1 BIOS.

## ✨ Highlights — what works

- **Boots and plays.** PS1 BIOS → disc detect → engine load → opening → stage
  gameplay, with **no known crashes**.
- **Intro cutscenes / FMV now decode and play.** This is the headline of this
  release. The opening movies (`CAPLOGO.STR` / `X5OP.STR`, with `BGM.XA` audio)
  previously bailed out with **zero decodes**; a faithful CD read/seek-timing
  fix in the framework (the CD "data ready" cadence was firing too eagerly, so
  the movie player gave up before it decoded a frame) makes them play. This is
  what makes X5 release-worthy.
- **Memory-card save / load.** Standard PS1 `.mcd` / `.mcr` images,
  emulator-compatible.
- **Controller input.** MMX5 requires an analog-capable pad before it reads
  buttons, so the runtime presents a DualShock by default. Keyboard and SDL
  gamepads both work; per-player override in the launcher.
- **Fast loading (turbo loads).** Loads fast-forward the whole machine at full
  host speed while keeping authentic 1× guest CD timing (and audio) intact.
- **FMV auto-skip toggle.** Off by default so you see the now-working intro;
  flip it on in the launcher (Settings → "Skip FMVs") to skip movies the instant
  they start.
- **Supersampling + anti-aliasing.** Internal-resolution SSAA (1×–4×) with
  optional linear present filtering.
- **Graphical launcher.** Pick BIOS / disc / memory cards, verify the disc, and
  configure renderer, supersampling, and controller — choices persist between
  launches.
- **Self-contained overlay toolchain.** As you explore new areas the runtime
  converts the game's overlay code to native code in the background. That needs
  no developer tools installed — the release bundles a fully self-contained
  toolchain (embedded Python + TinyCC), so newly visited areas are accelerated
  on any machine.

## ⚠️ Known issues

- **Not yet verified end-to-end.** Gameplay works with no known crashes, but a
  full start-to-finish playthrough hasn't been confirmed — please report where
  it happened if you hit something deep in a stage or boss.
- **OpenGL flicker → software is the default.** The OpenGL renderer shows
  intermittent black-frame flicker in this build, so the clean software renderer
  ships as the default. OpenGL is still selectable in the launcher if you want
  to try it. See `ISSUES.md`.
- **No widescreen this release.** X5 ships **4:3 only**. The true 2D wide
  field-of-view is not wired up yet — the background hook sites are located and
  documented but not implemented (see `annotations/widescreen_bg2d_sites.md`).

## 📝 Setup

- **Bring your own** PlayStation BIOS (`SCPH1001.BIN`) and Mega Man X5 (USA,
  SLUS-01334) disc image — the launcher asks for each. Verify your disc against
  `DISC.md` before reporting regressions. Use `.cue` + `.bin` (or `.bin`); do
  **not** convert to a 2048-byte "cooked" `.iso` — that discards the XA sectors
  the movies and audio stream from.
- Options live in the launcher's **Settings** and are remembered between
  launches.
- The overlay cache grows as you play; please keep `overlay_captures.json`
  private — it contains game code read from your disc (see README).

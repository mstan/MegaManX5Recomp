# MegaManX5Recomp v0.0.2-alpha

This release brings Mega Man X5 onto the current PSXRecomp runtime and launcher,
adds the Mods view, and exposes two reviewed presentation enhancements.

## Mods

The Mods view contains exactly two game-owned, default-disabled packages:

- **Widescreen** enables Mega Man X5's experimental true 16:9 field of view.
  It widens the Capcom 2D background tile window together with HUD bounds,
  object activation, and primitive culling. Authentic 4:3 remains the default.
- **Frame Interpolation** presents blended intermediate frames at the display
  refresh rate or a selected fixed rate from 90 through 240 FPS. It is
  presentation-only: game logic, VBlank, timers, audio, and machine speed remain
  at their stock cadence.

These are now Mods rather than generic display settings because both rely on
game-specific integration and should be explicit opt-in enhancements.

## Framework and launcher update

PSXRecomp and recomp-ui have been updated to their current MMX release
revisions. Notable changes since v0.0.1-alpha include:

- SDL3 as the default host backend;
- optimized MDEC/FMV hot paths and batched cycle accounting;
- faster launcher, game-start, and save-state paths;
- safer overlay caching and self-contained on-demand native compilation;
- updated OpenGL upload handling and Vulkan renderer improvements;
- improved disc, BIOS, controller, and launcher behavior;
- current fullscreen, filtering, memory-card, and Mods interfaces.

The prior intermittent OpenGL upload artifact has been fixed in the shared
runtime. OpenGL is now the release default, while software and Vulkan remain
available.

## OpenBIOS

The MIT-licensed OpenBIOS is bundled and selected automatically when no retail
BIOS is chosen. A legally obtained retail PlayStation BIOS remains optional.
Clearing a retail selection returns to **OpenBIOS (default)**.

## Setup and compatibility

- Bring your own legally obtained Mega Man X5 USA disc image (`SLUS-01334`).
- Use `.cue` plus `.bin` when available; do not convert the disc to a cooked
  2048-byte ISO because that discards XA sectors used by FMV and streamed audio.
- Existing standard PS1 memory-card images remain compatible.
- End-to-end completion has not yet been recertified on this build, so please
  report regressions with the stage, scene, and selected Mods.

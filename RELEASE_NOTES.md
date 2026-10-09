# v0.2.0-coop-alpha-debug (Windows prerelease)

Mega Man X5 now offers experimental simultaneous X + Zero co-op, default off.
Enable **X + Zero Co-op** in Mods and use OpenGL. Local play shares one camera;
netplay can use Unified or opt-in Split cameras, with independent displays of
the same world. Both peers must use this exact build and the USA SLUS-01334 disc.

- Configure separate Player 1 and Player 2 devices in the launcher for local
  play. In netplay, each peer binds its local device as Player 1 and the room
  assigns seats. LAN/Direct IP and Online Host/Join are included.
- P1 uses the natively selected X or Zero; P2 uses the counterpart when story
  availability permits. Health/resources and menus remain per player. P1 alone
  uses Ride Armor. Ride Chasers support both riders, surviving-player control
  and one shared retry after a team wipe; voluntary withdrawal is locked during
  the forced car sequence.
- Native arrival poses, stacked shared-screen meters, independent online HUDs,
  spectator fallback and resource-preserving departure/return are implemented.
- Measured fortress entry and controlled separated-player windows reached
  approximately 60 updates per second after reducing state-copy overhead and
  removing costly optional QA verification from ordinary playtests. Guest timing
  remains unchanged; other stages and hardware may perform differently.

Validation includes X/Zero selections, shared and independent displays, peer
state agreement, simulated jitter and render restoration, checkpoint/team retry,
P1-only Armor, both car death orders, survivor jumping, Select lock and shared
car retry. The owner accepted Grizzly's native defeat/rewards and subsequent
return with C-Slash and the defeated portrait retained. Full campaign completion,
late-game scripts and Internet transport are not certified; Internet play is
explicitly available for alpha user testing.

This Windows build includes debug support but excludes qualification cheats.
The co-op mod starts disabled and Unified is the default camera preference.
Bring your own disc; bundled OpenBIOS is included. No disc, retail BIOS or test
save is distributed. Existing saves should be backed up before alpha testing.
Co-op saves/settings use the launcher's isolated mod profile. Rewind/savestates
are unavailable while co-op is active. Widescreen/interpolation combinations
have not been qualified with co-op; use their defaults for initial testing.

# v0.1.0-alpha

- Windows and Linux packages include native shards freshly extracted from 53 original-disc code images, plus the BIOS resident helper.
- Every release invocation builds and audits the complete configured AOT inventory. Runtime compilation and interpreter fallback remain available.
- Original loader evidence, six explicit exclusions and coverage limits are documented in `docs/AOT_OVERLAYS.md`. Full static coverage is not claimed.
- Packaging uses current framework and launcher revisions and the same release version on both platforms.

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

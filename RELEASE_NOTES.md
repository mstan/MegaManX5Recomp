# v0.2.9-coop-alpha

Preserves the exact delivered v0.2.8 co-op title source (f715c9ff), including
23 bounded owner-report passes and the recorded A02/M06 gaps below. Later local
post-release fixes are not included. No new full-route, physical-controller,
LAN or per-title performance qualification is claimed by this packaging update.

- Local X + Zero co-op, side-by-side cameras, Either-player doors, native
  resident loading/frame waits, diagnostics and Alia suppression retain their
  delivered feature settings. Keep Zero remains off. Trusted overlay hooks use
  interpreted overlays; no unqualified native overlay cache is shipped.
- Uses the shared optimized release builder: fresh per-title selective PGO,
  public DEBUG_OFF, normal audio/window and bundled OpenBIOS. Profile booting
  is a build step, not an additional game regression qualification.
- Internal Resolution starts at 4x with 1x-to-32x choices in Mods. Hardware and
  GPU memory can clamp higher choices; existing saved choices are retained.

No personal settings, memory cards, QA snapshots, CD Speed or Fast Loading
packages are included. Preserve native memory-card progress; old snapshots
remain disposable build-specific files. See the historical v0.2.8 section and
docs/COOP_X5_027_FOLLOWUPS.md for the exact mechanic limits.

# v0.2.8-coop-alpha-debug (local Windows alpha)

This build packages the current owner-report campaign: **23 of 25 individual
reports have bounded passing checks; two remain outstanding below**. The owner
authorized this release with those gaps. The separate25 passing code tests do
not close gameplay reports. Individual evidence and limits are recorded in
docs/COOP_X5_027_FOLLOWUPS.md.

- Intro returns retain X's armor graphics. Both-player doors animate the
  companion through opening and traversal, and airborne passengers recover
  native floor collision. Actual defeated intro cars retire and respawn when
  the trailing player returns after P2 advances the checkpoint.
- Grizzly's pre-countdown background animates in both views. The recorded
  first P2-owned boss introduction returns hovering X safely.
- Kraken keeps each pane's own HUD throughout arrival and groups each rider
  with its bike. P2 shots destroy a native blockade with both players alive.
  Hidden Alia text preserves the original post-bike orb action and releases
  both players; a current-build mid-exit save/load also passes. Surviving Zero
  receives Zero's capsule dialogue and can collect its native armor grant.
- McWhalen retains its authored camera limits, native checkpoint retry and
  moving whale parts. P2-targeted AI now preserves the native hull-layer
  update, preventing the ship from separating from its fin and pods. Matched
  nearer-player controls, current-build save/load and cold load pass.
- Glow's chained platform contact, P2 virus/miniboss admission and stair
  animation have bounded passes. Native boss-door traversal and an actual
  Izzy Glow defeat/reward transition also pass.

Outstanding owner reports:

| ID | Remaining issue | Validation limit |
|---|---|---|
| A02 | P2's camera reportedly moves away from Zero after P2 triggers Alia and P1 returns. | The exact replay's four hints and current native continuations have not reproduced it. The owner's report remains open. |
| M06 | Both heroes reportedly die jumping down in McWhalen. | The original deaths are near native checkpoint3. A forward jump before defeating the admitted whale still kills X; the solo comparison failed to load. Native hazard versus co-op fault and normal post-defeat traversal remain unresolved. |

These are bounded mechanic checks, not full-route qualification. Some setups
bypass earlier traversal or prerequisites, as documented individually. The
historical parity audit, continuous Dinorex route, physical LAN and native
cold-save unlock/duel persistence remain outside this release's qualification.
No new performance or cross-build netplay compatibility claim is made.

Defaults remain local split-screen, Either-player doors, diagnostics and Alia
hint suppression, P1 keyboard/P2 DualSense, normal window and audio. Bring your
own USA SLUS-01334 disc; OpenBIOS is included. No personal saves, recordings,
QA states, CD Speed or Fast Loading packages are included. Save states are
build-specific; preserve native memory-card saves separately. Delivery is local
to Nextcloud; no push, PR or remote publication.

# v0.2.7-coop-alpha-debug (local Windows alpha)

This build includes the five latest owner-report fixes from `89a0e32`.
The owner authorized delivery with the remaining Dinorex route validation
recorded below; release authorization does not mark those checks passed.

- The intro road keeps its original foreground objects while a distant P2
  advances the building checkpoint. They retire after the last view leaves.
- Both-player doors initialize the companion's native scripted entry and keep
  animating after a transient brake/landing pose ends. The original door still
  owns movement and timing, with safe clearance at closure.
- A leading P2 triggers Dinorex's original lower-region graphics transition
  while the trailing player's upper-region art remains intact.
- Dinorex's three fire-breakable stone walls use the admitting player's camera
  for their original stops, actual fire destruction and camera release.

Three of the five latest report checks pass bounded runtime validation:
intro foreground, companion door animation and all three stone-wall stops/
destructions. Dinorex background and fire art look correct in captured checks,
but one continuous P2-led descent into the fire corridor remains unverified.
Position cheats bypassed shelter traversal/survival for the later wall checks.
Full intro reveal/control recovery was not repeated by this bounded door check.
See docs/COOP_X5_026_FOLLOWUPS.md for individual evidence and limitations.
The existing 25 code checks pass. Full campaign parity, physical LAN and a
new performance improvement are not claimed.

The accepted native character selector is retained. Release defaults remain
local split-screen, Either-player doors, diagnostics and Alia hint suppression,
with P1 on keyboard and P2 targeting DualSense. Normal window/audio are enabled.
Bring your own USA SLUS-01334 disc; OpenBIOS is included. No personal saves,
recordings, QA states, CD Speed or Fast Loading packages are included. Older
save states remain disposable; preserve native memory-card saves separately.
Delivery is local to Nextcloud, with no push, PR or remote publication.

# v0.2.6-coop-alpha-debug (local Windows alpha)

This build closes the latest fifteen owner reports with bounded automated
evidence and owner playtest acceptance. The native unit selector lets P1 choose
a hero/form, then P2 choose a form of the remaining hero. One X and one Zero
remain; Black Zero still requires the Sigma stage 3 capsule unlock.

- The intro building backgrounds, P2 debris triggers and personal wall dust
  retain their own camera and owner. Both-player doors keep native companion
  animation and safe closing clearance, including backward movement at entry.
  Co-op skips the meeting dialogue while retaining the statue and Sigma reveal.
- Dinorex retains each view's background, admits P2 to the flame corridor,
  holds the shared floor open for a living follower and provides two native
  Ride Armors. The large fire sheet appears in both affected panes, including
  when X died before Zero reached the corridor, and clears with the native fire.
- Existing transparent-effect batching substantially reduces the measured
  fire-effect cost on the loaded test host. This is not a stable60fps guarantee.
  The trusted overlay fallback remains selected to preserve the co-op hooks;
  the unqualified native-overlay cache is not included or enabled.

The remaining prepared door, lava, barrier and fire checks were accepted by
the owner with qualified recollection; their exact statements and automated
limits are retained in docs/COOP_X5_025_FOLLOWUPS.md. Final title tests pass25/25.
The older parity audit, full campaign/endings, physical LAN and native cold-save
unlock/duel persistence remain outside this release's qualification. Older
save states are build-specific; preserve native memory-card saves separately.

Local split-screen, Either-player doors, diagnostics and Alia suppression retain
their release defaults. P1 uses keyboard and P2 targets DualSense; choose the
connected device in the launcher if USB/Bluetooth changes. Normal window and
audio are enabled. Bring your own USA SLUS-01334 disc; OpenBIOS is included.
The package contains no disc, personal saves, recordings or obsolete QA states.
CD Speed and Fast Loading packages/controls are absent. Delivery is local to
Nextcloud; no remote release, push or PR is performed.

# v0.2.5-coop-alpha-debug (local Windows alpha)

This release combines game master and performance PR 14 with the accepted
v0.2.4 co-op work. It selects the audited native SDK frame-wait path and updates
the shared framework and launcher while retaining separate inputs, paired
cameras, diagnostics, native menu behavior and all accepted gameplay fixes.
Each local pane now keeps its own health bar throughout teleport-in and the
complete intro animation, including the interval between the heroes landing.

Original-disc overlay code is freshly regenerated and hash-audited for this
runtime. The portable package includes that cache; no compiler or disc is
included. Prepared intro, Grizzly and duel snapshots are recreated for this
exact build. Older snapshots are disposable; native campaign saves remain
separate. See docs/COOP_NATIVE_WAIT_RELEASE.md for qualification and measured
scope. Performance results describe the combined framework package.

The previous campaign, solo Grizzly pause and physical LAN gaps remain deferred.
Native cold memory-card Save/Load of the Black Zero capsule unlock remains
unverified. Full winner-selected stories/endings and native cold-save duel-winner
persistence remain unqualified. This release does not claim full campaign parity
or netplay compatibility with older builds.

Local split-screen, Either-player doors, diagnostics and Alia suppression retain
their defaults. P1 uses keyboard and P2 targets DualSense, with normal window
and audio settings. CD Speed and Fast Loading remain absent. Source integration
and delivery are local; no PR merge, push or remote release is performed.

# v0.2.4-coop-alpha-debug (local Windows alpha)

This build completes the latest eight opening-stage owner reports. Each split
view keeps its own lower-floor scrolling and personal effects, including Zero's
Z-Buster muzzle and wall-jump dust. Restarting from X to Zero in one process
retains the correct character, projectile and HUD graphics. Both heroes follow
the native white flash during the statue reveal and Sigma defeat.

- The companion continues native walking/dashing animation through Both-player
  boss doors, preserving the original door carry, scene and control recovery.
- Black Zero is available only after earning the Sigma stage 3 capsule unlock.
  Either seat can collect it; the native unit selector then offers Normal and
  Black Zero. Choosing Normal keeps the unlock. The Black Zero preference does
  not bypass collection, and X's palette remains intact when P2 Zero collects it.
- The campaign still fixes P1's hero, with one X and one Zero. Fourth Armor and
  Z-Buster remain available. Local split-screen, Either-player doors, diagnostics
  and Alia suppression remain selected/default on.

The eight reports have six bounded automated passes and two owner visual passes.
Both native capsule collection paths and locked/unlocked selector behavior pass;
native cold memory-card Save/Load of this new unlock remains unverified. Older
comparisons, solo Grizzly pause reproduction and LAN remain deferred. Full
winner-selected stories/endings and native cold-save duel-winner persistence
remain unqualified. See docs/COOP_X5_023_OWNER_BURNDOWN.md for exact scope.

P1 uses keyboard and P2 targets DualSense. Normal window/audio are retained;
devices can be changed in the launcher. Prepared QA snapshots are regenerated
for this build and use separate disposable saves. Native campaign saves remain
separate. Bring your own USA SLUS-01334 disc; OpenBIOS is included. CD Speed and
Fast Loading packages/controls are absent. No remote publication.

# v0.2.3-coop-alpha-debug (local Windows alpha)

This build completes the latest 11-report owner follow-up with bounded checks.
P2 can rescue native injured Reploids: the team receives one life and the
rescuer receives the native healing. Full P2 virus infections now retain their
shared progression counts; X and Zero keep their different original effects.

- Silent Alia hints retain their native completion actions without briefly
  holding controls or gathering the partner. Visible hints still gather and
  return safely; X's return retains its native armor/materialization layers.
- Intro building/lower backgrounds follow each view independently. The Both
  door path preserves the native roof carry, statue reveal, Sigma dialogue
  and control recovery. Companion boss-intro animation continues normally.
- Personal HUDs retain native frames, badges, lives and sorting when either
  hero overlaps them. X/P2 hover exhaust stays live, and the charged Crescent
  shield samples its original weapon graphics in either seat.

All 11 latest reports have bounded passes; this is not full campaign parity.
Older comparison work, the intermittent solo Grizzly pause and LAN expansion
remain deferred. Full winner-selected stories/endings are unqualified, and
native cold-save duel-winner persistence is not implemented.

Local split-screen, Either-player doors, diagnostics and Alia suppression
remain selected/default on. P1 uses keyboard and P2 targets DualSense, with
normal audio in a normal window. Native campaign saves stay separate from
build-specific disposable QA states. Bring your own USA SLUS-01334 disc;
OpenBIOS is included. CD Speed/Fast Loading are absent. No remote publication.

# v0.2.2-coop-alpha-debug (local Windows alpha)

This local release brings the latest owner-report fixes into the portable
co-op build. Fourth Armor's lingering charged orb now uses the correct
reflected source tiles and full-sized pieces, removing the seams through it.
Common scenery/effect palettes retain the original stage colors.

- Each seat uses the original unit selector in sequence. The campaign fixes
  P1's hero; P2 selects the counterpart's form. One X and one Zero remain,
  with Black Zero separately selectable and no replacement selector UI.
- P2 can quit from their pause menu through the native confirmation flow.
  Alia interactions gather a distant partner safely and preserve native stage
  actions. Intro descent camera bounds recover in the bounded regression.
- The cleared-stage Grizzly explosion countdown/GO cue is verified with
  default hint suppression. Native debris and moving-structure smoke retain
  their original palettes in both views.

The current 16-report follow-up has 13 bounded passes and three unverified
checks: damaged hovering-platform smoke, final intro Sigma arena camera/control,
and visible boss-door closure during that fight. These need one normal authored
intro route; incomplete teleport setups are not passes. The owner authorized
this release with those gaps recorded. See docs/COOP_X5_021_FOLLOWUPS.md.
The older comparative campaign, solo Grizzly pause reproduction and LAN
expansion remain deferred; full campaign parity is not claimed.

Local split-screen co-op and Either-player doors are selected. Diagnostics
and Alia hint suppression default on. P1 uses keyboard and P2 targets the
DualSense; devices can be changed in the launcher. Normal audio and windowed
mode are retained. Bring your own USA SLUS-01334 disc; OpenBIOS is included.
CD Speed and Fast Loading packages/controls are absent. Native campaign saves
are separate from disposable QA snapshots. No remote publication.

# v0.2.1-coop-alpha-debug (local Windows alpha)

This local co-op build includes the accumulated user-report fixes and the
owner-accepted playable X-versus-Zero defeat sequence. It defaults to local
side-by-side cameras, Either-player doors, diagnostic recording and Alia hint
suppression. X retains Fourth Armor availability and Zero retains Z-Buster;
X's player owns armor selection. Configure other devices or displays in the
launcher. The supplied local profile assigns P1 keyboard and P2 DualSense.

- Correct independent pause menus, cameras, HUDs, actor/effect graphics and
  scene ownership, including intro Sigma and Grizzly's trains, timer and door.
- Preserve native actions when suppressing Alia messages; health pickups no
  longer pause both players. Intro roof structures wait for every living hero.
- The duel has equal starting health, normal moves/personal menus, aligned
  meters and one real arrival. Only the loser loses their meter and crouches,
  then vanishes into the explosions during the white flash. Ordinary stage
  deaths retain their native animation. Physical input isolation is accepted.

The recent follow-up burndown is8 resolved,2 deferred,10 total. The older
Both-door intro death lacks a qualified normal-route reproduction. Solo
Grizzly's intermittent background pause was not reproduced in the bounded
scroll check. These findings remain open; see docs/COOP_X5_USER_REPORTS.md.
Default Either-player doors are retained. LAN is deferred by the owner.

Full winner-selected stories/endings remain unqualified, and native cold-save
duel-winner persistence is not implemented. Current-build save/load is separate
and validated; older snapshots are disposable. This is an alpha testing build,
with diagnostics, debug support and private setup commands for the supplied QA
recipes. It is not a full-campaign or competitive-balance certification.

Bring your own USA SLUS-01334 disc. Bundled OpenBIOS is included. CD Speed and
Fast Loading packages/controls are absent. Prepared offline QA states are
build-specific and keep native campaign saves separate. No remote publication.

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

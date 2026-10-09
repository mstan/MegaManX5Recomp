# X5 simultaneous X / Zero co-op

Tracking: central Beads `beads-eio.10.2`. Branch `spike/mmx5-coop`, created
from freshly fetched remote master `c8fde08` on 2026-10-08.

The approved sequence follows X4's original Codex plan, using X6's native
player-context projection and retained original-disc resources. P1's native
stage selection determines the character; P2 is the counterpart. Native Zero
story availability is the default, with an optional persistent counterpart.

1. Verify native X5 character, resource, input and shared-world boundaries.
2. Demonstrate two visible independently controlled and attacking actors with
   one world update per frame, then verify feature-disabled single-player.
3. Implement per-player combat/resources/contact state, survivor and team-wipe
   lifecycle, native menus/HUD, shared-camera bounds, pickups and scenes.
4. Add trusted unified-camera recomp-net delay sync and deterministic host-state
   checksums; qualify original-disc stage and two-peer behavior.
5. Deliver an isolated launcher-ready development build for owner validation.

Independent-camera netplay must wait for actual owner validation of this
unified baseline. A playable X-versus-Zero story encounter is optional later:
P2 controls the opposing actor, uses the native boss meter, and either winner
advances the normal post-fight script without changing campaign outcome.

No campaign or gameplay qualification is claimed by workspace setup or builds.
Original assets, captures, saves and generated code stay local and ignored.

## Native X5 boundaries

Original `SLUS_013.34` verifies body `8009A0A0`, shared PLAY `800D1C00`,
input `800C931C`, and the 32-entry attack pool `80098120` (stride `9C`).
The shared world at `80022D98` calls input `8003CDB8`, player `80035040`,
attack pool `80022084`, and terrain `8002FB58` separately. Native character
initialization is `8003BAC8`; `PLAYER+2` selects X/Zero, with armor separate.
Both characters' player code is resident. X6's character overlay exchange is
inapplicable: X5's BIN members identify stage actors, not player identity.

`PLAY+49` is a separate roster/parts selector (`8003D5E0..8003D614`),
and must also be projected: Zero is entry 5; counterpart X uses base entry 0.
Alternative P2 X armors are outside this first development checkpoint.
Original arrival calls `8003D5E0` before `8003B65C` at `8003553C/80035570`;
calling `8003BAC8` and forcing idle alone skips that native parts setup.
Arrival grace also sets native contact gate `PLAYER+A4` with timer `+61`:
`80038C54..80038C7C` expires the gate, and `80031968` checks it.

Rendering uses `80024C60`, with native ordering-table tails at `80091C50`.
DAT records 88/90 provide X/Zero graphics (member 3), assembly (member 9),
and palettes (member 0). The original decoder successfully read 461 X and
695 Zero frame entries. Native contact starts at `800317B8`; `800317B4`
is the previous function's delay slot. Native enemy damage is `80031DD0`.
Camera helpers `8002A720` and `8002A7E0` follow X and Y respectively.

Native roster construction in original BIN member 20 (`800EEE90..800EEED4`,
loaded at `800EE970`) includes Zero when `PLAY+79 < 5` or `PLAY+CB & 80`.
The local `keep_zero` option affects only P2 enrollment/availability; it never
writes those campaign flags. The trusted online profile currently fixes native
availability and ignores offline package choices.

## Reference review

The original X4 plan was recovered from the 2026-10-08 Codex session
`01a11d18-1248-71b0-9903-1f3c279c9b71` and compared with
`../_wt-mmx4-coop/docs/COOP_SPIKE.md` and its combat/lifecycle evidence.
X6's `../_wt-mmx6-local-coop/src/mods/mmx6_coop_plugin.c` supplied the primary
native-context, retained-resource and survivor reference. X5 addresses and
formats were then checked against its own original executable/DAT/BIN.

X6 review findings carried into this implementation include native SPRT_16
packet handling, final ordering-table tail retention, hit-pause ownership,
native death-effect origins, native menu texture restoration, and collector
ownership through delayed healing. X4 supplies per-seat persistent target hit
tokens, moving-solid history, and source-specific delay-sync identity/digests.
These are title-specific adaptations; no character overlay exchange is used.

## Current development checkpoint

The experimental package is staged only in debug-tool builds and defaults off.
Production single-player builds do not register the co-op netplay profile.
The isolated playtest staging tool enables the package only in its own output
directory, with private settings and memory cards. Original disc/executable
paths stay local. No owner checkout, personal cards, release, or remote branch
is modified.

Implemented: native two-body/input/attack passes; one shared enemy/world pass;
native independent health/ammo/HUD/menus; shared retry/tank storage; P2 native
Select withdrawal/return; survivor/team-wipe handling; independent target
tokens and moving-solid contacts; native pickup/heal ownership; script and
door owner/passenger transport; authored room/pit bounds and midpoint camera;
native Zero availability with local override; and unified 4:3 GL delay sync.
Rewind, rollback and savestates are blocked while the plugin is active.

The private profile hashes canonical host gameplay state (both saved bodies,
attacks/trails, lifecycle, inventory return, scene/death/pickup ownership,
contact histories and menu restoration state). It excludes host pointers,
padding, render caches and debug fixtures. Its source-contract fingerprint
also covers the relevant framework files and hook configuration. Retained
texture banks remain presentation resources; online VRAM authority stays at
software 1x while each peer may present through GL at its own resolution.

This remains an **early unified-screen checkpoint**, not a qualified full
campaign. Ordinary room transitions, pits, moving platforms, all Maverick
stages, boss defeats/rewards/return, late-story removal, vehicle ownership,
alternative P2 X armors and long-session texture capacity need qualification
before calling the full X4-derived baseline complete. Enemy AI retains its
native primary-world targeting while both seats are alive; the survivor owns
the native world when P1 falls. Independent cameras and optional story PvP
have not been implemented.

## Evidence and remaining stage matrix

All reports/captures below are ignored local artifacts under `build-coop`.
No automated result represents owner human acceptance.

| Path | Evidence | Scope |
| --- | --- | --- |
| X-selected fresh boot | `qa-baseline-x/report.json` | Original menus/dialogue, complementary bodies and correct native parts, independent movement/attacks, one counterpart pass per world pass |
| Native resource/scene fixtures | `qa-baseline-x/ownership/report.json` | P2 health pickup 11→27 with P1=13; shared tank16 consumed once for P2; HP/all16 ammo fields survive withdrawal and native script passenger transport |
| Both native death orders | `qa-candidate-local-zero/lifecycle/report.json` | Final candidate with P1 Zero/P2 base X: survivor control, once-per-world native timer, no fallen Select rejoin, retry2→1 and1→0, native team reload |
| Final X/Zero peer pairs | `qa-candidate-net-x/report.json`, `qa-candidate-net-zero/report.json` | Final candidate: private movement/attacks and both menus, matching core/mod CRCs through sampled ticks5888/4896; Zero case includes25ms±10ms jitter |
| Zero-selected fresh boot | `qa-candidate-local-zero/report.json` | Final candidate, complementary native bodies/parts and independent controls |
| Intro road/dialogue smoke | `qa-stage-x` and `qa-scene-x` captures | Reached native damaged-road dialogue; not an intro completion or boss qualification |
| Feature disabled | `qa-candidate-disabled/report.json` | Final candidate, native fresh boot and single-player controls, no co-op activation |
| Unit/integration checks | CTest assets, lifecycle, hooks, production catalog | Bounded decoder/format checks, lifecycle/availability rules, unique hooks and emitted coverage, production package count |
| Eight Maverick stages and late stages | Pending | No load/traversal/boss/vehicle matrix is claimed |

Keep the unsuccessful `qa-stage-x/report.json` selection-navigation timeout:
the unpaced test had already advanced into native gameplay when it timed out.
Shorter menu pulses corrected subsequent fresh-boot navigation. The explicit
enemy-HP-delta attempt in `qa-final-x2/combat.json` was inconclusive; an earlier
native P2 accepted-hit counter/visible saber capture is not boss qualification.
Retain `qa-final-x3` (the parts check caught the skipped arrival setup) and
`qa-arrival-x/ownership` (the harness tried closing the native menu during its
opening fade). The corrected harness waits for native menu interactivity and
compares inventory at the actual absence boundary: an alive actor can take
ordinary damage during the 90-update Select hold.

The final candidate executable SHA-256 is
`b7f7066d431cf9fa8730abfacc086076cea463ee77e9b572806e66297834cd38`.
The final corpse/passenger rendering adjustment is covered by the Zero
candidate's two death orders; the resource/script fixture report preceded that
presentation-only adjustment.

Build uses the local OpenBIOS image and framework `1483cc3`, UI `36cf0da`.
Generated C is regenerated from `game.toml`, never patched. Rebuild with two
compile jobs. Focused runtime commands are in `tools/test_mmx5_coop_*.py`;
they own private processes/cards and use native input for boot/navigation.
Controlled ownership/death fixtures are debug-only and reject online use.

Stage an owner test with:

```powershell
py -3 tools/prepare_mmx5_coop_playtest.py --build build-coop --out build-coop/playtest-unified
```

Run the generated `Play X5 Co-op.ps1`, bind two controllers in the launcher,
and use original stage/character menus. Start belongs to the requesting seat;
P2 holds Select for 90 gameplay updates to leave, then taps Select near a
grounded living P1 to return. Fallen actors wait for a native team retry or
new stage. Keep other enhancement mods off for this initial baseline test.

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

On 2026-10-09 the owner approved **unsplit/shared-screen netplay as an opt-in
feature**. Host / Join explicitly starts the existing trusted co-op delay-sync
profile; offline play requires no network session. The package defaults off
outside its isolated playtest build. Positive early playtest feedback does not
qualify the full campaign or unlock the independent-camera stretch goal.

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
Native stage loading includes modes8 and9 (`203F0`, player initialization at
`204E4`) before gameplay mode10. Crossing either loader state preserves the
continuation classification; leaving for title/stage selection clears it.
The saved-inventory discriminator is **signed `PLAY+1E == -1` (`FF`)**:
`8003BBA8` loads `v0=-1` before the comparison at `8003BBD0`, and native
`80020794..80020818` saves HP/ammo/weapon/virus before choosing mode9.
Value1 is an arrival/effect flag and does not select that inventory path.
The earlier checkpoint checked1 incorrectly; controlled native continuation
now verifies living, absent and fallen P2 across original init.

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
native independent health/ammo/HUD/menus, with meters stacked on the left; shared retry/tank storage; P2 native
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
stages, boss defeats/rewards/return, late-story removal, Ride Chaser traversal,
mounted Armor traversal/doors, alternative P2 X armors and long-session texture
capacity need qualification
before calling the full X4-derived baseline complete. Enemy AI retains its
native primary-world targeting while both seats are alive; the survivor owns
the native world when P1 falls. Independent cameras and optional story PvP
have not been implemented.

Counterpart enrollment now starts the original character-specific teleport
arrival, matching X4/X6's stage-entry lifecycle. X5 `8003B33C` selects its own
X/Zero incoming/resolving parts (arguments1/2); original `80017248` advances
them before `8003A670` restores idle and native contact grace. Withdrawal
already uses arguments3/4 through the original departure loop and action/sound
helper `8003B28C`. Fresh stage/retry and living continuation arrivals animate;
retained absent/fallen seats stay out. Forced car arrival remains native
`8003D784`. Inspected captures show Zero's descending red beam/resolving pose
and outgoing pose/upward red beam, including a save-loaded Maverick stage.
`qa-native-spawn-x/continuation` retains a harness failure that held Select
while the counterpart was still JOINING. The corrected arrival-aware check
waits for native teleport completion before requesting withdrawal and passes
in `continuation-arrival`; elapsed arrival time is not a despawn request.

Ride Armor is **P1-only** by owner preference. Original BIN member6 (logical
ID10, stage4/section1) supplies `800F2A4C`; resident `80022960` dispatches the
two original world slots (`800D1DC0`, `800D1E70`, stride `B0`). During P2
survivor/script world ownership, only that native dispatcher temporarily uses
P1's body/resources, preserving original vehicle/effect pointers and one world
update. Original overlay ID and instruction guards prevent applying it to
unrelated code at those VMAs. P2 stays on foot. Native Ride Chaser co-op remains
undergoing qualification; the Armor policy does not qualify that separate
forced-vehicle stage.

The owner also requires **no voluntary despawn anywhere in the forced car
sequence** and **one rider death must leave the surviving rider in the
sequence**. Original `80020968..80020984` and X4's Marine Base gate select
stage5/checkpoint0 even before mounting; the explicit leave lock also covers
an active native car after that checkpoint changes. Both riders receive a
private `B0` native car context and the original `8003D784` rider arrival.
The counterpart enrolls during the scripted arrival instead of waiting for
ordinary idle pose2 (a native rider uses pose12). Original `3D784` activates
the vehicle during the player pass, after that frame's `22960` dispatcher;
enrollment waits for the next initialized native car rather than copying an
empty slot. Its input, native `22960` vehicle pass, body, attacks and terrain
advance once at the existing private
player boundary; the shared world still advances once.

Car rendering uses the original stage atlas, with the native character CLUT
choice from BIN7 `800EED30..800EED40`. Native moving-solid car histories
(`+73/+77`) are independent. Verified following-source constructors at
`800EFEC8`, `800F0BE8`, `800F0D64` store `+50`; bounded native-pool ownership
redirects only the exact original car pointer to stable per-seat mirrors.
Private native attack-pool arrival effects retain their own car projection.
`800EF05C..800EF098` applies the authored camera delta to a car, so this
sequence uses the native camera controller throughout. The saved cars and
source owners are included in the delay-sync digest.

Controlled car rules pass in `qa-palette-survivor-x/sequence-drive` and
`qa-palette-survivor-zero2/sequence-drive`: Select
stays locked for 180 world updates, either first death leaves the other rider
alive with its own car, and the last death consumes one native shared retry
(2→1,1→0). The control check waits for both native grounded driving mode1
and verifies an actual upward jump response by the survivor. Arrival mode0's
automatic motion alone is not treated as independent control. Full road
traversal remains pending. Feature-disabled
`qa-chaser-stock/car.json` also reaches the opening pit when unattended;
that stock comparison is not co-op failure evidence. Early car tests
(`qa-chaser-bringup-x`, `qa-chaser-paced-x`, `qa-chaser-title-x`,
`qa-chaser-camera-x`, `qa-chaser-arrival-x`) retain their failed control
windows/loader observations. They do not qualify road traversal or survivor
orders. New checks use full original stage-entry5/6 and loader7/8/9,
require a new living enrollment, and observe the native arrival itself.

Both native HUD blocks keep their geometry and original character colors.
P2 moves down104 pixels at the packet boundary, leaving shared life digits and
boss meters in their original positions. The existing native virus/radar
sprite block moves48 pixels right to clear the lower meter in Maverick stages.
The owner's requested layout is P1 above P2 on the left. Native menus still
belong to the requesting seat.

Survivor image review caught a shared-palette bug in the Armor stage: original
`56514` caches world palette pointers, `2BD8C` builds a whole-stage fade from
the `4000`-byte palette, and `17484` uploads it. Projecting a character-only
palette for P2's entire world corrupted the background. Shared world passes
now retain the native stage palette; P2 body/attack passes temporarily select
its private colors. `qa-palette-survivor-x/armor` passes P1 controls/dismount,
P2 staying on foot and survivor ownership, and its inspected survivor capture
shows intact lava/background and native death orbs. Earlier corrupted captures
remain in `qa-stacked-car-zero2/armor` and `qa-stacked-final-x/armor-entry`;
their control checks alone did not qualify presentation.

## Evidence and remaining stage matrix

All reports/captures below are ignored local artifacts under `build-coop`.
No automated result represents owner human acceptance.

| Path | Evidence | Scope |
| --- | --- | --- |
| X-selected fresh boot | `qa-palette-survivor-x/report.json` | Latest source: original menus/dialogue, complementary bodies and correct native parts, independent movement/attacks, one counterpart pass per world pass |
| Native resource/scene fixtures | `qa-baseline-x/ownership/report.json` | P2 health pickup 11→27 with P1=13; shared tank16 consumed once for P2; HP/all16 ammo fields survive withdrawal and native script passenger transport |
| Both native death orders | `qa-candidate-local-zero/lifecycle/report.json` | Earlier candidate with P1 Zero/P2 base X: survivor control, once-per-world native timer, no fallen Select rejoin, retry2→1 and1→0, native team reload |
| Final X/Zero peer pairs | `qa-palette-net-x/report.json`, `qa-palette-net-zero/report.json` | Latest source: private movement/attacks and both menus, matching core/mod CRCs through sampled ticks6011/4935; Zero case includes25ms±10ms jitter |
| Zero-selected fresh boot | `qa-palette-survivor-zero2/report.json` | Latest source: actual paced native Zero selection, stacked HUD, complementary base X parts and independent controls |
| Native continuation/resource checks | `qa-stacked-car-zero2/continuation/report.json`, `qa-stacked-car-zero2/ownership/report.json` | Native FF continuation preserves independent HP/ammo and absent/fallen P2; pickups/tanks/Select return/script ownership pass |
| P1-only Armor and survivor palette | `qa-palette-survivor-x/armor/report.json`, `qa-palette-survivor-zero2/armor-controls/report.json` | Native entry/load, P2 overlap stays on foot, P1 controls/dismount, survivor routing and inspected original stage art; Zero case also checks P2 survivor input |
| Car sequence rules and driving | `qa-palette-survivor-x/sequence-drive/report.json`, `qa-palette-survivor-zero2/sequence-drive/report.json` | X and Zero selected: native entry/load, Select180 lock, both first-death orders, surviving native jump, one timer/world pass and one retry per team wipe; not road traversal |
| Intro road/dialogue smoke | `qa-stage-x` and `qa-scene-x` captures | Reached native damaged-road dialogue; not an intro completion or boss qualification |
| Feature disabled | `qa-stacked-final-disabled/report.json` | Stacked/vehicle checkpoint before the enabled-only palette fix: native fresh boot and single-player controls, no co-op activation |
| Unit/integration checks | CTest assets, lifecycle, hooks, production catalog | Bounded decoder/format checks, lifecycle/availability rules, unique hooks and emitted coverage, production package count |
| Native stage arrivals | `qa-native-spawn-x/report.json`, `qa-native-spawn-zero/report.json` | Both character selections and original incoming parts, independent control after arrival; Zero's red beam and resolved actor inspected |
| Arrival-aware resources/continuation | `qa-native-spawn-x/ownership/report.json`, `qa-native-spawn-zero/ownership/report.json`, `qa-native-spawn-x/continuation-arrival/report.json` | Native pickup/tank/withdrawal/return/script checks; native FF living/absent/fallen continuation |
| Arrival checkpoint netplay | `qa-native-spawn-net-x/report.json`, `qa-native-spawn-net-zero/report.json` | Matching core/mod samples through ticks5623/4879; Zero pair includes25ms +/-10ms jitter; both actors and menus exercised |
| Stage-access card | `qa-stage-access-complete/report.json` | Original Continue/DATA1/Yes loads, eight native cursor choices map to stages1..8, both characters available, native Grizzly stage entry with both living actors; no stage traversal/boss qualification |
| Zero departure image review | `qa-stage-access-ready/departure/report.json` and captures | Original P2 Select request, outgoing Zero pose and upward red beam, absence and native return |
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
The unpaced selector observation can still race a native default confirmation
(`qa-staged-loader`). The harness now accepts an already-confirmed
original-input path only after verifying the
requested character from the actual native body; it never forces a selector.
`qa-palette-survivor-zero` correctly rejected an already-confirmed X pair;
the fresh paced `qa-palette-survivor-zero2` verified actual Zero selection.
`qa-palette-survivor-zero2/armor` retains the loader harness timeout caused by
its strict idle-pose gate and repeated Cross pulses. The corrected gate waits
for fresh enrollment and open native control, then leaves input neutral.

The previous candidate executable SHA-256 is
`b7f7066d431cf9fa8730abfacc086076cea463ee77e9b572806e66297834cd38`.
The final corpse/passenger rendering adjustment is covered by the Zero
candidate's two death orders; the resource/script fixture report preceded that
presentation-only adjustment.
The subsequent loader-mode9 classification fix is in playtest SHA-256
`8aeb28e12233debd891646ab8e029e5722f04d33c2559c5471da91b5c16c6819`.
Its X-selected private pair passes in `qa-loader-net-x/report.json`, with
matched core/mod samples through tick5984. Ordinary section traversal remains
pending; the loader fix does not turn this input/CRC check into that evidence.

Build uses the local OpenBIOS image and framework `1483cc3`, UI `36cf0da`.
The latest stacked-meter/vehicle/palette checkpoint executable SHA-256 is
`500840386f7d42976aab48fb39f5a6bd10e31ad3832a68b8f8a5b05064665ec3`.
The subsequent native-arrival checkpoint executable SHA-256 is
`d7c5fd5f3408c802ebff5b625062f48eb0db6174590ea557a44c8ddeaa5fff61`.
Generated C is regenerated from `game.toml`, never patched. Rebuild with two
compile jobs. Focused runtime commands are in `tools/test_mmx5_coop_*.py`;
they own private processes/cards and use native input for boot/navigation.
Controlled ownership/death fixtures are debug-only and reject online use.

Stage an owner test with:

```powershell
py -3 tools/prepare_mmx5_coop_playtest.py --build build-coop --out build-coop/playtest-native-arrival --stage-access-card build-coop/stage-access-ready/card1.mcd
```

Run the generated `Play X5 Co-op.ps1`, bind two controllers in the launcher,
and use original stage/character menus. Start belongs to the requesting seat;
P2 holds Select for 90 gameplay updates to leave, then taps Select near a
grounded living P1 to return. Fallen actors wait for a native team retry or
new stage. The forced car sequence locks voluntary withdrawal; one rider death
keeps the other playing, and the last death spends one shared retry. Keep other
enhancement mods off for this initial baseline test.

The next-test card is a separate **stage-access fixture**, with all eight bosses
unbeaten, both characters available, 16 hours remaining and native starting
equipment. Choose CONTINUE, Memory Card1, DATA1, then Yes. The staging tool
seeds a fresh directory's root and OpenBIOS-isolated cards; it refuses existing
output directories and never replaces previous playtest cards.

`tools/prepare_mmx5_stage_access_save.py` reproduces the original BIN24 slot
serializer `800F2970..800F2BF0`, `158`-byte slot and resident checksum validator
`8001DB78`, using a fresh offline native PLAY snapshot. The card filename is
`BASLUS-01334`, with original title/palette/three icon frames. The fixture sets
the native post-intro phase1, one completed opening mission, and the original
`FF` empty pending-story queue. Resident `8001DCC8` otherwise resumes a queued
story event instead of stage selection. Initial queue0 cards were recognized
but reentered the intro (`qa-stage-access-save`, `qa-stage-access-post-intro`);
these observations remain unqualified. The corrected card SHA-256 is
`4a173ef3b348f61073a89b8e416687f556b44b7475ef60c0e3564a0ac093f3e9`.

`tools/test_mmx5_stage_access_save.py` uses original input and private copied
cards, with no gameplay RAM writes or savestates. It waits for the interactive
eight-portrait selector, visits each native choice, checks Zero availability,
and enters one selected boss stage with both actors. Original selector
`800EF858` owns PLAY+28; original BIN20 `800F5050` maps those eight portraits
to stages1..8. A load-mode transition alone is not this validation. Native
briefing/dialogue still runs when the card is loaded.

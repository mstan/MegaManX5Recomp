# X5 simultaneous X / Zero co-op

Current local X4-parity work: [burndown](COOP_PARITY_BURNDOWN.md), branch
`feature/mmx5-x4-parity-local`, issue `beads-eio.10.6`. Alpha acceptance and
release instructions below are historical and do not qualify the current
candidate or authorize publication.

Tracking: central Beads `beads-eio.10.2`. Branch `spike/mmx5-coop`, created
from freshly fetched remote master `c8fde08` on 2026-10-08.

Release assessment resumed on 2026-10-09: the owner accepted the independent
performance fix and Grizzly reward/return result, and waived a boss repeat.
See `COOP_DEBUG_TESTING.md` for the remaining bounded checks and packaging work.
The synthetic stage-access card and its generator described below are
historical evidence only. Future test saves must come from an online download,
with the original preserved and any edits kept sparing. Do not reuse the
generated card in the staging/launch commands below.

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

The owner approved moving to independent displays after the unified human
playtest on 2026-10-09, while reporting Zero's Ride Chaser pose/facing defect.
That explicit approval supersedes the earlier independent-camera gate. A
playable X-versus-Zero story encounter is optional later:
P2 controls the opposing actor, uses the native boss meter, and either winner
advances the normal post-fight script without changing campaign outcome.

On 2026-10-09 the owner approved **unsplit/shared-screen netplay as an opt-in
feature**. Host / Join explicitly starts the existing trusted co-op delay-sync
profile; offline play requires no network session. The package defaults off
outside its isolated playtest build. Positive early playtest feedback does not
qualify the full campaign. The subsequent human approval above authorizes
independent displays; its new build still requires its own owner playtest.

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
the native world when P1 falls. Optional story PvP has not been implemented.
Independent online displays now use the native camera and shared-world
implementation described below; general native enemy/shot targeting selects
the nearest eligible actor in that mode. Unified targeting stays native.

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

Owner image review found that the counterpart rider kept its old teleport
parts and faced backward. Original `8003D784` sets rider+6=1; the original
BIN7 car initializer `800EEC50` normally clears it and selects the mounted
pose. Copying an initialized car bypassed that latter rider step. Enrollment
now borrows the actual car facing/current native rider action and clears+6,
letting `800377A0/80037854` select each character's original mounted parts.
`qa-chaser-pose-fixed-x/sequence` passes both fatal orders, survivor jump,
Select lock and shared retry; inspected Zero frames16/17 face40 with the car,
instead of face0 and the expired teleport frameFF. The subsequent
Zero-selected `qa-independent-offline-zero/sequence` also passes both fatal
orders and the strengthened native pose/facing check, with inspected original
mounted X/Zero art. Saving Split as an offline preference still uses Unified.

## Independent online displays

Tracking: central Beads `beads-eio.10.3`. **Netplay cameras: Unified / Split**
is an opt-in choice, with Unified first/default. Offline always uses Unified.
The trusted profile accepts only its declared co-op settings feature; room
agreement adopts the host's choice and fingerprints the canonical options.
Direct LAN peers must choose the same option. Native 4:3 OpenGL delay sync,
independent inventory and the original shared retry/script rules remain.
The framework pin advances only to `8fff59f`, the direct child of `1483cc3`
already used by X4, for trusted settings agreement and isolated render-pass
transactions. This repin does not change the codegen version.

Each seat owns a native `FC`-byte foreground/parallax camera record. Original
`2A440`, `2A4E4`, `2A554` update those views; P2's private controller uses its
own layer origins. Split removes the mutual horizontal tether while retaining
the original authored left/right/pit bounds for each seat. Native `5609C`
camera areas use X5's fixed sixteen-byte record (`55F60`, FFFF preserves a
bound), and `56AA0` checkpoints select the eligible participant. Shared
script/door acquisition adopts the owner's camera; absent/fallen seats watch
the owner/survivor. Forced Ride Chaser scrolling remains the one authored
camera, with private native rider control and a HUD for each online seat.

Original `2AE50/2B070` scan both complete activation rectangles, rather than
the bounding box across the gap. Their native pools/placement flags remain
shared. A successful-allocation latch at `2B39C` records visits; kills cannot
immediately respawn during a stationary scan, leaving both regions restores
native re-entry, and allocation failure stays retryable. Supplemental scans
admit ordinary native enemy/item categories; scripted allocations stay with
the original directional scans. Native `2D800/2D938` lifetime and
`2D89C/2D9EC/2DAA0` draw predicates retain actors visible to either seat.

Original enemy and world-shot pools `21F64/221B0` execute one callback per
actor. Their inspected `22044/22290` dispatch instructions select the nearest
eligible full player context, with a stable P1 tie-break and each seat's own
CC hit pause. Native collision/solid/pickup/door boundaries temporarily restore
the canonical two-body context. The nearby actor's character resource pointers
are projected too, because native grabs/status changes can select player parts.
Campaign selection and the whole-stage palette stay with the shared world.
Camera records and successful visits join the
canonical co-op digest; local port selection never controls simulation.

At the completed native frame (`68C28`, caller11E50, buffer pointer9E794),
each peer draws its local seat through `psx_mod_render_local_view`. Original
`272B4` produces the scene and native HUD. The background producer `27F88`
resolves the complete authored map with the common Capcom renderer, replacing
the canonical camera's tile ring for that local pass. Original heads93F78 /
tails91D60, three layers, seventeen page buckets, parity stride198 and
CLUT7900 were verified from `27CD8/281C0`. Native menus/story scenes keep their
shared presentation. The local view gets one native meter at its original
position; Unified retains the stacked meters.

RAM, CPU, scratchpad, GPU/device state and authoritative VRAM are restored by
the sandbox, and the plugin restores its private host contexts. Local retained
sprite/UI banks have separate IDs/caches, preserving canonical bank numbering.
Reports require committed seat views, enabled restoration checks, no host-state
leak, and zero watchdog/device-read/VRAM-leak/restoration mismatch counters.
Inspected images show both original character HUDs and intact distant stage art.

Native two-peer evidence: `qa-independent-separation-x5` reaches camera origins
1/501 with both living actors, native nearby-P2 enemy updates, then overlaps
and rejoins at camera0/0. Matching core/mod samples extend through tick6974,
including both menus. `qa-independent-zero-jitter` uses actual Zero selection
with25ms +/-10ms jitter; native withdrawal switches P2's display to Zero's
survivor view, native return restores X's own HUD and private HP/all16 ammo
fields, and both menus restore with matching digests through5438. These are
bounded intro/selection/view checks, not every actor family, every room, boss
handoff, or full campaign qualification. Live Internet lobby play and the new
independent mode's owner playtest remain pending.

Retain unsuccessful own-view bringup (`qa-independent-close-x`, recursive
background callback guard), simultaneous Zero saber/movement control window
(`qa-independent-separation-x`), native truck/rejoin input windows (`x2/x4`),
and unpaced selector-observation race (`x3`). Corrected native-input checks
separate movement from grounded saber, jump original obstacles, allow enough
time to rejoin, and accept an already completed default X selection only after
verifying the real enrolled characters. No online RAM writes or stage forces
are used. The final X source-contract/withdrawal/separation check is recorded
in `qa-independent-final-x`: native absence/return, distant separation,
overlap/rejoin and both menus pass with matching core/mod samples through7331
on the final candidate executable. Zero's jitter run uses that same executable.

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
| Independent displays | `qa-independent-separation-x5/report.json`, `qa-independent-zero-jitter/report.json` | Native separate/overlap/rejoin cameras, shared enemy pools/nearest-player callbacks, per-seat HUD, verified local render sandbox, native absence/return and25ms +/-10ms jitter; bounded intro evidence |
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
The independent-display candidate uses framework `8fff59f`, with the same UI.
The latest stacked-meter/vehicle/palette checkpoint executable SHA-256 is
`500840386f7d42976aab48fb39f5a6bd10e31ad3832a68b8f8a5b05064665ec3`.
The subsequent native-arrival checkpoint executable SHA-256 is
`d7c5fd5f3408c802ebff5b625062f48eb0db6174590ea557a44c8ddeaa5fff61`.
The independent-display candidate executable SHA-256 is
`9a88e837bbd4e46a7f019acd3b3633a2e31206984930b35731a1fc2be5b747f0`.
Its final native-character resource projection passes the X separation,
absence/return, rejoin and menu matrix in `qa-independent-resources-x` with
matching core/mod samples through7816. Earlier independent-view reports
above precede that resource-scope correction and retain their own identities.
`qa-independent-resources-zero2` passes the same native separation/rejoin
matrix, both menus, spectator fallback and resource-preserving return with
actual Zero selection and25ms +/-10ms jitter, matching core/mod through6568.
Its preceding `qa-independent-resources-zero` report retains a harness error:
ordinary native enemy combat killed P1 before a check requiring his menu.
Menu qualification now occurs while both actors are alive, before traversal.
`qa-independent-final-unified` verifies the default shared camera and both
menus through5832 before the Split-only native-resource projection correction.
Generated C is regenerated from `game.toml`, never patched. Rebuild with two
compile jobs. Focused runtime commands are in `tools/test_mmx5_coop_*.py`;
they own private processes/cards and use native input for boot/navigation.
Controlled ownership/death fixtures are debug-only and reject online use.

Stage an owner test with:

```powershell
py -3 tools/prepare_mmx5_coop_playtest.py --build build-coop --out build-coop/playtest-independent-displays --stage-access-card build-coop/stage-access-ready/card1.mcd
```

Run the generated `Play X5 Co-op.ps1`, bind two controllers in the launcher,
and use original stage/character menus. Start belongs to the requesting seat;
P2 holds Select for 90 gameplay updates to leave, then taps Select near a
grounded living P1 to return. Fallen actors wait for a native team retry or
new stage. The forced car sequence locks voluntary withdrawal; one rider death
keeps the other playing, and the last death spends one shared retry. Keep other
enhancement mods off for this initial baseline test.

For independent online displays choose **Mods -> X + Zero Co-op -> Netplay
cameras -> Split** before hosting; Unified is the default. The room host
chooses for both peers, while direct LAN launches need matching choices. Bind
the local controller as P1 on each peer; netplay assigns its logical seat.
Offline always uses Unified, including when Split is saved as a preference.

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


## Performance follow-up

Tracking: central Beads `beads-eio.10.4`. Owner reports independent X5 plays
well, with substantial slowdown in Izzy Glow's fortress (third left portrait,
native stage6), especially as the players separate. Audio was not observed.

The original paired launcher inherited `PSX_RENDER_PASS_VERIFY=1` and
`bios_hle=false` from the qualification harness. Full restoration hashing
costs about 17ms per local view. Human launches now disable that optional
verification and preserve the staged title's normal BIOS HLE setting. The
restoration sandbox, device guards and peer state checks remain active;
qualification still enables the expensive hashes by default.

Player/camera/pool projections also used one runtime call per byte, including
every nearest-player scope. `mmx5_coop_memory` copies aligned sections through
runtime word reads/stores, retaining byte prefixes/tails and the normal host
store/invalidation path. A native camera now uses63 accesses rather than252.
The memory test checks every host/guest alignment, lengths0..513, adjacent
bytes and the complete payload, with assertions enabled in Release builds.
Guest clock, timers, DMA, CD, SPU and the native update rules are unchanged.

Native fortress entry samples use original Continue/selector input and private
matched cards, never online RAM writes or savestates. On the previous9a88
runtime, QA verification gives about26Hz and17ms local passes. Without that
verification, the LLE sample gives46.5Hz; the normal HLE byte-copy sample gives
39.8-40.1Hz and96-98% CPU per peer. The word-copy candidate47c52e26 gives
59.8-60.1Hz and70-74% CPU, with local passes around1.1-1.25ms. Each window has
both participants alive and matching peer core/mod samples. These numbers
qualify the entry window, not a full fortress traversal.

Ape Escape and X4 use the shared gameplay-gated `guest_cycle_scale=2`.
X5's diagnostic scale2 overlap sample did not improve the measured rate
(about44.4-44.9Hz with LLE and byte copies). No cycle scaling is enabled here.
The measured host costs offer a fix while retaining original guest timing.

`tools/measure_mmx5_coop_performance.py --overlap-only` measures a selected
native Maverick entry; its default portrait2 is the fortress. Automated
fortress traversal attempts hit the authored overhang/crusher, stalled or
killed P2 and are retained as failures. They do not qualify distant fortress
performance. Native intro separation/rejoin remains the controlled distant
view fixture, with original input, enemies, both menus and peer digests.

`tools/launch_mmx5_coop_netplay.py --build <staged-directory>` starts a fresh
visible LAN pair with Split selected, matching stage-access cards, P1's Sony
controller and P2's keyboard. It navigates the original Continue/DATA1/Yes
menus to the interactive eight-boss selector, validates the staged hashes and
releases its temporary host overrides before handing over. The companion
handoff verifies the owned executables before arranging/focusing the windows.
It preserves prior playtest directories and cards.


The same controlled intro trip passes on the previous byte-copy executable and
the word-copy candidate: overlap stays about60Hz in both; distant views improve
from49.9-50.0Hz to59.9-60.1Hz. Candidate camera origins1/501 retain distinct
native art/HUD/enemies, rejoin normally and pass both menu-owner checks. Core
and mod digests match through8272 (`qa-perf-intro-word`), compared the retained
byte baseline through8391 (`qa-perf-intro-byte`). These measured runs use paced
hidden SDL windows and no restoration hashing, matching human performance
settings. The verifier-on Zero/jitter regression separately qualifies sandbox
restoration and inventory/lifecycle behavior.


Final `qa-word-zero-verify-jitter` passes actual Zero selection, native P2
absence/spectator/return with unchanged HP/all16 ammo, both menus, distant
native cameras/enemies and rejoin with25ms +/-10ms jitter. Core/mod samples
match through6451. Restoration checks are active and mismatch, watchdog,
device-read, VRAM-leak and private host-state leak counters remain zero.
All8 CTests, strict debug/release-guard C and modified Python checks pass.
Human fortress separation and wider stage/room/actor-family performance
remain for owner review; no full campaign performance guarantee is inferred.

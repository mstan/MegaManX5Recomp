# X5 co-op parity with X4

The current focused owner candidate is **82A36EAA**, local only.
[Grizzly's individual stage list](COOP_X5_GRIZZLY_BURNDOWN.md) is
**8 passed / 1 open / 9 total**. The owner accepted the co-op main-cannon
and camera check; solo hidden-Alia cannon destruction remains manual.
[X-owned armor selection](COOP_X5_ARMOR_SELECT.md) is **5 passed / 2 open / 7**:
both seat assignments, Fourth/Z-Buster bonuses, native forms and snapshots are
qualified; Gaea projectile visual acceptance and the actual LAN armor-menu
interaction remain open. Gaea attack art, Zero's Z-Buster and airborne dash
exhaust are fixed locally with actual composed-frame evidence. The new health
pickup freeze check passes native delayed healing and independent partner
movement on both seat projections. Those four individual follow-ups are in
[COOP_X5_POST_GRIZZLY_QA.md](COOP_X5_POST_GRIZZLY_QA.md): **1 passed / 3 fixed
awaiting owner visual retest**. Q03/R9 and A05 keep their original entries.
The combined candidate passes24 CTests, three native item cases, the three
actual graphics checks and six matched local peer checkpoints. Physical LAN
remains separate; no pass is inferred from local transport.
These focused lists do not change the frozen234-comparison denominator or
claim full parity. Current fixtures and owner steps are in
[COOP_MANUAL_QA_HANDOFF.md](COOP_MANUAL_QA_HANDOFF.md).

Previous manual QA candidate **084830CA** fixed opening Sigma's head disappearing from P2's private view and kept X's base/face/armor together when Zero overlaps him. The original33E9009F recording and that presentation fix both verify all22,145 complete native input intervals, including original controller-read order and state CRCs; fifteen composed captures cover both attempts, combat, retry and actual victory. Native head flashing and independent cameras remain intact. The owner's normal-route video also confirms intact statue art and reveal. Owner clarified that Zero cuts across X's face: draw P2 before P1's native base/armor sequence, keeping the existing sort buckets. The original first-attempt X death with Both doors is separate and remains visible in both recordings. Duel polish/default diagnostics/hint suppression/Either doors are retained. General automated burndown remains stopped: **163 passed, 2 inapplicable, 4 failed, 65 unverified; 69 open of 234.**

Owner request: 2026-10-19. Local issue `beads-eio.10.6`.
Worktree: `F:/Projects/psxrecomp/_wt-mmx5-x4-parity`.
Branch: `feature/mmx5-x4-parity-local`. No remote publication or tracker sync.

Current non-duel sweep (2026-10-23): **163 passed, 2 inapplicable, 4 failed, 65 unverified; 69 open of 234.**

## Opening Sigma visibility report (2026-10-23)

Owner33E9009F archive `X5 Opening stage boss fight bugs.zip` matches execution
5ca1047f and records actual split/Both doors/keepZero/separate tanks/boss
scaling choices. Preserve the original archive, native cards and recording.
Original replay verifies22,145 complete native intervals with CRC and native
controller-read-order checks intact. Seventeen passive captures and the video
show Sigma present in P1 and absent in P2 after retry, includingN15508/17008.
The first attempt also contains an original X death; the later retry has both
heroes alive, so it does not explain the P2-only visibility failure.

U12 / beads-eio.10.19: Sigma uses native background layer1. F7EF0 publishes
visibility at CAMERA57; F9B20 writes the animated atlas origin at5E/62.
F8534/F8538 configure the layer flag/foreground attachment. A retained P2
mode2 camera instead retains spatial4460/192 while the real head uses772/356.
The guarded fix projects only these five authored fields inside the existing
private RAM/VRAM draw transaction. It preserves each foreground camera and
history, original map animation, collisions, controls and native world timing.
No new simulation/snapshot state or instruction hook is needed.

CE9B51FB verifies the same22,145 original state CRCs and controller reads:
only its diagnostic build header is rebound for this disclosed presentation
comparison. It is not a cross-build netplay compatibility claim. Fifteen
composed captures now show the head in both panes, with original head flashing
and distinct private camera positions. There are no observed renderer aborts,
watchdog incidents or VRAM leaks. All23 CTests pass. The final084830CA
receipts also cover the seven-checkpoint peer regression, fresh fixture
cold load and one actual32HP duel entry after the ordering fix.

Owner clarifies the separate priority symptom: Zero goes over X's face.
Native24698 paints P1's base sprite, then27370/26CAC draws X's armor.
The old co-op hook appended the whole partner between those native calls,
splitting X's face/body from his armor inside the same sort bucket.084830CA
paints the partner before P1's base so the original base/armor sequence stays
together. Actor priorities, native buckets, foreground/enemy/HUD order and
solo behavior remain unchanged. This follows the X4 lesson to keep a hero's
rendered parts together; it does not reserve speculative new priority bands.
The final combined candidate repeats the original22,145-interval recording,
the23 CTests and ordinary seven-checkpoint peer regression. Composed overlap
captures qualify this reported X/P1-Zero/P2 case; no blanket ordering claim
for every vehicle/effect follows. The first-attempt Both-door X fatal remains
a separate untriaged finding, not claimed fixed by these draw-only changes.

U11 / beads-eio.10.17 now has the previously missing normal-route art evidence:
the owner's original33E9009F video and exact replay show the intact statue at
N10397, subsequent explosions and native Sigma emergence. The earlier camera
command fix remains unchanged. This is evidence from the authored route,
not the earlier bypassed atlas setup.

Evidence: `G:/Temp/Codex/psxrecomp/x5-parity/owner-intro-sigma-20261023`,
including `owner-evidence-receipt.json`, `sigma-native-source.txt`,
`layer1-differential.json`, `layer-replay-qualification.json` (Sigma-only CE),
`combined-replay-qualification.json` (final084830CA),
`baseline-observer/`, `current-replay/` (Sigma-only), `combined-replay/`,
`paired-combined/report.json` and
`duel-cold-check/report.json`. The original baseline driver's unsupported
GPU-status query prevented its own images; the separate passive observer
captures are the retained visual evidence. Original replay CRC completion
is separately recorded. No frozen234 comparison status changes.

## Duel owner feedback and replacement candidate (2026-10-23)

Owner reports an initially oversized/misaligned boss meter, lingering Zero
aura and a visible NPC followed by a second player arrival on 6987087D.
Owner approves equal starting health, no healing or damage-scaling bonuses,
normal moves and native menus, then requests replacement of the local QA build.

33E9009F hides only the retained native shell and its owned intro children.
The real participant arrives once at the original grounded anchor during
the native introduction; controls release when the original introduction
actually completes. The original actor remains available for story logic.
Both native NPC/entrant paths pass, including retirement of Zero's two
class24 aura actors. No real player's effects are cleared. Captures confirm
aligned bar/fill/frame/level text and no stationary aura after movement.

Actual entry normalizes current health to32 each. Display-only maxima are
scoped to the native HUD call; earned32/34 campaign maxima remain unchanged.
Actual eligible native collisions use the bounded PvP profile and retain
native recovery/token/disposition. Difficulty, armor and parts cannot scale
the accepted injury. Actual X basic, Zero opening slash and X full charge
deal2/3/5HP respectively without self-damage. A genuine large health pickup
at the injured participant remains unconsumed; E-tank confirms are blocked.
Both native Start menus and an active-duel current-build save/load pass.
A positive-HP1 final-hit setup followed by an actual X/P2 attack selects the
native X result text with retries unchanged. This is an initial tuning
profile for owner feedback, not a claim of competitive balance.

Snapshot schema11 includes intro/arrival/health state and rejects old states;
the deterministic digest includes the same new fields. All23 CTests pass.
The exact final executable also receives the existing seven-checkpoint
two-process Split movement/attack/personal-menu regression and a cold launch
of its fresh pre-inner-door fixture before packaging. These bounded checks
do not qualify physical LAN or every duel/result/ending interaction.

F62 remains unverified: complete appearance, remaining story/ending branches,
native cold-save winner persistence and current LAN duel remain open. Native
cold-save winner persistence is not implemented. No frozen comparison is
closed by this replacement, and general automated burndown stays stopped.

Evidence: `G:/Temp/Codex/psxrecomp/x5-parity/owner-duel-polish-20261023`,
including `handoff-native-duel/session.json`, `handoff-combat/report.json`,
`handoff-second-NPC/report.json`, `ctest-HUD-group.log`,
`paired-current/report.json` and `duel-cold-check/report.json`.
Earlier rejected HUD variants and their visual failures are retained.

## Fifth owner report and manual QA handoff (2026-10-23)

Owner confirms the co-op reunion skip works and reports no other intro issues
on FA55C32F apart from the missing intact statue. The original17990-row
recording yields17989 verified replay inputs (the incomplete trailing interval
is excluded), with original CRC/input-read-order checks intact. It records
trailing X's actual grounded return with HP32 and unlocked controls, normal
Sigma combat/progression and a later ordinary Zero death. General acceptance
does not qualify every individual authored-descent, warning-fatal, opposite
campaign, input/control or effect comparison; those statuses remain explicit.

U11 / beads-eio.10.17 is separate from the reunion policy. Original eight
class09 statue records are all admitted, but intact subtypes0..3 retire while
reveal overlays4..7 remain. Native2D800 uses layer1: P2 incorrectly retains
mode6/vertical offset560 (layer1 at3120/704 while its main view is4160/192).
Original EEBE0 already publishes mode2/offset0 for the roof; P1 receives it,
but the retained P2 camera misses this one-shot command. Guarded EEC0C now
publishes exactly the native mode and four offset bytes to both retained
cameras. Original updates compute private XY/history/bounds. Allocation,
culling thresholds, sprite layers, fatal rules and shared animation stay native.

C1097F02 runtime retains all eight actual pieces and layer1 matches Zero's
roof view while X remains at144/1131. The source-format-preserving final build
is6987087D. The attempted late-atlas prerequisite times out; its original false
receipt is preserved. A bounded continuation proves actual actor lifetime,
not full reveal art: its composed scenery still reflects that bypassed native
atlas prerequisite. U11 remains open for the owner's normal-route visual
retest. No admission exception or speculative scenery matrix was added.

The owner now requests a local Windows QA ZIP and a stop to automated work,
plus both heroes outside their duel. A genuine native game-created91 card is
loaded normally, then the original raw18/0 checkpoint2 loader bypasses prior
stages/outer door. Both heroes are positioned at6312/6336, natural floor778/779,
camera6080/608. A native placement rescan admits the inner class8/sub8 door
at6384/768 skipped by teleport. Both heroes are alive, grounded and unlocked;
no duel entry/result is latched. Initial no-rescan setup failure is retained.

P1 is Zero/keyboard; P2 is X/controller. Native Start pauses before slot7 is
saved. Actual same-build save/restore and a fresh-process cold load pass;
closing the pause restores both controllable heroes outside the door. The
packaged duel launcher makes a disposable run each time, leaving normal
campaign cards separate. Either player may enter; the other becomes the boss.
This is setup/restore evidence, not a completed F62 duel or native cold-save
winner-persistence pass. Current-build states are intentionally build-specific.

All23 CTests pass on6987087D. The same final candidate receives the bounded
two-process Split movement/attack/personal-menu regression before delivery;
its exact receipt is bundled. Physical LAN and the playable duel remain owner
validation. The individual frozen234 comparisons/15 source hashes remain intact;
no comparisons close from general acceptance, loading or a setup screenshot.

Evidence root: owner-fifth-reports-20261023. See statue-source/replay-lifetime-cause.json,
baseline-exact-owner/session.json, current-statue-check/report.json (false),
current-statue-lifetime/report.json, duel-current-preparation/session.json (false),
duel-current-ready/session.json, duel-cold-check/report.json and final-paired-regression/report.json.
See COOP_MANUAL_QA_HANDOFF.md for bundled episodes and the remaining69 checks.

## Fourth owner reports: reunion policy and immediate entry fatal (2026-10-23)

U9 / beads-eio.10.13 supersedes the earlier preserve-dialogue policy for this
exact scene: co-op skips the X/Zero reunion because the heroes are already
together. Guarded F8170 uses the original replay branch after WARNING retires,
temporarily presenting PLAY2E1 for only that callback and restoring it0.
Native1->4 skips counterpart allocation and message0. Sigma-specific emergence,
taunt/message1, HP fill and normal control release remain original; solo is
excluded by the co-op guard. Actual new native skip count is1, no authored NPC
is allocated, and the original Sigma fight reaches HP48/target48 with its
FB3D8 collider and combat action8. This is more than a scene load.

U10 / beads-eio.10.16 is separate: original55A1D945 input7965 changes XHP32->80
and clamps X2688->4168 at unchangedY1131 while Zero enters the roof. Exact
original14946-input replay retains native CRC and input-readorder checks.
Scene2 publishes its192-height camera during a still-running P1 world pass;
original2A440/2A6C0/29184 then applies those bounds to P1 before teleport ends.
The native fatal instruction is source-bound; the truncated write trace does
not capture its exact store. The fix projects the actual living scene owner's
body for that one remaining camera call, preserving canonical campaign, shared
palette and world restoration. Native pit rules and camera bounds stay intact.
The current callback executes once; X keepsHP32 through outgoing/hidden and
actual incomingY344 as the initialized Sigma fight starts. Zero also keepsHP32.

The bounded setup/observer stops are retained individually. The genuine native
CP0 atlas prerequisite ultimately completes, then the same episode continues
through actual P2door, WARNING, native skip and Sigma-specific text/HP fill.
Aggregate600-world-update budgets stop during normal emergence and then at
incoming phase4. A later unattended combat interval reaches a real retry
(checkpoint3/retries1). Its fresh roof bodies/scene0 are not a first-encounter
return or control pass. Leave R11/F2/F53/F196 unverified; owner validation must
confirm X finishes returning and both heroes can move/attack in that first fight.
All cards are unchanged. No boss-defeat, physical-LAN or full-cinematic pass.

Evidence: owner-fourth-reports-20261023/intro-room-qualification.json and its
hashed original, source, native-prefix, raw-stop and current-peer receipts.
Owner retest uses FA55C32F, Either doors, keyboardP1/controllerP2, normal window
and audio, default diagnostics/hint suppression/capture and ordinary watchdog.


## Third owner report: separated heroes and moving supports (2026-10-23)

U8 / beads-eio.10.15: Zero died near the opening roof while X approached from
below. The exact original4782D7A2 recording replays with native CRC/input-read
checks intact. At input10694 Zero HP20->0 while X HP19 is unchanged. Native
class3/sub1 support is still idle atX4408; it only starts after the trailing X
crossesY608. Zero's fixed roof camera limit192 remains unchanged. This binds
the failure to a P1-only structure trigger, rather than proving a camera-bound
collapse.

The guarded original F65C8 idle callback now uses eligible P1 first, otherwise
living P2. All three structures retain their native movement, terrain edits,
quakes and contacts. Their two provenance-checked, collisionless7E pieces also
use the original predicate so they move and retire with their actual parent.
No camera relaxation, pit immunity or new mutable state is added.

On55A1D945, X remainsY1131 while Zero activates all three structures. Their
native movement/map updates complete; both linked pieces move and retire.
With assistance off and immunity61/A4 zero, one ordinary Left crosses the old
fatal region and Zero landsX4313.5/Y362 with HP32 unchanged and native support8.
World/native clocks both advance421; render-error deltas are zero and native
cards are unchanged. The raw97-sample harness remains false at its final
campaign-byte observer: its initial asynchronous read caught transient P2
projection. An independent guarded read-only review confirms six stable
canonical X campaign reads0. Both receipts are preserved without overwriting
the original failure.

DirectCP2 already shows incorrect scenery before any new callback executes;
it bypasses the earlier native atlas prerequisite. This shortcut qualifies
support mechanics, not native-route presentation, dialogue or boss entry.
The owner retest starts on the real opening route with keyboardP1/controllerP2,
normal window/audio, default diagnostics/hint suppression and door_players=either.
Either hero's door entry should summon the partner under the existing option.

F196 gains supplemental geometry/context evidence but stays unverified: its
original script-handoff/camera-height recovery trigger has not occurred here.
The nine-check H-intro bundle and all234 frozen comparisons keep their status.
Evidence: owner-third-reports-20261023/{baseline-original-owner/session.json,
current-linked-structure-check/report.json,canonical-progress-review.json,
intro-geometry/f196-ledger-mapping-assessment.json,intro-structure-qualification.json}.


Previous owner-fix qualification4782D7A2 (2026-10-23): F170 passes the actual
terminal X-death/wipe/fade/retry counterexample after distant Zero death.
F95 returns to unverified: Grizzly's black sky is repaired in two completed
native intervals and original layer wraps, while Mattrex/native route obligations
remain. The report's later third-interval fatal and raw false are preserved.
R9 stays failed pending owner-specific visual confirmation after bounded native checks; F151 stays unverified.
Evidence: owner-second-reports-20261023/current-wipe-grizzly-fresh/report.json
and current-views-analysis.json. Native cards are unchanged; native control
release is observed, not a new own-seat post-retry input claim.


Owner second-report baseline reconciliation (2026-10-23; later qualification above): R9 and F170 reopen from
passed to failed; F95 moves unverified to failed. Actual Zero airborne-dash
garble, exact-owner final X-death scenery corruption and native Grizzly black
sky are retained individually under owner-second-reports-20261023.
Earlier bounded passes remain valid for their inspected scopes; F59's normal8
pixel comparison is unchanged. Compatible source fixes are applied/building,
but none has current fixed-runtime qualification yet. Local issues .10.11,
.10.12, .10.13 and .10.14 remain in progress.
U6 chooses only the authored intro counterpart: hide its co-op drawing while
preserving dialogue and native scene progression. Global F151 stays unverified.

The exact-build baseline collects input7005 terminal-death garble and
input33871 Grizzly black sky; its success is replay/collection evidence, not
a graphics pass. R9/F170 form one new technical fix bundle. F95 retains its
existing Mattrex route/upload membership and separately needs automated U7
requalification, without another owner navigation assignment.

Earlier owner bugfix milestone E0C20000 (2026-10-23; bounded scopes retained): independent native menus now
show X and Zero separately, retain equipment management during partner healing,
share E/W budgets under the existing policy, and restore current-build paused
states and both controls. F110 is requalified through the late native large
heal with actual composed intervals. The owner intro scenery overwrite is
fixed by a per-view journalled original atlas; actual region crossing, native
replacement VRAM and P2 fatal/survivor art are qualified. At that milestone
F95 returned to unverified for distinct earlier-door/train-art obligations;
the native Grizzly counterexample briefly reopened it before the current
Grizzly repair qualified above.
All23 CTests pass; regenerated native menu hook sites are checked in the
source guard to catch stale generated bodies. Local issues .10.8/.10.9/.10.10
and owner-reported-bugs-20261023 retain the bounded evidence and observer
setup corrections. One current Split delay3 CLI pair passes7 checkpoints and150 matched core/mod
samples through4896, including independent menus and non-opener team resume.
This does not close physical LAN, wider menu/modal or campaign gaps.

Continued automated content audit (2026-10-23): F22 and F56 are individually
inapplicable. X5's authored normal, charged, armor, Zero and deferred weapon
paths have offensive contacts and finite retirement, but no owned supporting
surface for Lightning Web wall-grip/top contact. The remaining animation/
combo/release/child domains are now source-bound. These two are counted
separately from passed gameplay; evidence is in
`weapon-support-final-source-20261023/receipt.json`. Other technical work
continues; human-dependent checks remain deferred.
The frozen 15-source Playtest42 reference and all 234 original
report/failure/P columns remain preserved. The F97 X5 check clarification
is source-audited below.

Automated remainder milestone (2026-10-23): F81 and F133 close after individual
native evidence review. The prior queue did not mean all unverified checks
needed human intervention. F81's observer decoded the palette mask backwards
and expected raw colors where the native effect restores ambient darkening.
Current evidence now proves all17 affected banks and actual survivor input.
F133 exposed and fixed a real Chaser bug: deferred P2 debris read P1's car
through parent+54. All11 debris now use P2's existing car mirror/palette;
original art and native retirement are qualified alongside ordinary/Armor paths.
No every-frame or bug-by-roster matrix was added.

Current automated milestone (2026-10-23): F111 passed after reproducing
P2-only failure and same-parent P1 success. The guarded native distance hook
now admits P2, while original warning, animation, usable collision and expiry
run once. Actual P2 top contact and release passed. F160 contact-collapse and
F201 signed-negative release remain distinct.

F97 passes its corrected failure-mode comparison using verified retained4DB
final-door evidence: native boss initialization waits43 ticks during actual
Both traversal, then commits door -> boss init -> stand14, with no passenger
cycle and both native controls. Relevant current guards match that qualified
path. This does not claim a literal X5 midboss door or a new current-build run.

Nearby water-head ownership is also fixed and actually qualified: both native
heads are born on one round from separate hero origins, rise and retire.
The native clock/RNG is shared and P1 allocation retains priority. Art identity
is source-bound; distant-view/unoccluded P2 visibility and distinct scene
boundaries remain unverified. All raw observer failures are retained. No
further whale-floor/camera variant is queued.

The guarded memory helper avoids repeated equal plain-RAM stores only with
PGXP master off and outside private replay; all precision/device/arena and
cross-boundary stores retain their original semantics. Existing regression
coverage passes. SDK existing stack samples now precede bulk trace output;
watchdog deadlines, ring caps and sample limits are unchanged. The earlier
4.153s native class39 watchdog stop remains unresolved.

The native lift hurt/release/reland/jump check reproduced no support defect;
F201's signed-negative producer remains unidentified. F86 reaches actual
P2FALLEN and a real eye-targeted shot, but misses the final hit before P1 dies;
native clear/report/fresh17 remains unverified and deferred for human alignment.
Raw observer failures and later independent deaths are preserved.

| Shared check | Current result |
|---|---|
| F182 laser geometry | Passed: actual native Firefly beam injures P2, preserves P1/resources, retires and renders in both offset views. |
| F28/F144 scenery/menu | Passed: genuine final42 defeat->43, actual menu/resume and repeated native shared atlas cycles; stationary bodies, coherent private phases, intact meters/foreground. Later P2 fatal is separate and unqualified. |
| F81 shared flash | Passed bounded actual P1-preinit-death/P2-survivor flash: all17 native palette banks brighten/restore ambient, both shared views consume live GPU colors, fresh input responds before later ordinary hurt. |
| F36 moving pickup | Gravity/support/20-tick carry pass; full movement cycle stays unverified after bounded residency loss. Human setup deferred. |
| F166/F187/F190 midboss | One C72 encounter completes P2 admission/release, then both fall from the ledge before Y960. Native ending/core deaths/cleanup/CP4 stay unverified; safe living descent is deferred to human setup. |
| R23 homing | Current7336 reaches actual native emission/startup60, P2contact/hurt and finite cleanup without watchdog. Tracking/reversal remain unverified after bounded nondiscriminating/fixture-latency setup; no second attack variant. Prior4.153s watchdog rootcause remains unresolved. |
| F93/F16/F133 vehicles | F93 speed/clock pass retained. F133 ordinary/Armor/Chaser death presentation passes after fixing P2 debris ownership and validating original art/retirement. F16 exact exhaust ordering and the road endpoint remain separate and unverified. |

Current open primary buckets: netplay7, environment22, doors9, controls8, combat16, vehicles2, story3, diagnostics1.
All 68 have individual evidence and an explicit next action:
42 in 15 deferred game-setup/encounter batches,
13 technical source/setup/presentation prerequisites, 11 attended controls/frontend/network,
the parked duel and one elective scene-staging decision.
See [the exact remaining queue](COOP_HUMAN_SETUP_QUEUE.md).

Camera-emitter milestone B4EC5680 fixes an actual X5 defect: an effect
allocated by P2 initialized from P1's upper camera and immediately skipped
its live state. Allocation now retains its logical owner through the original
parent/child X, random Y offset and native camera eligibility check. Exact
same-frame evidence proves P2's camera origin, 34 native motion updates and
original retirement after 35 frames. Full F194 stays unverified for its
separate authored start/end and scene handoff; the remaining route setup is
deferred with the existing Mattrex batch after bounded attempts. No more
camera/floor variants are queued. Evidence is in
`raw6-sensor-chain/run-B4EC5680/component-qualification.json`.

Additional native X5 breadth: actual Gaea selection through the original
roster and initializer passes, and all six complete graphics/assembly/color
comparisons match DAT95 across stage2 and raw7 CP4. The block-carry attempt
ends at a native P2 fatal during position-only setup, before any push. Later
layout review shows the setup overlaps an authored upper block; the fatal
producer was not traced. No game defect, carry pass or F104 slippery-floor
analogue is inferred. Original cards remain unchanged.

The final compatible B4EC5680 default-capture peer regression stopped after
10 of 14 checkpoints on a debug TCP connection timeout. Retained native core
and mod state agree through the last common sample; the cause of the disconnect
is not established. Earlier nonfatal slow-frame dumps are followed by more
than 1,200 frames of progress and do not establish a terminal deadlock. All 23
title tests and independent emitter source review pass. The three original SDK
contract tests remain qualified. The final peer bundle remains unqualified.

Three authorized workers completed one bounded source/evidence review of all
13 technical comparisons; all remain individually unverified. No new safe
runtime recipe or whole-game absence proof justified a status promotion or
another game launch. F52 can share its remaining intro and native CP4 water
handoff observations with the existing owner encounters; R10/F115 have one
conditional shared visibility observation. Unidentified mechanics remain agent
technical work. Evidence: technical-13-owner-batches-20261023 and the individual
reference rows.

The owner prefers shared human validation for the 42 setup-deferred comparisons. Four
playtest blocks preserve all 15 original encounter bundles; local controls are
P1 keyboard/P2 controller, and LAN is available for the 11 attended checks.
See the [shared validation plan](COOP_HUMAN_SETUP_QUEUE.md). No owner session
has been launched and no additional comparison is closed by this planning.

Prior automated candidate7336E94F combines the native ownership fixes,
PGXP-aware projection optimization and SDK evidence ordering. It passes all23
title CTests and11 original watchdog timing checks. One actual default-capture
Split delay3 CLI pair passes14 checkpoints and260 matching core/mod samples
through8352 (resolved8358), with1458/1458 clean private-view restorations and
all10 typed diagnostics options. P1 scaled native contact passes; P2's saber
miss remains a disclosed limit. This is same-build local CLI evidence, not
physical/frontend/Internet or cross-build validation. The separate R23 natural
homing retest completes native startup/contact/cleanup without watchdog, but
tracking/reversal and the prior watchdog cause remain unverified. No blanket
stability claim. Three additional original SDK PGXP-store/freeze-policy/watchdog
contract tests also pass.

Evidence: `continuous-automation-20261023/candidate-receipt.json`,
`combined-regression-qualification.json`,
`water-fixed-native-component-qualification.json`,
`head-effect-art-audit/receipt.json`,
`scene-door-conflict-source-20261023/receipt.json` and
`subagent-x4-qol/class1b-native-access-20261023/F111-fixed-7336E94F-qualification.json`.
All profiles/cards/captures remain local and prior owner profiles are preserved.

Prior combined QA candidate CDD2FE2A adds the Chaser parent fix to C72CD101
and retains isolated SDK commits93f3afbb and21994c4e: arm the existing heartbeat immediately before guest execution,
and refresh it after completed diagnostic requests/synchronous display capture.
Watchdog deadlines and forensic evidence remain enabled. The original shared
SDK is preserved untouched; the X5 SDK junction points to its isolated local
worktree. 23/23 CTests pass. Controlled native-mechanic runs use documented
PSX_DISPLAY_RING=0. Its default-capture Split CLI peer bundle passes
14 checkpoints/265 matched core+mod samples through8544, resolved8546;
1458/1456 clean private-view restorations and all10 typed options are retained.
Actual P1 scaled contact passes; P2 saber misses, so no P2 contact claim.
This successful bundle does not resolve the separate4.153s native class39
watchdog stop. No current blanket stability/renderer/physical/frontend claim.

CDD2FE2A now has a separate current human-setup profile, prepared windowed
with normal audio, Split/Both and private native-save copies; default capture is restored
by its launcher. Physical/frontend/audio-output checks remain unverified.
[Playtest instructions](COOP_PLAYTESTING.md) record the handoff. C72CD101 and D1B93AFC are
preserved alongside it. No owner window is launched. Evidence: automated-remainder-20261023,
finish-burndown-20261023/ledger.json,
human-setup-queue.json, scenery-qualification.json, candidate-receipt.json and
frozen-reference-audit.json, combined-regression-qualification.json and the
individual Chaser/homing/midboss qualification receipts. Earlier counts below
are historical.
This is not a full parity claim.

Owner steering (2026-10-23): finish all feasible non-duel comparisons and
park F62 for final human validation. Its implementation and retained evidence
are preserved; it remains unverified and is not a gate for the other77 open
comparisons. Current resumed campaign starts156passed/3failed/75unverified,
78openof234. Bounded shared encounters target the authored moving pickup,
actual midboss cleanup/exit, native laser geometry, and heavy-scene palette/
menu provenance. Use the existing human queue when a genuine setup remains
unavailable; do not replace missing mechanics with loading/screenshots.

Owner story-duel design milestone (2026-10-22): F62 now has a concrete X5
contract: the two players fight the X-versus-Zero encounter, the opposing
participant has exact native NPC presentation and the right health meter,
and the winning character selects the story continuation. This replaces the
undecided consecutive-fights proposal; it does not add an extra NPC fight.
[The design and bounded implementation checks](COOP_X5_DUEL_DESIGN.md) record
the native integration findings. The owner has settled role and controls:
the non-entering player becomes the boss and keeps normal moves, weapons and
full native Start-menu move selection. Implementation has resumed under that
clarification.
F62 remains unverified. Counts remain156 passed/3 failed/75 unverified,
78 open of234. The remaining queue is39 human-setup comparisons,26 unresolved
native/source/presentation prerequisites,11 attended controls/frontend/network,
1 playable-duel implementation and1 deferred scene-staging policy.
No production, SDK, watchdog, save or candidate changes in this design milestone.
Evidence: design-pvp-duel-20261022/qualification.json.

Playable-duel local milestone (2026-10-22): candidate F58BE959 passes
both actual inner-door entry paths, each with the other player previously
fallen. Native introduction/remote return, full-HP revival without spending
a retry, retained moves/ammo, normal movement, own-menu C-Shot selection,
reciprocal nonlethal injury and actual current-build slot7 restore pass.
The hidden NPC's owned aura retires; both cases record zero local-view leaks.
23/23 CTests pass. Source/private cards are unchanged and the owned runner is
stopped. Quit returned busy/frozen; graceful exit is not qualified.

F62 remains unverified: complete NPC pose/palette presentation, actual final
hits/winner-selected story, native cold-save persistence, interrupted/draw
handling and actual peer qualification remain open. Genuine starting-character
privileges are preserved. D1B93AFC remains the separate owner playtest build.
Counts remain156 passed/3 failed/75 unverified, 78 open of234.
Evidence: `pvp-duel-normal-controls-20261022/qualification.json` and
[duel design](COOP_X5_DUEL_DESIGN.md).

Genuine duel-result milestone (2026-10-22): current 7EF32812 bundles the
palette correction with actual X/P2 and Zero/P1 boss-side victories after
positive-HP1 setup. Each real attack produces the fatal, each losing body
finishes native death to FALLEN, and the corresponding original result
dialogue runs without spending a retry. X reaches initialized native
mode20/stage11/CP0E; Zero reaches observed story-loader mode19/raw18/CP0E,
so its later chapter initialization remains unverified. Actual slot7 restore
from the X story back to the duel passes. 23/23 CTests pass; both encounters
also retain entry/revival, normal movement, menu/equipment, reciprocal injury,
mid-duel restore and zero local-view-leak checks. Owned runners are stopped;
source/private cards are unchanged. Original native starting-character
privileges remain untouched. Winner story identity is independent and
included in current-build snapshots/digest; native cold-save persistence is
unimplemented. F62 stays unverified for the remaining presentation,
chapter/ending, persistence and peer gaps. Totals remain156/3/75,78 openof234.
Evidence: `pvp-duel-normal-controls-20261022/winner-qualification.json`.

Bounded palette correction (2026-10-22): original palette producer2C338
writes contiguous 16-color slots; upload17484 uses RECT(0,480,256,32).
Both retained sprite lookup paths incorrectly treated the PSX CLUT encoding
as 64 slots per row. Armor CLUT7843 now reads offset260 instead of860;
parts7844 reads280 instead of880. A producer/consumer regression covers all
16 colors of those slots and unchanged first-row slots; 23/23 CTests pass.
This source-backed fix does not prove F133/F144 causality or close either
comparison. The corrected palette code is also bundled in current7EF's two actual duel
encounters; complete ordinary color/effect presentation remains unverified.
Evidence: `pvp-duel-normal-controls-20261022/appearance/palette-layout-audit/qualification.json`
and `ctest-palette-layout.log`.

Owner-profile preparation milestone (2026-10-22): the G: D1B93AFC human-batch
profile is ready; see [playtest instructions](COOP_PLAYTESTING.md). Its launcher
configuration is windowed with normal audio, Split cameras and Both-player
doors. A separate hidden, muted run loaded the copied genuine campaign card
through native CONTINUE and entered co-op. Source/private cards are unchanged
and both verification runners are stopped. The initial observer omitted card
metadata and was corrected once before native input; raw results are retained.
The debug quit request returned busy, so owned processes were terminated.
This qualifies profile preparation only: physical controllers, visible frontend,
audio output and graceful exit remain unverified. No comparison status changed.
Evidence: qa-owner-human-profile-20261022/qualification.json.

Current bounded forms/temporary-beam and human-setup milestone (2026-10-22).
**156 passed, 3 failed, 75 unverified; 78 open of 234.** F37 and F73 close
after individual scope/evidence review. Candidate D1B93AFC is unchanged.

| Comparison | Result and evidence |
|---|---|
| F37: simultaneous native forms | Reconciles eight actual feature contracts across both rosters, original art/abilities, genuine progress, solo/host choices, death/retry and current states. A clean current-D1 mapped rewind retains native P2 menu[1,0,0]/world638 through51 neutral host frames; P1Start is ignored, P2Start closes, and both controls recover independently. Fresh slot7 restores changed resources, exact private DAT93/Zero90 and saved state. Generic rollback/rematch and an exhaustive native read-site matrix are separate from this bounded feature pass. |
| F73: temporary P1 departure restarts stage | Corrects the X5 mapping from unrelated distance/tether to the original failure: a living P1 passenger completes its outgoing beam while P2 keeps the world. Retained9FFB capsule evidence has156 normal observations,139 hidden P1 body4=3 samples, unchanged HP13/17/CP2/retry2 and stationary P2, then both native controls. Its535-byte outgoing path and shared-state restoration match current source. Passes the closest native analogue; no literal automatic off-camera catch-up is claimed. |
| F63/F99/F181: protected balance | One pre-encounter CLI error is corrected once. Current native Mattrex entry/fullHP65 and two real Wing8 casts work, spending288->276->264; neither reaches accepted contact or halfword10000. Current primaryX4555.5 differs from historical accepted4725.5, while boss4568 and P2=4719 remain separated. Preserve the observed difference without inventing AI causality; defer nearby-pair/native contact to the human batch. All three remain failed. |
| F190/F194/F195/F202: scripted hazards | Bounded Mattrex source identifies an authored F202 Y/map-change candidate: sub0 camera2192/hero2228; sub1 camera2656/hero2692. Actual map solidity/P2 admission/safe P1 footing are unobserved. This is not proof of the distinct bomb/fireball/quake producers; all four remain unverified. |

Owner steering: use cheats for setup/resources/upgrades and shortening fights.
Keep the actual outcome under test native. Difficult setups now go to the final
human-assisted batch after a bounded attempt and one concrete correction.

[The final setup queue](COOP_HUMAN_SETUP_QUEUE.md) lists all78 remaining IDs
exactly once. It distinguishes39 comparisons in14 human game-setup batches,
26 unidentified native/source/presentation prerequisites,11 controller/frontend/
network checks and2 feature-policy decisions. Navigation alone does not settle
unidentified mechanics or optional encounter/narrative policy.

| Remaining primary bucket | Open comparisons |
|---|---:|
| Netplay | 7 |
| Controls | 9 |
| Environment | 24 |
| Doors | 10 |
| Combat | 20 |
| Vehicles | 3 |
| Menus | 1 |
| Story | 3 |
| Diagnostics | 1 |
| **Total** | **78** |

All234 original reports and15 frozen source files remain preserved. The only
new check-definition correction is F73's audited native failure mapping.
Existing D1 production21/21 tests and13-checkpoint peer regression remain the
baseline; no executable, SDK or watchdog change was made. Both original
observer failures and exact corrections remain recorded. All four owned
runners are stopped and source/private cards are unchanged. No owner window
was launched; no remote publication or tracker sync.

Evidence: qa-beam-rewind-protection-20261022/qualification.json;
F37-root-review.json and F73-root-review.json in that directory;
qa-rewind-input-clean-20261022/qualification.json;
subagent-halfword-protection-runtime-20261022/qualification.json;
subagent-mattrex-scripted-hazard-20261022/receipt.json.

Previous final-chain and native Armor/source milestone (2026-10-22).
**154 passed, 3 failed, 77 unverified; 80 open of 234.** F139 closes after an
actual native final-boss chain and recovery. Candidate D1B93AFC is unchanged;
this milestone adds runtime/source evidence and ledger corrections.

| Comparison | Result and evidence |
|---|---|
| F139: final-boss Zero chain | Actual P2 Zero types18/19/1A damage72->70->68->63, advance own combo0->1->2 and install original recovery2/2/30. The complete trace retains finisher29..0/table restoration, native idle/ground recovery and a fresh accepted63->61 hit. One disclosed position/history setup; no boss-state repairs. Passes native final42's matching interaction; no class43 chain or scaled-final claim. |
| F133: Armor death presentation | Actual native forced rider ejection and finite vehicle/seven-debris retirement preserve both heroes, retry and progress. Source corrects the observer's name: seven class3 actors are debris; separate common class4 explosions were not retained. Olive-disc pixel ownership/color remains unproved. Raw final control assertion stays false because its baseline retains prior P1Right/action4 before release/stop. Full F133 stays unverified. |
| F100: forced recovery | Safe-platform native P1 Armor ejection adds distinct evidence, but does not reproduce returning P2 in tight ceiling/floor geometry. Remains unverified. |
| F63/F99/F181: balance | Historical actual active-bank native-menu save/load is independently revalidated, including exact bank/boss bytes after real HP/carry mutation. One current first-Dynamo run reaches original14 pages/fullHP60/control, but its Square attempt produces no accepted bank advance in45 updates. Birth/contact cause is unproved; independent weakness/protection checks were aborted before attempt. All three remain failed; no relaunch. |
| F111/F160/F201: native supports | Bounded source exclusions retain individual gaps. F160's bounded check is corrected to original contact-driven collapse; frozen X4 explicitly did not add an expiry timer. The only nearby-negative carry candidate is E-tank initialization, not platform release. All remain unverified. |
| F138/F140: other final mechanics | Genuine06 lacks the native Zero weakness ability bits. A distinct solid-fist tracking path is source-mapped, but its exact authored window was not observed. Neither is promoted by the chain pass. |

| Remaining primary bucket | Open comparisons |
|---|---:|
| Netplay | 7 |
| Controls | 10 |
| Environment | 24 |
| Doors | 10 |
| Combat | 20 |
| Vehicles | 3 |
| Menus | 2 |
| Story | 3 |
| Diagnostics | 1 |
| **Total** | **80** |

Preserve all234 individual reports and all15 frozen source hashes. Only F160's
audited X5 check wording changes; original X4 bug/failure/source remains intact.
Common arrival/safety gates may be shared, but a bounded ordinary-hit failure
must not abort an independent mechanic check that remains safe to attempt.

Existing current-candidate21/21 tests and13-checkpoint peer regression remain
the production baseline; no executable, SDK or watchdog changes were made.
All three owned runners are stopped and private/source cards are unchanged.
Everything remains local.

Evidence: qa-armor-support-reconciliation-20261022/qualification.json;
final-zero-chain-20261022/qualification.json;
subagent-dynamo-protection-runtime-20261022/qualification.json;
subagent-armor-disc-interpretation-20261022/receipt.json;
subagent-temporary-support-contract-20261022/receipt.json.

Previous native ending, weakness and diagnostic metadata milestone (2026-10-22).
**153 passed, 3 failed, 78 unverified; 81 open of 234.** F131 and F142 close
with individually reviewed mechanics. The current local candidate D1B93AFC
contains the focused logger correction; native final progression is qualified
on its preceding F1115C16 gameplay code. Frozen Playtest42 remains unchanged.

| Comparison | Result and evidence |
|---|---|
| F131: distinct counterpart weakness | Historical E61 actual X/P2 Wing Spiral8 against class34 damages71 to66, spends only its own12 ammo, enters native wind phase0B/halfword protection and recovers its original75FC0 table/timer0. This is a distinct native reaction family. Failed class36 setup, F135's attached-parent condition and scaling remain separate. |
| F142: next final phase | F111 retains both living, drawn heroes in all1,034 post42-defeat observations through class43's33 native pages/fullHP127/release. Complementary valid same-candidate P1/P2 jumps each rise39.28125px. Both interrupted zero-rise probes stay rawfalse and are explained by original hurt, not promoted. |
| Actual final defeat and ending | A genuine base-X charged child0x12 supplies native damage5 after disclosed positiveHP1 and position/history setup. Original43 death,14 death pages,66 Xending pages and literal staff credits follow native branch writes. F151 stays unverified for global two-hero staging, alternate ownership and other/terminal endings. |
| F144: heavy scene | Both meters, original fists/effects and native menu/resume are retained, but the postmenu left background is cyan while right staysgray. Aggregate GPU/restoration counters cannot prove intended colors. Remains unverified; no speculative renderer change. |
| F36: pickup on moving lift | Original source proves pickup carry and an authored item on the first lift path. The bounded runtime's early parent-allocation gate prevents approach, so no item contact/carry occurs. Preserve both observer/setup failures and defer; F201 remains distinct. |
| Logger | Fixes four missing session metadata fields: both automatic forms and both boss-balance options. Both actual peers now record all10 typed effective choices, even when copied offline preferences disagree. Existing input format, paths, defaults and watchdog stay unchanged. |

One current D1B93AFC Split/delay3 loopback bundle passes13 compatible checkpoints:
271 matched core/mod samples through8672, resolved8675, and1,218 clean private
draw restorations per peer. Actual X/P1 buster advances serial1->2/carry0->5/10
without integer HP loss54->54; own recovery is observed positive13 then0,
with ready partner clock0. The preceding F111 run directly observes15.
Host formsOFF overrides standalone offline formsON and preserves native
FourthArmor1/Zero5/genuine flags. Actual P2 ground Saber misses an airborne
boss; it is not a rejected-contact or Zero damage pass. F63/F99/F181 stay failed.

Fresh21/21 CTests pass4.56s. The added metadata regression first fails against
the old recorder, then passes with all10 local/effective online values.
The first current peer attempt ends on a transient busy debug response; its
slow-frame/startup dumps, incomplete48MB capture and rawfalse report remain.
One bounded observer correction retries that rejection on the same live
process; the corrected current bundle completes. No watchdog change or
unproved freeze-cause claim. The compatibility profile is
mmx5-coop-delay-v1:0d733b5ce25321dc2b1e3e57c30c8fb18966e2d2.
No cross-build compatibility is claimed.

| Remaining primary bucket | Open comparisons |
|---|---:|
| Netplay | 7 |
| Controls | 10 |
| Environment | 24 |
| Doors | 10 |
| Combat | 21 |
| Vehicles | 3 |
| Menus | 2 |
| Story | 3 |
| Diagnostics | 1 |
| **Total** | **81** |

All81 open comparisons retain individual checks and evidence. Class39's450
update window miss, actual intro/earlier-door footing, support/item setup,
physical controllers, frontend START/rematch, other content and inducedrollback
remain explicit gaps. No unsafe setup repeat or Cartesian variants. All owned
runners are stopped; SDK/watchdog unchanged; commits and issue notes stay local.

Evidence: qa-native-breadth-reconciliation-20261022/qualification.json;
final-death-progression-20261022/qualification.json and
F144-readonly-interpretation.json; subagent-class39-contact-20261022/qualification.json;
subagent-pickup-moving-support-20261022/qualification.json;
qa-logger-complete-options-20261022/observer-corrected/qualification.json.

Previous pause, Continue and final-transition milestone (2026-10-22).
**151 passed, 3 failed, 80 unverified; 83 open of 234.** F18, F136 and
F141 close with distinct native mechanics and transitions. Candidate F1115C16
contains both observed fixes; the comparison ledger retains every frozen ID.

| Comparison | Result and evidence |
|---|---|
| F18: pause body/HUD pop | Reproduced 37 opening and 35 closing world frames that omit P1 in P2's menu. The fix retains P1 and frozen personal pools only when the original world draws. All 567 completed transition frames are retained: 217 world frames contain both heroes and both HUDs, while 350 original UI-only frames contain none. Both owners, actual charge effects and both native Quit pages are covered. |
| F136: final intro after death/Continue | On 4DB, real team death/retry and literal Game Over/Continue each return through untouched CP21, paired walking, original door/intro/full HP and both native jumps/landings. Retry resource zero is a disclosed setup cheat; death, Continue, scene and progress flags are not set. |
| F141: final phase beam-out/return | Actual first-body defeat reproduces the unwanted passenger cycle on 4DB. On F111, the exact native death hold retains both heroes through class43's 33-page intro, full HP127 and original release. All 1,026 postdefeat observations retain both living, drawn heroes; pair state clears 2 to 0. |
| Separate control limit | P1's post43 jump rises 39.28125px. The overall helper stays failed because P2's probe runs in native hurt action0x11 and rises0; it is not an idle-control pass. The original reaction waits for its recovery timer. Preserve and defer this input gap; no extra setup or combat variant. |
| F37: enhanced forms | Native survivor/team retry, current save/load, restored attacks/resources and standalone Black Zero now have bounded runtime evidence. Rewind restores an earlier state, but menu/input isolation, scenery/thumbnails and remaining session policies stay unverified. |
| F109: delayed emitter | All 61 retained camera pairs put the emitted child 9px outside both original margins. Current source already unions both views; no canonical-only cull fix is justified. A safe visible-child route remains unverified. |

Both fixes are validated together on this current candidate: fresh build and
21/21 CTests pass (4.75s), including current snapshot paired-state2 roundtrip and
3/255 rejection. One combined local peer run passes 12 checkpoints, with
261 matching core/mod CRC samples through tick8352 and 1,166/1,168 clean draw
restorations. Both personal menus, automatic forms, effects, native scenery
clocks, owner death/release, default diagnostics and Alia suppression are covered.
Its compatibility profile is mmx5-coop-delay-v1:31e92ddb484bea7e41d0b15e9debd553641ff2c6.
Actual final combat over netplay, physical controllers and frontend services
remain separate. Existing F95 scenery corruption remains visible.

| Remaining primary bucket | Open comparisons |
|---|---:|
| Netplay | 7 |
| Controls | 11 |
| Environment | 24 |
| Doors | 10 |
| Combat | 22 |
| Vehicles | 3 |
| Menus | 2 |
| Story | 3 |
| Diagnostics | 1 |
| **Total** | **83** |

F63/F99/F181 remain failed; all 80 unverified comparisons retain individual
checks and evidence. Later final defeat/endings, weakness/chain/targeting,
current restore/rollback gaps and safe-route prerequisites are not promoted by
the first defeat. The failed P2 probe and native raw receipts remain unchanged.
No new schema or old-state migration is introduced. All owned runners are
stopped; SDK/watchdog unchanged; all work, commits and issue notes remain local.

Evidence: qa-paired-pause-final-candidate-20261022/pause-qualification.json,
final-transition-qualification.json, peer-regression.json and tests.json;
subagent-final-native-comparisons-20261022/qualification-summary.json;
subagent-final-release-interpretation-20261022/receipt.json;
subagent-form-lifecycle-20261022/qualification.json;
subagent-emitter-retirement-20261022/qualification.json.

Previous survivor, armor effect and final-door milestone (2026-10-22).
**148 passed, 3 failed, 83 unverified; 86 open of 234.** F121 and F149 close with native runtime evidence.

On 89E80004, original P1 death on the known first train platform retires
natively; Zero/P2 keepsHP17/full ammo/shared retry2/progress, moves31.5px and
returns grounded. The spatial controller continues152 world/152 native updates,
with51 distinct sampled origins and actual wraps in both saved views. Composed
images visibly move. Both-alive baseline is150/150 updates. Separate F95 black
sky completeness is retained; this is the distinct survivor clock check.

One current inverse Black Zero/P1 and Ultimate X/P2 run qualifies F149's
native aerial-dash analogue. Its real deferred effect1 follows X while Zero
is idle, uses X's private assembly and exact original DAT1 colors, is visible
with blue trails/exhaust, then expires through native falling/ground/Right.
An overstrict third-active-image assertion is corrected in the same encounter;
no second dash or setup. F37 remains open for its distinct form lifecycle,
restore/session and conditional ownership checks; earned capsule acquisition
belongs to F55/F71, and ordinary Saber palette3 is not an earned attack family.

Current candidate 4DB4A5F3, local fix c3b756b, adds a focused final Both-door admission repair.
Walking both heroes from untouched original12/0CP21 admits the authored door,
solving the old missing-placement setup. On89, native class42 starts before
paired crossing, so scene_begin refuses ownership and only P2 is stand-locked.
Nineteen pages still advance: the earlier150s bound does not prove frozen
dialogue or identify its timeout cause. The fix holds only this uninitialized
raw12/class42/sub0 body under exact loaded-module/dispatch guards. Native
closing/init/stand order is13207<13253<13266; scene owner2 retains both heroes
without passenger warp,34 genuine owner2 pages complete, native HP fills0->72,
both locks release, and actual jumps rise43.03125/46.5234375px then land.
Retained dump-cap/counter-schema observer errors are corrected without reload.
A bounded optional combat observation issues no HP change or shot. All270
samples retain native surface/weapon/facing/global release,161 are idle/grounded,
but boss/X remain4756/4757 and the observer's non-native32px distance heuristic
rejects them. This is not a native attack prohibition or failed hit. The corrected
future observer removes that heuristic; actual defeat/next phase, weakness,
chain/targeting, death/Continue and ending comparisons stay unverified.

A fresh snapshot-target rebuild exposed the stale schema6 unit fixture and
missing forms dependency. Corrected tests exercise schema7, independent option
rejection, all three immutable source identities, base/Ultimate binding and
allocation-free restore, including retained DAT93 UI after a base binding.
Current21/21 CTests pass in4.47s. Earlier green runs did not establish this
freshly linked schema7 fixture. The death QA helper also corrects its old112-byte
host stride to the actual120-byte descriptor, validates both characters and
records scope; compiler/layout/accessor/CLI checks pass, with no gameplay claim.

The same4DB candidate passes one combined local loopback pair:12 checkpoints,
258 recorded matching core/mod samples through8256,1199/1200 clean restoration
checks, default diagnostics/Alia suppression, both menus, automatic forms,
personal effects, train wraps and original scene-owner fatal/release.
This is compatible current-candidate regression, not an untested final network
encounter, frontend Host/Join, physical controllers, full campaign or ending pass.

F77/F108/F186 remain unverified. Exact native collider explains the old vertical
miss, but one corrected-height setup encounters the native pit before damage.
CP2's fixedY2416 camera leaves the gate's hurt-top2704 outside the established
safe approach. No damage/cleanup/exit or survivor variant is claimed.
No further aim/respawn/camera matrix; retain the missing authored prerequisite.

Evidence: qa-survivor-forms-final-admission-20261022/qualification.json,
qa-survivor-scroll-menu-20261022/qualification.json,
subagent-ultimate-movement-20261022/qualification.json,
subagent-final-prerequisite-current-20261022 and
subagent-breakable-contract-20261022/qualification.json.
All owned runners stopped; SDK/watchdog unchanged; all work remains local.


Previous native artwork prerequisite and Black Zero milestone (2026-10-22),
unchanged candidate 89E80004: **146 passed, 3 failed, 85 unverified;
88 open of 234.** No comparison closes in this milestone.

F95 moves failed -> unverified after original source identifies a bypassed
graphics prerequisite. Raw4/1 CP1 starts at X4000, beyond the earlier subtype3
door at X3856. That door normally requests the alternate background; the final
subtype0D door at X4528 does not. The checkpoint fixture retained the initial
atlas while drawing the later room. The retained noisy art is real, but this
fixture does not establish a co-op rendering regression.

A rendering-only canary in the same unchanged process writes pending request1
at800E92F8, then original11F9C/289D8/28A00 performs LoadImage and commits2.
The cached source matches the immutable DAT106 asset4 SHA exactly. Both changed
128x128 VRAM rectangles match that source after upload and differ beforehand;
the native and composed images regain coherent room art without the black gaps.
The initial sample observer incorrectly decoded word-hex as byte-hex and chose
unchanged corner art; that failed assertion and its correction are retained.
No second upload request, direct image/palette patch or production change.
This qualifies the native image pipeline only. Safe approach to the earlier
door, its actual trigger and raw1 black-sky completeness remain unverified;
the canary cannot pass a door, transition or the complete F95 comparison.

F37 also remains unverified. Source corrects the earlier enhanced-type3 label:
3 is the ordinary Saber palette index, not a separate earned attack family.
Current Black Zero/P2 naturally selects palette4E, copies the exact32 private
palette bytes, shows a purple Saber in both panes, expires and recovers native
idle/own Right. No contact/damage or other-family claim. Special31E5C, distinct
deferred producers, specific preview/capsule and actual rewind/rollback remain
open. subagent-enhanced-type3-20261022/qualification.json.

F63/F99/F181 stay failed. Source and unchanged balance-source SHA narrow the
remaining work to five distinct groups: actual rejected/held contact token
disposition; class34 halfword weakness/protection; class39/37 HP-delta reactions;
first Dynamo protected frames; and active-combat restore/rewind/scaled peers.
Native fallback encounters are disclosed scope, not an every-boss test matrix.
The current class32 setup reaches actual Both passage,16 text pages and full
native HP/control, but becomes unsafe before the first hit. No rejection or
positive-clock result is claimed; defer rather than repeat aim/respawn variants.
subagent-boss-policy-gaps-20261022/qualification.json.

Retain the initial tracing run's real starvation-watchdog abort at frame8541.
Its stack is in device service during original collision from attacks_tick;
cause is unqualified. A function-trace-disabled observation completes actual
door/intro/control and the canary; this does not prove tracing caused the crash
or close stability coverage. An observer output-directory collision is corrected
in the same encounter without another checkpoint reload. Watchdog unchanged.
All owned runners are stopped; no SDK/build/production or remote change.

Evidence: qa-background-prerequisite-burndown-20261022/qualification.json;
qa-bg-source-boundary-20261022; and
subagent-postdoor-native-upload-20261022/bg-restore-prerequisite.json.
Primary buckets remain netplay7, controls14, environment24, doors10, combat23,
vehicles3, menus3, story3 and diagnostics1. Each open row retains individual
evidence and a missing prerequisite.

Previous bounded lift and peer milestone (2026-10-22), unchanged89E80004:
**146 passed,4 failed,84 unverified;88 open of234.**
F49/F51 pass together on the first original8/0/CP1 lift. X/P1's native full-charge
release allocates one owned child, which travels143px while the support descends,
hits original terrain (70=00 ->05, native impact/state3), retires naturally and
returns the owner shot counter to0. Original dispatch/motion/terminal source
qualifies terrain contact; no enemy-HP or composed travelling-pixel claim.
Zero/P2 then dashes24.875px, jumps39.28125px and releases/reacquires native top
contact before idle while X/P1 stays exactly still. The initial phase5/scene-owner
observer race and one disclosed original CP1 reload correction are retained.
No forced support phase, pose, contact, camera or progression state.

F72 stays unverified after a fresh genuine mask08 Continue/Zero, native Rosered
Both entry/intro/control and one bounded low-positive-HP/native attack-box setup.
No real capture occurs across1609 native updates, so no fatal is attempted.
F75/F93/F101 also stay unverified. Current Chaser peers show original entry,
controls and147 checkpoint-qualified matching core/mod samples through4736;
the initial READY subset observer is contradicted by the composed image.
One corrected full native producer/child observer reaches road death before
complete READY/art coverage. Its151 raw CRC matches are partial mechanics.
Retain startup frame0/cycle0 hard_freeze-labelled dumps with later advancing
heartbeats; cause is unqualified. No third pair, full-road traversal, new online
checkpoint command or speed baseline substitution. The offline two-door setup
cannot supply the required online admission.

Evidence: subagent-lift-mechanics-20261022/qualification.json,
subagent-capture-owner-fatal-20261022/qualification.json and
subagent-current-peer-scenes-20261022/qualification.json.
All owned runners stopped; no production/SDK/build change or remote action.
Current first-P buckets: netplay7, controls14, environment24, doors10,
combat23, vehicles3, menus3, story3, diagnostics1. These total88; each open
comparison retains its own failure mode, bounded evidence and prerequisite.

Previous launcher, native Black Zero and artwork milestone (2026-10-22):
F76 passes on unchanged candidate89E80004. **144 passed,4 failed,86 unverified;
90 open of234.** F63/F95/F99/F181 remain failed. Current buckets below are
recomputed from each row's first P-code; older milestone summaries are historical.

Actual bundled recomp-ui opens the co-op detail pane with ten coherent labels.
Real dropdown/checkbox clicks choose Local Split, Both, automatic forms, scaling
and reduced shared cooldown; PLAY persists the six changes. The game enrolls
native FA4/5 without enhancement grants and renders both local panes. Seven
actual Both-door checks pass, including single-arrival waiting, owned menus,
reversal, full-box crossing/native closing and retained private resources.
All1430 draw restoration checks are clean. A fresh launcher reloads the same
choices, and its scrollbar exposes lower door/boss controls at1120x860.
This verifies discovery/persistence and representative runtime behavior, not
every setting's mechanics. The first observer used a nonexistent cooldown key;
the declared boss_cooldowns correction continues the same healthy game.

F37 gains independent current native Black Zero mechanics on a real F9 card:
one earned R2 action3A costs48 =4*12, energy288 ->240, with FC0, native recovery
and own Right2.5px. An original hit takes native damage5 ->2 and HP42 ->40;
native facing40 becomes hurt direction and original hitkind2 clears motion.
Natural hurt/feet/immunity recovery is observed. Fresh post-injury Left is
interrupted by another real hit24 ->22, so that movement check stays unverified.
No second injury/action, pose repair, inventory grant or extra variant.
All174 saved observations preserve genuine4A0/C9F9/capsule0/program0 and both
source/private cards stay unchanged. Special contact31E5C, enhanced attacktype3,
deferred producers, specific preview/capsule and rewind/rollback remain open.

F55/F71/F149 source review separates special capsule8's real pending4B=1 grant
from the later native report commit4B=2/4A08. The authored raw18 pod is located,
but safe lower-route access/P2 admission/report and equipped art/effects are
not established. Ordinary capsule5 evidence cannot substitute. Defer this
bundle; no guessed floor, direct unlock bit, capsule state or success write.

Retained-image review inspects16 exact captures without new runtime variants.
F18 continuous fades/effects, F16 identified exhaust ordering and F133 distinct
vehicle death presentation remain unverified; ordinary X/Zero death subchecks
stay qualified. subagent-retained-visual-obligations-20261022/receipt.json.

README and local playtest guide now carry all ten controls, current unchanged
gameplay/menu captures, form behavior and the actual UI scope. Existing license
terms stay unchanged. F67/R1 remain partial: actual control pages show stock
bindings and explicitly report no gamepad; physical distinct-device input and
owner-facing window/audio are unverified. Automated launches stay hidden/muted.
F1/F32/F40 actual UI Host/Join/START remains deferred: current LAN entry emits
BROWSE, hosting emits BEACON, binds are not loopback-only, and ROOM requests a
public IP externally. No LAN room/service/sign-in was started. Do not replace
this missing frontend boundary with existing CLI agreement evidence.

Previous post-door artwork check (classification superseded above): F95 was
marked failed on89E80004. A fresh
original title-code Ultimate X (automatic Ultimate off) plus Black Zero repeats
the raw4 noisy red/green background and black strips after actual Both passage,
native class34 intro/HP fill and control release. Before-door art is normal.
Automatic Ultimate is therefore not a necessary cause; Black Zero/co-op/native
resource-loading attribution remains unresolved. The initial35s approach observer
expires while the game advances; one longer same-encounter continuation completes
without reload, position, palette, phase or success writes. Original checkpoint
setup bypasses the preceding route; manual level1/HP54 differs from the retained
automatic level3/HP56 encounter.

The retained2184-command GPU frame, exact layer/map fields and live texture/CLUT
reads precede any native pause. Sampled CPU/FBO pixels agree and whole-VRAM diff
is zero;6118 clean restoration checks do not imply correct artwork. Raw1's
separate missing heroes are explained by cameraY240 versus bodyY75/74 and the
existing y<-144 cull. No character-cache purge or speculative graphics fix.
Evidence: qa-post-door-visual-20261022/manual-control/qualification.json and
live-vram-followup/receipt.json. The owned runner exits without forced kill
after quit reports busy; do not infer a successful acknowledgement or hang cause.

Evidence: qa-launcher-settings-20261022/qualification.json,
subagent-black-native-mechanics-20261022/qualification.json,
subagent-native-earned-armor-20261022/receipt.json,
subagent-product-docs-20261022/qualification.json and
subagent-launcher-lan-source-20261022/receipt.json. All owned runners stopped;
no production/SDK/build change, publication or tracker sync.

Previous automatic forms milestone (2026-10-22), candidate89E80004:
F37's missing implementation is replaced by independent default-off Ultimate X
and Black Zero Cheats and host co-op settings. It moves **failed -> unverified**;
no individual comparison closes. **143 passed,3 failed,88 unverified;91 open of234**.
F63/F99/F181 remain failed; the primary bucket counts are unchanged.

Native X starts in Fourth Armor1, so A121's base0-only automatic policy failed.
The corrected option uses effective native X rosters0..4 -> Ultimate4, including
the selected preview and seven original Gaea predicates. Native selection,
unlock/capsule grants and card bytes stay genuine. Three immutable host
DAT88/90/93 sources share the original two guest banks; private body/UI cache
identity and schema7 snapshot binding retain the source. No third guest bank,
native progress grants, SDK change or old-state migration.

Current automatic X/P1 and Black Zero/P2, native Nova/independent partner input,
guest-menu save/load/restoration and standalone FA4/fullDAT93/native Right pass.
Actual raw4 Both crossing, native boss admission, positive1HP setup plus real
buster final hit, paired victory and original reward/Save/selector return pass.
A fresh process loads that native card: genuine4A remains01 (08/10 clear),
earned mask8 and both weapon masks8 survive, native X cycle0->4->0 and guest
menu work, and actual forms remain4/5. The saved card is unchanged after Load.
The retained A121 inverse passes private DAT93/GPU UI, Black Zero palette/owned
Saber, actual P2 Nova/P1 jump and warm schema7 menu restoration; it retains its
original candidate identity. A121 option-off genuine code/Nova also passes.

Current combined local peers pass13 checkpoints/271 matching core+mod samples
through8724 despite opposite standalone choices, with1816 clean restoration
checks each: automatic host forms, arrivals/HUD, attacks/effects, menus, Nova,
default hints, native train wraps and P2-owner fatal/recovery. All21CTest checks
pass. No WAN, actual rewind/rollback or every Black Zero modifier claim.

F37 remains unverified for one injury/contact cluster, a naturally earned Zero
resource action, enhanced attacktype3, distinct deferred producer ownership,
specific preview/Gaea/capsule behavior and mapped rewind/induced rollback.
Standalone projectile emission has only a wrong-pool observer, so it stays
unverified without a third correction. Ordinary overlay interpreter fallback
in isolated standalone staging is disclosed. Retain all failed receipts and
defer unavailable setups; do not expand these into seat/character matrices.
Evidence: qa-auto-forms-20261022/qualification.json.

Previous story artwork and aerial recovery milestone (2026-10-22):
42085F25 closes F84; the qualified 993C8CA6 combat run closes F152.

The original raw18 door sheet selector reads the temporarily projected hero.
Under P2 Zero it requests DAT140, corrupting the X campaign's class41 opponent.
The native X branch requires DAT141. Extend the existing guarded campaign
predicate correction to original56848 and configure/generate that native hook.
Keep the original asset requests, authored replacements, actor, palette and
scene timing. No campaign/save bytes, new serialized state or SDK changes.

Current X-campaign admission, dialogue and live release match all35 expected
texture blocks and all9 authored frame1 tiles from the correct native sheet.
The NPC is coherent in both composed views. One actual Both crossing,30 native
pages, HP fill and both controls pass. The inverse Zero campaign also passes
one crossing,35 pages,50HP and both controls with coherent X/Lifesaver art.
Its direct live140 texels remain unverified: a paused-menu0/64 comparison is
retained as invalid residency evidence. Original-off encounter setup failed
after its bounded correction; its pristine35-block preload is setup evidence.
No actual story defeat/ending, awakened variant, menu reload or story-online pass.

F152 now has an actual rising Zero/P2 body31/type1B accepted hit, followed by
a ready X/P1 hit during P2's private recovery and natural expiry of both clocks.
Original32188 contact, F9A00 recovery and unrelated native AI stores are retained.
An overly strict32/36 observer and wrong-facing P1 miss are preserved; one
source-backed correction passes. F63/F99/F181 stay failed for broader gaps.
F130/F131 stay unverified after one corrected fresh native entry: normal7 was
selected, but a ledge-height muzzle setup lost P2 before fire. No third variant.

All21CTest pass. The first hook-coverage check correctly rejected an omitted
config site; corrected config/generation changes only ignored native shard23.
Same42085 combined peers pass286 unique core/mod matches through9184 across12
checkpoints, including actual train clocks/wraps, Ultimate/Nova, arrival/HUD,
attacks/effects, menus, default hints and native P2-owner fatal/recovery.
Private restoration1803/1804 checks has zero mismatch/watchdog/VRAM/device faults.
This is local same-build regression; the story image branch was checked locally.

**143 passed,4 failed,87 unverified;91 open of234**. Primary buckets:
combat24, environment24, controls16, doors10, netplay8, vehicles3, story3,
menus2, diagnostics1. Every open comparison retains individual evidence.
Frozen15-source X4 reference and SDK unchanged. All owned runners stopped.
Local commit only; no publication, remote synchronization or old-state migration.
Evidence: qa-story-art-source-20261022/qualification.json,
subagent-air-cooldown-20261022/qualification.json, and
subagent-story-art-regression-20261022/qualification.json.

Previous spatial-scroll milestone(2026-10-22):993C8CA6 closes F175/F189.
Original raw1/0CP0 reserved train controller advances independent8/4 spatial
origins and native wraps while heroes stay still. Baseline952 advances P1
but freezes P2. Publish the original final wrapped X origins and refresh
flags before the extra mode5 camera update. Guard actual stage/header/code/
authored controller; retain per-view Y/history/bounds and exclude F7's
camera-delta branch. No new clock, hook site, state, setting or SDK change.

Current native150/152-world windows show both saved phases aligned, actual
panorama wraps and moving composed scenery. Short native P2Right/release
settles X233.5 versus stationary P1 X160, cameras0/73, both feet/HP32;
this qualifies nearby independent views, not a distant route. A26-tick
observer bound at safeX229.5 is retained; same-encounter continuation uses
the settled position without another setup variant. Same-candidate raw17
atlas cycle also passes6/6 changing backgrounds and50 publications, with
four native palette timers retained. F175's two distinct mechanisms now
have current local evidence. F95's visible black sky window stays open.

Current21/21CTest and280 matching core/mod samples through9056 across12
peer checkpoints pass together, including actual replicated native CP0
train clocks/wraps, Ultimate/Nova, paired arrivals/own HUDs, attacks/effects,
menus, default hints and actual P2-owner fatal/recovery. Final private
restoration1816/1818 checks has zero mismatch/watchdog/device/VRAM faults.
One overstrict per-read clock observer failed and is retained. Diagnostics
can straddle one update; the corrected observer bounds per-read phase AND
cumulative drift to one native update, preserving real speed/wrap checks.
Both peers then observe151 world updates and152/151 native clock steps,
each with both actual wraps. No atomic-read, WAN or atlas-online claim.

Individual partials stay explicit: F78 distant progression, F121 continuous
scroll after P1death, F28 boss-room animation and F95 art completeness.
F166/F187's real D/E opening damage still fails setup before full defeat/
CP4; one lower-port correction causes native P2fatal. Legacy18 reaches
original Chaser CP0/controller, P2FALLEN and6px native camera progress,
then the survivor hits a road obstruction before the endpoint; actual
handoff stays unverified. No further positioning variants. F37 resolves
original DAT93 resources/exact graphics length but automatic forms remain
absent; future virtual settings must preserve native save progress.

**141 passed,5 failed,88 unverified;93 open of234**. Primary buckets:
netplay8, controls16, environment24, doors10, combat25, vehicles3, menus2,
story4, diagnostics1. All93 open reports have individual evidence. Frozen
15-source reference and SDK unchanged; all owned runners stopped. Local
only, no publication/tracker sync or old-snapshot migration. Evidence:
qa-shared-scroll-fix-20261022/qualification.json and
subagent-scroll-controller-20261022/qualification.json.

Previous story-door and trail milestone(2026-10-22):95288302 closes F91/F12.
Actual raw18/0CP2 Zero/P1 and X/P1 story opponents initialize after both
body boxes clear and the native Both door commits. Each passes one crossing,
original dialogue/HP fill/release and independent native controls. Canonical
campaign selection permits wrong-campaign native retirement; class41 subtypes
0/1 share an initializer. F84 opponent-art corruption stays failed; no actual
defeat/ending, awakened runtime or story-online claim.

Ordinary X/P2 and Zero/P2 dash trails now use original common working CLUTs,
preserving immutable character palettes and native weapon/variant updates.
Actual native dash, live original colors, defined private-bank blue/red pixels,
composed captures, stationary P1 and native expiry qualify the bounded check.
Alternate forms, weapon/menu/rejoin and specific trail-online stay separate.

Current21/21CTest and276 matching core/mod samples through8896 across11
peer checkpoints pass together: paired arrivals/own HUDs, attacks/effects,
menus, default hints, Ultimate/Nova and P2-owner fatal/recovery. Private
restoration1655/1656 checks has no mismatch/watchdog/device/VRAM faults.
Prior failed wrappers remain. The35s host observation expired while actual
native positions continued; same-encounter continuation passed. Observer
deadline90s, game watchdog unchanged. No camera/map workaround.

Water legacy10/F115 gain actual native head-effect producer/origin/draw/
retirement evidence, but distant safe footing, P2 emission, visible glyph
identity and authored surface crossing remain unverified. Final rows
F136/F138..142 retain missing native door placement after one HP-only setup
correction; no fight/Continue/phase/ending pass. F67 gains two missing balance
controls in the guide; actual launcher walkthrough stays open.

**139 passed,5 failed,90 unverified;95 open of234**. Primary buckets:
netplay8, controls16, environment26, doors10, combat25, vehicles3, menus2,
story4, diagnostics1. All95 open reports retain individual evidence. No new
persistent fields, SDK/watchdog change, remote operations or old-state
migrations. Evidence: qa-story-door-boundary-20261022/qualification.json and
subagent-story-integration-20261022/qualification.json.

Previous native-death milestone(2026-10-22):0D73D4CB closes legacy3/F4.
Reproduced59E: DYING immediately adopts survivor view and native random
glyphs are culled outside living P1's pane. Keep native death view/camera and
visibility/lifetime bounds through DYING; AI/scene ownership stays native.
Original co-op-off Zero pose/gold-glyph control and actual DAT common images/
colors identify the private static painter contract. Preserve original SPRT
UV/drawmode and tile-local FT4 UVs; both on-foot character poses/colors now
visually pass. Intermediate blank/purple/partial-tile captures remain retained.

Actual ordinary X/P2 and Zero/P2: native44 random+32 radial attempts each,
40 source-color observations each, finite random retirement/radial draw flags,
post-FALLEN spectator fallback and survivor controls. Original radial slots
intentionally remain allocated. Actual Grizzly P2 approach/hidden passenger/
fatal adds exactly one native burst/handoff, retained resources/retries/boss
flags, original foot anchors and controls. First retirement observer counted
frozen dialogue as particle aging; one future-update correction passes with
367 actual world/native updates after control release. No60FPS benchmark claim.

Current21/21CTest and286 matching core/mod peer samples through9153
across11 checkpoints pass: native paired arrivals/own HUDs, attacks,
effects, menus, default hints, original Ultimate/Nova and actual P2-owner fatal/
passenger recovery. Separate ordinary distant-death peer capture hit setup
gaps after native Nova moves P1; two wrappers retained, check deferred and
unqualified QA flag removed. F133 remains open for remaining vehicle deaths.

**137 passed,6 failed,91 unverified;97 open of234**. Primary buckets:
netplay8, controls16, environment26, doors11, combat26, vehicles3, menus2,
story4, diagnostics1. All97 open rows have individual evidence. No new
persistent fields, SDK/watchdog/settings changes, remote operations or older
snapshot migrations. Evidence: qa-death-palette-breadth-20261022/qualification.json.

Previous shared-quake milestone(2026-10-22):59E33933 closes F197/F198.
Reproduced80ED219B with original raw0CP1 native falling-slab/debris/impact-blast analogue: P1 Right stops at contact09/X3013.5625; source-backed native jump crosses original X>=3048. Both composed nearby panes show native debris/impact smoke, but P2 saved quake flags/timers/amplitude remain zero while primary horizontal24/amp3 and vertical32/amp4 run. Fixed59E33933 publishes only the original2A5C4/2A600 final command fields at guarded native stores2A5FC/2A638; each camera retains its native timer/phase/update. No copying an already-ticked timer, custom clock, palette/flash change or new persistent state. Configured and regenerated instruction sites; first omitted-coverage build/test failure retained. Current native approach/jump passes both axes: P2 X2760/2763 in17 active observations and Y960/964 in25, exact shared timers/phase/amplitude, two original commands total, native quiet retirement and P2Left-21.5. Both composed axis images inspected: original debris and impact clouds are shared and own HUDs remain. First shortened direct-position fixture never reached the trigger; restored proven native approach on the SAME candidate, failed receipt retained. Current full peer regression retains native Ultimate/Nova, paired beams/own HUDs, attacks/effects, menus, hints and P2-owner fatal/recovery: 282 matching core/mod samples through9056 across11 checkpoints, five shared commands per peer,1663/1663 clean restoration checks. Specific nearby effect not replayed online; no every explosion/distant flash/route/damage or rescue handoff pass. qa-shared-explosion-breadth-20261022/qualification.json.

F54/F158/F176/F196/F202 gain individual source/setup evidence and explicit
remaining requirements. The original intro boss record/checkpoints/structural
placements were checked directly against EXE bytes. Native CP1 fixes Y960,
so its quake recovery cannot qualify a varying-height slope/handoff. This
global quake API is not an established local palette-flash producer. Native
P2-led rescue/progression and an actual Y-triggered closing wall remain
unverified. These five are not passes or whole-game absence claims.

**135 passed,6 failed,93 unverified;99 open of234**. Primary buckets: netplay8, controls18, environment26, doors11, combat26, vehicles3, menus2, story4, diagnostics1.
All99 open entries have individual source or runtime evidence;93 remain
unverified.21/21CTest passes. No SDK/watchdog/option/persistent-state changes,
remote operations or save-state backward-compatibility work. All owned
runners stopped. Evidence: qa-shared-explosion-breadth-20261022/qualification.json.

Previous native-Nova milestone(2026-10-22):80ED219B fixes F65.
Reproduced on013A530E with actual original title-code Ultimate X/P1, native R2 admission to action22/FA4: P2 camera X498->16 and controls stop. Native3E390 and frozen X4 lifecycle prove this personal attack reuses PLAY1C. Fixed80ED219B: only native Ultimate action22 bypasses shared-scene fallback; P2 body/camera private projections clear the attack lock locally and leave_second restores its canonical ownership. Other Giga actions24/25 and actual scene ownership retain native behavior. Fresh local original-code case passes P2 native jump16.664px during Nova, separated cameras and increasing verified private draws; native Giga288->288 and original idle/lock release. Actual two-peer input-only native-code/separation/Nova case passes each P2 jump, separate cameras/private draws, energy and recovery. Same-candidate combined native arrivals/own HUDs, attacks, effects, menus, default hints and P2-owned fatal/recovery regression passes 288 matching core/mod samples through9248 across11 checkpoints; final clean restoration 1645/1647. Both peer arrival/own-view images retained; no WAN/otherOS/every special attack claim. Earlier rightward observation hit native side contact09; reused post-Nova setup lacked camera gap. Both wrappers retained, fresh jump correction passed. First peer batch passed actual Nova but later walking fixture failed after separation; reordered generic checks before that setup. The next observer admitted Nova while P2 was still in original hurt11 on the floor; require native idle recovery as well as landing. Both failed wrappers retained; same candidate final bundle passes. qa-native-forms-20261022/qualification.json.

F37 remains failed: no automatic simultaneous Ultimate/Black Zero options or
qualified enhanced X/P2 resources. Actual original title code/rosterFC0 and
native ability are prerequisite evidence only; no equipped counterpart,
completed capsule program, native persistent-save or movement-effect pass.
No new persistent state, watchdog changes, SDK edits or remote operations.

**133 passed,6 failed,95 unverified;101 open of234**. Primary buckets: netplay8, controls18, environment28, doors11, combat26, vehicles3, menus2, story4, diagnostics1.
94 open rows have evidence;7 lack it.21/21CTest passes.
Evidence: qa-native-forms-20261022/qualification.json.

Previous paired-arrival milestone(2026-10-22):013A530E repairs F20's actual
zero-spacing/late-enrollment failure. Native3D72C/35468 proves action0 with
personal67=FF is the original incoming phase. Enroll there rather than waiting
for P1 idle; retain one original incoming/resolving animation for P2. Distinct
stage phases3/4 follow the real floor and preserve safe spacing through resolve.
Bounded original2FB58 standing probes reject crush, hazards and HP loss.
Current shared-support geometry can supply a floor only with the candidate's
own box/feet inside a live, contacted, positively carried native support.
Incoming as well as resolving phases qualify after native world contacts;
no extra animation/world update runs at that boundary. Ordinary Select and
native mounted/chaser policies retain their distinct paths.

Reproduced6C baseline: P1 falls while unenrolled; afterward bothX160/feet432.
Final native6/0CP0: both original beamsX160/184,Y240,24px safe spacing,
feet432, both original jumps/landings/controls. Real2/1->3/0 water-opening
regression passes four native jump/landing checks; actual terrain chooses
safe feet at distinct X. Original6/0->6/1CP0->CP1 checkpoint support passes
24px spacing, one enrollment, original phase3/contact/carrier, both jumps and
independent movement. Initial C0C963C7 support stall was caught and repaired.
A final checker sampled an earlier compound observation without final support;
the SAME native encounter continuation requires the actual support/unlocked
controls and passes. Failed wrappers remain retained, not overwritten.

Actual mid-beam save/load and fresh-process pre-enrollment load restore P2
JOINING and complete with the same24px spacing/feet. Released-input boundaries
then prove each own control3px with other stationary. The first cold checker
switched seats before release settled; its failure and native input/velocity
receipts are retained. The existing serialized phase byte now accepts3/4,
with no new persistent field or migration. A separate fresh X campaign
qualifies its distinct native Fourth ArmorFA1/ZeroFA5 initialization path:
24px spacing, both feet1152, independent movement/attacks. That helper did not
capture its initial beam; concurrent-beam proof is the named Zero/P1 case.

The actual new arrival also exposed shared HUD fallback. Private presentation
now accepts only the paired original on-foot incoming phase; actual shared
scripts retain their gates. Local composed and real Split peer captures show
own Zero/X HUDs during BOTH native beams/PLAY1C=1/scene0/P2JOINING. Own-view
commits13/16 and12/16 restoration checks already occur within that phase.
F65 remains unverified for the distinct camera-moving special attack.

Final same-candidate21/21CTest passes. One combined native peer batch passes
250 unique core/mod CRC samples through8075 across10 checkpoints, including
both attacks, effects, menus, hint suppression, P2 scene-owner fatal/recovery.
Final restoration1125/1126 has no mismatch/watchdog/device/VRAM/host-state leak.
Ordinary Select regression retains one start,219.5px leader movement,HP17/
ammo73 and no reload. Controlled mounted return retains both controls
(P2+15.5px/P1Armor+15.125px), native footbox/floor and occupied vehicle.
Actual mounted boss entry/unsafe forced dismount remain unverified.

**132 passed,6 failed,96 unverified;102 open of234**. Buckets: combat26,
environment28, controls18, doors11, netplay8, story4, menus3, vehicles3,
diagnostics1.95 open rows have evidence;7 still lack it. Named native arrival
families, not every authored spawn/route/vehicle/pixel sequence or launcher/
WAN/other-OS qualification. All owned games stopped. Evidence:
qa-paired-arrival-20261022/qualification.json. No SDK/watchdog/option changes,
remote operations or save-state backward-compatibility work.

Previous paired-view breadth milestone(2026-10-22): unchanged6C4A3B54 closes
F43 controlled Split performance and F94 native layered-background isolation
with one shared opening-road setup. Both actual CLI peers boot original X/P1
and Zero/P2, take ordinary independent inputs, restore both menus, separate
their native cameras with6 original enemies active, and rejoin without cheats,
loaders or state writes. Default diagnostics and hint suppression remain on.

Ordinary performance uses the normal QA-hash-off presentation setting,
BIOSHLE true,1080p and original cycle scale; no turbo/frame interpolation.
Two6.03s samples measure world59.838Hz overlapped and59.817Hz separated;
peer VBlank/net cadence remains59.84..60.00Hz. Cameras5/486, both alive,
intact visually inspected original scenery/HUDs;204 unique core/mod samples
match through6563. This is a bounded representative workload, not every room/
actor/host or a universal speedup. The verifier run is not used for speed.

One separate same-candidate restoration qualification repeats the native
separation/rejoin path. Cameras3/500,6 live enemies and distinct original
layered art are inspected in both peers; full504-byte saved private camera and
252-byte canonical-camera receipts are retained. Readbacks/captures are
asynchronous, not atomic comparisons.180 unique core/mod samples match through
5841;1507/1508 restoration checks have zero mismatch, watchdog, device/VRAM
faults or private-host-state leaks. The existing shared peer helper now retains
these camera bytes instead of dropping them after origin extraction.

L1 remains unverified for actual launcher/controller/other-OS behavior.
F30 actual startup pointers/log headers/LAN startup are retained, but no real
Discord sign-in failure or service connectivity is claimed. F65 ordinary
post-arrival own HUDs/cameras pass subchecks, but HUD during concurrent arrivals
and camera-moving special attacks remain unverified. No later intro CP3 or
special-form setup variants were attempted.

**131 passed,7 failed,96 unverified;103 open of234**. Primary buckets:
combat26, environment28, controls19, doors11, netplay8, story4, menus3,
vehicles3, diagnostics1.96 of103 open rows have individual evidence;7 still
lack it, down from12. All four owned peers stopped in harness cleanup.
Evidence: qa-view-breadth-20261022/qualification.json. No runtime rebuild,
SDK/watchdog/serializer/options changes or remote operations. Prior current
21/21CTest qualification remains applicable to unchanged executable6C4A3B54.

Previous native mounted-scene milestone (2026-10-22): current6C4A3B54 repairs
a real passenger-return blocker reproduced on FBE91E18. Native mounted P1
action12/personal67=1 has no on-foot box/ground; the old phase3 gate keeps a
living P2 hidden after original stand/release. The fix requires one live,
grounded, occupied native P1 Armor and uses its current box/coordinates,
then bounded same-height original2FB58 standing-body probes. Missing floor,
opposing terrain contacts, hazard79 and HP loss reject the landing. Full body
and shared collision scratch are restored after probes; native animation
and world updates still commit the actual return.

Final candidate6C4A3B54 actually completes native incoming to scene0 with
X/P2 footbox800748D0/floor8, P2Right+15.5 and occupied P1ArmorRight+16.5;
world/native increments both17. Both heroes remain alive and the vehicle
remains occupied.656 local restoration checks have no mismatch/leak/watchdog.
Native earned6 Load precedes the test. CP1 has no Armor; the one correction
uses original4/1CP0 and position/history-only movement of its existing Armor
under native pause, followed by native mounting. HP-only heroes/contactgrace
off; no mount, scene, success, defeat or reward flags are written. Controlled
stand/release is a mechanic check, not actual boss admission.

F148 and F100 stay unverified for actual mounted boss entry and forced
dismount/unsafe geometry respectively. Online ordinary command7 mistakenly
selected Chaser; one source-corrected command8 attempt reached4/1 but never
completed native mounting. Both receipts and failures are retained; no actual
online mounted-return/unsafe-floor pass and no third setup variant. Unused
temporary QA inputs were removed from the final candidate.

F71/F149 gain individual source/receipt reviews: counterpart X selects base
roster/regular assets; actual program40 acquisition/Save is not equipped-form
palette or movement-effect evidence. Enhanced-resource reuse and the separate
native special-form path remain unresolved; no whole-feature absence claim.

**129 passed,7 failed,98 unverified;105 open of234**. Buckets unchanged:
combat26, environment29, controls19, doors11, netplay9, story4, menus3,
vehicles3, diagnostics1.93 of105 open rows now have partial/source evidence;
12 still lack it, down from15. Final21/21CTest passes; one current combined
loopback pair passes267 unique core/mod CRC samples across10 checkpoints
through8611, including hint suppression, both attacks/menus/personal effects,
P2-owned scene, owner fatal and survivor release. Final renderer restoration
checks1144/1148 have no mismatch/leak/watchdog. This is same-build generic
CLI loopback evidence, not targeted mounted online, launcher UI, WAN or other OS.
All owned games are stopped after receipts. Evidence:
qa-mounted-entry-breadth-20261022/qualification.json. No SDK/watchdog/options/
persistent serializer changes. Everything remains local.

Latest grouped native melee milestone (2026-10-22): unchanged FBE91E18 closes
F132/F153/F157 in one actual class32 Both/Split/scaling-per-player encounter.
Real earned6 Load, original final door/intro and fullHP precede controller-only
attacks. Ordinary18/19/1A occur while boss rises; only18 accepts, then a native
Left/Right separation/fresh18 proves F157. Native31F98/31F6C token acceptance/
clearance and actual serial/remainder changes are retained, not inferred
from integer HP loss.

One correction waits the original grounded boss window without teleport or
phase edits. Native18/19/1A accept raw2/2/5 at32188, HP55->54->53->51 and carry
6->4->2->2. F9A00 sees original19/1A/1B, short native2/native30 branch with
per-player native minimum1; own first1 and finisher15 are sampled. Second
short window falls between stable reads, already0 next update. This qualifies
F132; no full/held/rejected-contact policy blanket pass.

An ordinary incoming hurt stops the first helper after the real finisher;
its overallfalse/error is retained. Continue SAME encounter: own15 expires0,
native hurt releases to idle atworld13423, then first fresh Square accepts
18/raw2/51->50 at13430 and serial6->7. World13401->13421 includes native
hitstop; it is not15 unconditional world updates. This qualifies F153's
finisher policy separately from normal hurt/control delay.

The added F152 air/readypartner bundle reaches no native32/36 aerial attack
or accepted contact in the ordinary hop and one shorter-hop attempt; observed
shots are residual standing18 only. Earlier rising-aerial/geometry labels are
not qualification. No actual ready-P1 contact was attempted. Keep F152 and
that F99 gap open; no more jump/aim variants. F63/F99/F181 gain actual class32
chain/carry evidence but stay failed for remaining supported-scope/policy gaps.

F145 and L14 source/evidence reviews preserve distinct original triggers:
transient missing P2 geometry under a closing hatch, and P2-first optional
area activation followed by room-camera lock. Verified X5 midboss admission,
attack-breakable class14 and capsule grant/rescan receipts do not cover them.
No verified hatch/upper-route recipe reached; defer both without guessed
coordinate/loader variants or game-wide absence claims.

**129 passed,7 failed,98 unverified;105 open of234**. Combat26, environment29,
controls19, doors11, netplay9, story4, menus3, vehicles3, diagnostics1.
90 of105 open rows have individual partial/source evidence;15 still lack it.
Evidence: qa-melee-breadth-20261022/{qualification.json,
hatch-and-optional-route-review.json}. No runtime/build/SDK changes; prior
21CTest/current combined270CRC qualification retained. This melee path is
offline, not an actual network-chain qualification.

Latest native weakness breadth milestone (2026-10-22): unchanged FBE91E18
closes F127 using the actual Chaser-stage boss and native earned Zero/P2
airdash. Original type22 private shot retains parent8009A0A0; native32188
writes56->53, same-frame F9A24 receives result23 and enters weakness phase0B.
Actual recovery exits to4/7, P2 returns native1/2, both alive/scene0; both
composed panes inspected. No boss/phase/ability/result/success flags seeded.
Route/checkpoint bypass and HP-only refill remain explicit; no vehicle-route,
defeat/reward or actual online weakness pass.

The native dialogue was advanced in the same encounter with scene-owner P2.
Native23EFC/23F1C stores prove accepted Cross; two subsequent pages finish intro.
P1 text timeout was wrong-owner observation, not a confirmed game defect.
Original75s entry timeout and initial GameOver/corrected entry are retained.
One helper fixture timed out while native-paused before any attack; resume
ordering corrected in the same encounter, without another loader or replay.

F155 fire-overlap stays unverified after one ordinary unsafe-anchor setup
(crush before firing) and one observed-floor correction. P1type20/P2type4 and
finite effects occur while both remain alive; asynchronous pool observations
are not an atomic simultaneous hit. Mainboss65->65, last7F targets a child.
Charged/deferred matching effects and accepted weakness remain missing; defer
without more fire setup variants. F135 reuses prior native Wing Spiral receipts
but retains the distinct wall/surface collision and actual parent gaps.
Generic weakness acceptance is not a pass for that different obligation.

**126 passed,7 failed,101 unverified;108 open of234**. Open cells without
individual evidence21->18;90 of108 open rows now have partial/source evidence.
Primary open buckets: combat29, environment29, controls19, doors11, netplay9,
story4, menus3, vehicles3, diagnostics1. Evidence:
qa-ice-weakness-breadth-20261022/qualification.json,
phase-specific-comparison-review.json and
qa-fire-overlap-breadth-20261022/qualification.json.
No runtime/build/SDK changes; retained21/21CTest and current combined270CRC
qualification remain unchanged. This native weakness itself was offline.

The same native encounter also completes an actual Zero/P2 ice finisher,
paired victory, report/rewards, private-card Save and selector return. After
one disclosed positive53->1HP setup, original32188/type22 writes1->0. Both
heroes celebrate once, boss retires, report commits only06->16; native Save
checksum6088/card2 and source card unchanged. Both victory and selector
images inspected;4944 renderer restores match without watchdog/VRAM leaks.
F134 gains this current owned-ice-final-hit supplement; its existing pass and
the ledger126/7/101 remain unchanged. No full route/cold-new-card Load/online
claim. qa-ice-weakness-breadth-20261022/native-final-save-qualification.json.

F86 source/evidence reconciliation also preserves the original dead-partner
prerequisite: X4 carries a corpse from the prior boss across a fresh fortress
level, not a generic healthy hub arrival. Tighten that individual failure/check
against current reset/spawn-policy source. Existing84EDFD5A healthy native
refight/return controls are partial only; actual dead-partner fresh-fortress
progression stays unverified. No new runtime/loader variants or closure.
fortress-dead-partner-scope-review.json. Open evidence cells18->17;
91 of108 open rows now have individual partial/source evidence.
Ledger stays126 passed/7 failed/101 unverified.

Latest shared-atlas repair (2026-10-22): FBE91E18 fixes the actual raw17
living-P2 frozen background. Source/native writes identify one twelve-frame
atlas producer, BIN27 EF7E0/EF800 origins plus EF7FC refresh. The old private
mode5 camera kept stale origins until P1 died. Publish just these fields before
the existing extra camera pass, with stage/section/header/mode/opcode guards;
no duplicate clock, new state/schema/site/option or SDK edit.

One grouped native before/after-P1fatal session qualifies F113's shared-atlas
analogue and F120's survivor palette uploads. Baseline living panes6/1 distinct
backgrounds become7/7, survivor8/8, with both sampled origins always matching.
Post-death-orb tail3 remains3/3. Actual survivor Right+41.5 retains ground.
Direct source-derived256x32 GPU palette mapping confirms5/4/4/3 changing rows
and6/4/5/5 stable exact source matches; an initial wrong-stride observer and
one original-source correction are preserved. Four native timers each have
207/207 and194/194 once-per-native-frame decrements before/after death.
4780 renderer restore checks have zero mismatches/leaks.

The frozen F175 report contains both an atlas and a distinct scrolling
controller. Its atlas fault is repaired; move failed->unverified rather than
closing the whole report. F121 and continuous scrolling remain unverified.
**125 passed,7 failed,102 unverified;109 open of234**. Blank cells24->21.
21/21CTest pass1.22s. One current combined Split peer regression passes270
unique matching core/mod CRC samples through8672, resolved8698 both; owned
peer PIDs reaped. Both menus/attacks/glow/P2 scene-owner fatal/release and Alia
suppression are covered, but stage17 atlas behavior was not exercised online.
Evidence: qa-shared-atlas-fix-20261022/qualification.json.

Latest comparison-reuse milestone (2026-10-22): no new runtime/build. Frozen
F193 says the other Slash Beast train starts; its inferred same-controller
stop/restart prerequisite was unsupported. Correct that failure-class/check to
P2-only shared starting and retained ownership. Original X4 train owner code
and existing immutable native X5 CP3 baseline/fixed receipts qualify the same
failure: P2 crosses3617 while P1 stays3488, corrected native class12/sub2 starts
phase7, carries upward64.5px and changes only P2 cameraY1200->1135. World/native/
co-op clocks each advance158; sampled frame156 is not atomic VBlank proof.
Both retained Split images inspected and original/prior hashes verified.
F193 is bounded passed on named C0DF29A5, with no new current2DB qualification.
F192 stays unverified: an automatic lift endpoint is not a P2-triggered shared
STOP command. **123 passed, 8 failed, 103 unverified; 111 open of234**.
Blank cells26->24. Evidence: qa-controller-comparison-reuse-20261022/receipt.json.
The original234 reports and train-stop obligation are preserved.

Latest private-camera retry milestone (2026-10-22): unchanged 2DB9581E closes
F170 in one original layered local Split section reached through a real P2-led
2/0->2/1 exit. First X/P2 native fatal leaves P1 moving +133.5 with no spent
retry; actual team wipe spends exactly2->1 and native arrival restores both
camera bounds. Both original jumps/landings and fresh Right+79.5 each pass.
Original map/metatile/descriptor hashes match before/after retry. Both composed
panes retain correct layered scenery, terrain and heroes;3028 renderer restore
checks match with zero mismatches/leaks. One observer pointer error was corrected
from the renderer source in the same encounter before death, without a reset,
replay or loader. GPU palette samples vary during native time/fade and are not
claimed bit-identical; full death-effect color/cleanup L3 remains unverified.
**122 passed, 8 failed, 104 unverified; 112 open of234**. Blank cells27->26.
Evidence: qa-private-retry-breadth-20261022/qualification.json. No code/build/SDK
changes or new configuration matrix; private cameras required this actual Split
check after the distinct Unified case.

Latest bounded breadth checkpoint (2026-10-22): unchanged 2DB9581E retains
five individual comparisons unverified. L23/F172 source preparation identifies
native offensive orb allocation and initial velocities, without qualifying
continuous homing or the shared assessment producer. F73 reaches a real wall
before the tether/recovery condition. F102 records an actual P2-led original
2/0->2/1 exit, both native jumps/landings, P2 +151.5 with P1 stationary, camera
+59 and recovered P1 control, but the background is static and never exercises
the original dead-P1/stale-region callback. Its right-side black area aligns
with native empty background tiles beyond source X336; no new ownership fault
is established. F161 retains intact native enemy/attack art after that actual
transition; one ordinary shot and one short native approach correction do not
qualify accepted contact. Defer these missing mechanics without more setup
variants. **121 passed, 8 failed, 105 unverified; 113 open of 234**. Blank
individual evidence cells 32 -> 27. No runtime/build/SDK changes.
Evidence: qa-camera-area-unified-20261022/qualification.json and
qa-orb-source-breadth-20261022/receipt.json.

Latest preinit-survivor color milestone (2026-10-22): unchanged2DB9581E
closes F203 bounded class32 sharedpalette/child-init analogue. P1fullyFALLEN
world3784 while nativebossABSENT; actual P2door then initializedboss3970.
Original2C338/BIOSmemcpy writes128 sourceS30 bytes into canonicalS28+2780;
all64 source/working/liveGPU words match at192,499, nativeclass1E children
inheritCLUT7CCC (890 sampledobservations), bothpanes visually inspected and
actualP2Right+13.5. No phase/palette/result flags seeded. The internalF96D0
fn_entryfilter emitted0 despite actualcopy; its observerassertion is retained,
and SAMEencounter continues using authoritativeNPCorder/nativebytewrites.
Do not compare distinctframe clocks or require a diagnostic build.

The samefight F81 flash subchecks are partial: native6->0, original93DD0
mask writes, visiblebackground brightness/recovery in bothpanes and actual
P2Right+9.5/+7.5 pass. Protectedboss rows316..319 and one source-mask-guided
rows4..7 correction show no sampledGPUwordchange. Activepalette/upload
ownership stays unverified; no rendererfailure/absentflash inferred. Stop
variants at the boundedlimit. No new source/build/SDK, roster or networkcases.
**121 passed,8 failed,105 unverified;113 open of234**. Blankcells34->32.
qa-survivor-electrical-20261022/qualification.json.

Latest two-door milestone (2026-10-22): unchanged2DB9581E closes F83/F164
together in actual localUnified/Both raw5/0, authored class8/sub5 doors8368
then8656. Completedfirstphase4/nativebody/locks/cameras carry into second;
there is NO loader, teleport, fixture or state reset between actualcrossings.
Exactly2starts/2crossings/1intro-without-warp, no newabort or re-enrollment;
both originaldoors end2/0, HP13/17 stays unchanged, actual class32 native
intro/HP65/control releases and fresh independentRight moves2.5px each.
Untruncated31-entry originalanimation trace has no outgoinganimation3.
Composed Unified frame inspected; private-view renderer/network notclaimed.

The ordinarybackward CP4setup8360 overlaps closedbarrier at its nativeclamp
and killsboth. One setupcorrection extends current/targetleftmin32px to8320,
places pair8336 on observedfloor, then lets FIRSTnative door commit original
8352; second commits original8640. This firstapproach/camera/route is bypassed,
notpassed. The firstdoor closing overlaps secondopening, so an observer
waiting firstretirement plus oldphase missed its earliercommit and asserted
at second8640. Preserve that observerfailure, qualify SAMEactual encounter's
nativeintro/control continuation without rerunning. No game/source change.
F97midboss/F101actualnetwork retain their separate gaps.
**120 passed,8 failed,106 unverified;114 open of234**. Blank evidence cells38->34.
qa-two-door-unified-20261022/qualification.json.

Latest breadth checkpoint (2026-10-22): unchanged2DB9581E reaches raw4/0CP2,
both actual native26px jump/landing controls, and authored class0C/sub0 exit
solid HP32/action1. Native P2 approach runs with P1 behind. Ordinary Square
plus one nativeDown+Square correction after positiveHP1 shortening do not
establish actual damage/destruction; no gateaction/exit/clear flags written.
The61update cleanup, actual4/1 transition and P1-dead destruction are therefore
unverified F77/F108/F186 prerequisites, deferred without a third variant.
Far position/camera/rescan bypass does not qualify route or composed scenery.
qa-breakable-exit-breadth-20261022/qualification.json.

Existing-evidence reuse separately adds individual F3/F36/F64/F68 receipts.
Local CP3 platform motion/draws do not pass LAN animation; hero carry does not
pass pickup carry; subviewport images do not pass distant wrapping; P2-first
approach with actual owner1 does not pass a P2-owned drop. No comparative
promotions: **118 passed,8 failed,108 unverified;116 open of234**.
Exact blank evidence cells fall45->38; these are recorded partial/deferred
checks, not newly passed comparisons. No new build or runtime source change.
subagent-x4-qol/untouched-stage8-evidence-review-20261022/receipt.json.

Latest held-recipient milestone (2026-10-22):2DB9581E fixes F126's actual
native Rosered wrong-body periodic damage and stuck P1. The scoped existing
AI target override retains one unique living reaction3 victim through native
negativeCA damage commands, escape input and original release. On preserved
real capture through unchanged native loader:76 corrections, P1HP42->36,
P2HP20 unchanged, seven negativeCA observations,15 fresh native escape edges,
CA0 release and real P1Right+5.5. No capture/phase/success fields seeded.
The fresh P2-capture attempt caught P1 first; L24 stays individually unverified
and setup is deferred, rather than adding variants. F72 death/multiple-capture
and actual online-grab checks also stay open. No old-state compatibility work,
schema/option/site change or SDK edit.21/21CTest and 268 unique matching
core/modCRC samples through8608, resolved[8624, 8623], owned pair reaped.
**118 passed,8 failed,108 unverified;116 open of234**. Qualification:
qa-grab-recipient-fix-20261021/qualification.json.

Latest hostile-grab finding: A093A836 reproduces F126 actual recipient
switch. Native child27 catches Zero/P1 withCA1/reaction3, then later samechild
3D010 consumes unheldX/P2 and transfers pendingdamage32->26->20->14.
Original P1 staysCA1 after childretirement; real12update Right dx0 while
bothALIVE/scene0. The first unsignedCA observer mislabels P2FF as caught:
FF is a negative damage/releasecommand, not a P2capture. Timing proves P1caught
before wrong-body damage, not unequivocally before positionwrite. L24P2 and
F72death stay unverified. **117 passed,9 failed,108 unverified;117 open of234**.
No grab/phase/success flags seeded. One actual held snapshot is saved; oldbuild
acceptance/migration is never required. Scoped source-backed recipient repair
is being reviewed; native reaction3 negativeCA presentation must be preserved.
qa-hostile-grab-20261021/baseline-qualification.json.

Latest bounded delayed-enemy breadth: unchangedA093A836 raw6/1CP2 both
native39.281px jumps/landings pass. F109 actual original parent120 holds while
both excluded; P1X3616 stays outside, P2Right admits countdown and THREE real
class12 emissions reuse80096D98. Address-only observer undercount is corrected
by original2D1A4/2D6FC traces; no source/game fix or fake pass. Children retire
early, source-consistent with offcamera cull; exact callcamera not retained.
One nearerRight correction hits nativewallX3669/contact09. Usable child
travel/contact stays unverified; defer actual upperroute/emitter-room setup
without a third variant. F200 frozen report requires distinct environmental
event-produced group; no blanket reuse. F182/F111/F160 keep separate identity
gaps. **117 passed,8 failed,109 unverified;117 open of234**.
qa-delayed-emitter-20261021/qualification.json. Next genuine hostile grab.

Latest native support-arrival milestone: A093A836 fixes an actual raw6/1CP1
P2 incoming stall and closes F34 as the bounded warp-on-support analogue.
Original class0E/highbit checkpoint support deliberately remains in native
phase3 with collision; old phase1-only guard rejected it despite exact current
feet/top2336 and native positive carry. Exact overlay/root/table-qualified
phase3 allowance preserves all current contact/geometry and retirement checks.
The same baseline native6/0CP0->6/1CP0->6/1CP1 sequence now finishes one
paired enrollment, both native jumps/landings/movement and correct settled
scenery. First directlatersection case has bad scenery; repeating baseline
native prerequisites corrects it on the same binary, not a renderer fix/claim.
**117 passed,8 failed,109 unverified;117 open of234**.21/21CTest and274 matching
currentcore/modCRC samples through8832, resolved8862both, owned pair reaped.
No new state/schema/option/site/SDK. Actual online support/lobby/rematch,
literal rotating stairs/route and transientpixel claims remain unverified.
qa-support-arrival-20261021/qualification.json.

Latest survivor crusher milestone: unchanged39161AF2 closes F88 bounded
control analogue using one original raw6/0 class10/sub0 cycle with P1 dead.
The first enemy-knockback passage is excluded; one correction waits native
hurt recovery, isolates incidental contact, then actual P2 Left crosses the
raised gap103.5px, followed by native16.664px jump/landing and Right response.
Original F8 remains0 and terrain contact08; no cycle/geometry/result writes.
Damage/crush lethality and carry/release remain unverified. **116 passed,
8 failed,110 unverified;118 open of234**. No build/source/SDK change.
qa-survivor-crusher-20261021/qualification.json.

One bounded source follow-up retains F22/F56 unverified: normal3/charged0C
colliders resolve projectile against terrain, not hero against weapon. No
native supporting-weapon analogue/absence proof or safe gameplay recipe.
Class10 secondary-source audit likewise keeps F190/F194/F195 unverified:
autonomous press/attached damage child is not a scripted threshold/fireball.
No further source variants in these families during this breadth pass.

Latest native arrival breadth check: unchanged39161AF2 reproduces F20 spacing
failure at original raw6/0CP0: both heroes settle atX160, feet432, with zero
horizontal spacing. Both native independent jump/land probes pass35.2734px;
this does not pass the paired arrival QoL. Initial enrollment waits for P1 idle
and join_animation discards enroll's requested24px spacing. Record the failure
and defer a scoped repair while covering other mechanics. **115 passed,8 failed,
111 unverified;119 open of234**. No new build or timing/renderer claim.
qa-native-arrival-breadth-20261021/qualification.json.

Latest exact gate-occupancy milestone: unchanged39161AF2 closes F183 with
leaderfullleft9622 beyonddoorright9376 and livingpartnerbox9344.5..9366.5
insideclosingregion, actualnative1/3 held28updates, realfullclearance/closing
and both freshpostclosurecontrols+5.5/+7.5. **115 passed,7 failed,112 unverified;
119 open of234**. The same case repeats F35 mechanics oncurrentcandidate.
Upper-roomposition-only setup leaves camerasY240/bodiesY74..75: presentation
and original vertical-entry/camera path stay unqualified, not called a renderer
failure. No extra source/build/matrix. qa-both-occupancy-20261021/qualification.json.

Latest bounded final-stage setup deferral:39161AF2 original12/0CP21/P2Right
stops at the final native door without admission. One position/originalscanner
shortcut begins the authored final loader (nativePLAY31 0->1), then P2 dies
during setup before paired firstboss readiness. No finalboss gameplay failure,
real defeat/phase/ending or Continue pass is claimed. F136/F138/F139/F140/F141/
F142 retain individual unverified reasons and source gaps. No third variant.
Counts remain114/7/113 of234;120open. qa-final-boss-breadth-20261021/qualification.json.

Latest native-field/gate-reuse milestone:39161AF2 qualifies actual original
boss field expiry/work/control (F171) and fixes a newly reproduced recipient
switch leaving stale weapon suppression. Fresh nativephase7 release and one
different-nearer-hero ownership check exercise both original release branches;
7actualtargetoverrides preserve the held hero through alternate nativeF5B60.
F35 closes from independently hash-verified named Both-gate receipts; F183
keeps its missing inside-region condition. **114 passed,7 failed,113 unverified;
120 open of234**.21/21CTest and268 matching currentcore/mod CRC samples
through8608(resolved8616/8616) pass.
F162 separated-camera condition and actual field online remain unverified.
No new state/schema/sites/option/SDK change. Evidence:
qa-native-field-20261021/qualification.json. Final native boss route is next.

Latest corridor-transfer milestone:96AA8916 fixes actual raw2/1 native-arrow
admission and omitted passenger handling, closing F24/F96 with one P2-owned
forward transfer and a P1-owned inverse with nearby partner. Both native fades,
destinations and releases execute; both heroes remain alive/visible and recover
actual controls. **112 passed,7 failed,115 unverified;122 open of234**.
21/21CTest and272 matching current core/mod CRC samples through8704 pass
(resolved8736/8735). Actual arrow online, all13arrows and literal gravity
inversion remain unverified.
Retained baseline, failed setups/guard correction and actual source-word proof:
qa-corridor-arrow-20261021/qualification.json. No SDK/option/schema change.

The source audit also corrected an authored-content inventory omission:
native2B520 scans the primary root72EAC, then entry-spawns a separate list
from72F64 through2B61C. The additive inventory has206 secondary records in21
sections, all unverified. Existing113checkpoint and1535primary records are
independently unchanged. These are content inputs within current coverage,
not206newcomparisons or passes; dynamic producers remain a separate limit.

Latest native hazard milestone: A685CFC6 fixes a distinct raw2/1 class0F
proximity/solid-contact omission for leading P2. F107 passes actual native
activation while P1 remains144, floor-state/collision at1669.5, living P2 and
nativeLeft-10.5px; current second-hazard P1 trigger preference is preserved.
**110 passed,7 failed,117 unverified;124 open of234**.21/21CTest and274
current combined core/mod CRC samples through8768(resolved8796both) pass.
No SDK/option/schema change. Late-allocation squeeze setup and the helper's
invalid demand to walk inside the solid remain retained; route/toggle/pixel
and actual hazard-online checks stay separate. No further approach variants.
Evidence:qa-falling-hazard-20261021/qualification.json.

Latest breadth/reuse milestone: F199 passes the original falling-subtype
eligibility analogue using independently hash-verified D7BAE910 receipts:
three authored variants inventoried, actual P2-only subtype1 arm/fall/retirement,
then a genuine retry/repeat. F89 passes the current C8493C6E native arena span.
**109 passed,7 failed,118 unverified;125 open of234**. No new candidate/build.

The same current session adds six authored raw7/0 interior checkpoints
(CP1,2,3,4,5,7) and12 actual independent jump/rise/landing passes, with the other
hero stationary. Routes, alternate exits, hazards, enemies/audio and progression
remain separate. Four Zero combo/aerial/finisher/fresh-followup comparisons stay
individually unverified after one shared native attack bundle; source/physical
contact review confirms only one standing fractional hit, not a full chain.
No further attack/entry variants. Sole approved worker has finished.

Latest arena milestone: unchanged C8493C6E closes F89 with actual original
P2-owned class36 intro/HP67/controlrelease, continuous native X/P2 wall-to-wall
walk9467->9925(458px), native solidcontacts0A/09 and turnback-17.5px.
Both composed boundary images inspected. HP/contactprotection excludes combat
while terrain collision stays native; prior route/door is bypassed.
**108 passed,7 failed,119 unverified;126 open of234**. No rebuild or new profile.

Latest bounded deferrals: intro L11/F2/F53/F66/F41 remain unverified after
one original raw0/0CP3 entry and one corrected fresh Zero NewGame continuation
both reach nativefall/GameOver before control release. No door/warning/boss
mechanic pass or crash claim. F104 remains unverified: no original slippery
material/location established; body79 is lethal contact, not an ice flag.
F84 live CLUT matches native palette; texture/upload mapping is the remaining
art prerequisite. No further setup variants or diagnostic builds in this batch.
Counts stay107/7/120 of234;127open. Receipts retain every bounded attempt.

Latest story/dialogue milestone: C8493C6E repairs the native P2-owned class41
producer's campaign predicate. Actual P2 door,30 original pages,HP114 and both
native controls now release. F9 passes exact original source/working/VRAM colors
and readable composed dialogue. F84 fails visibly corrupted opponent art;
matching actor resource pointers do not qualify displayed pixels. **107 passed,
7 failed,120 unverified;127 open of234**. Actual Xbuster114->113; three Zero
swings did not connect and are deferred.21/21CTest pass. Current combined
loopback passes 261 unique core/mod samples through 8352
(resolved 8368/8367). No new network configuration matrix.

Latest midboss milestone: AFC81AB3 fixes reproduced P2-only native admission;
actual owner2/camera/init/control/opening component hits close F184/F191.
**106 passed,6 failed,122 unverified;128 open of234**. Full midboss defeat/exit
is deferred after the bounded attack batch; no flags seeded.21/21CTest pass.

Latest breadth milestone: F14/F23 pass actual78E06414 post-P2 scene/dash and
P2-owned Either door/P1-dash. Reused native gate/death-camera receipts close
F57/F58. **104 passed,6 failed,124 unverified;130 open of234**.

Latest balance milestone:78E06414 adds scoped native scaling/recovery, actual
X/Zero carry/current-state/scaled finish and shared30->15 evidence;21/21CTest
and267-CRC current combined loopback pass. F63/F99/F181 stay failed/partial
for explicit unsupported encounters and gameplay gaps. Ledger100/6/128,134open.

Latest native weakness milestone: E61EFF70 closes F125/F207 together using
actual P2-owned Wing Spiral against original class34. Native HP71->66 and wind
reaction0B pass; continued attacks do not restart the finite reaction, native
recovery restores the original damage table and AI moves again. **100 passed,
6 failed, 128 unverified; 134 open of234**. No rebuild or new option variant.
Other campaign story entry exposes a Both-door stand-pose blocker; F84 stays
unverified and F91 failed. Local commit a5c9fab retains the snapshot/story fix.

Latest current-build snapshot milestone: E61EFF70 passes actual warm save/load above the old 16 MiB sprite-wire limit, exact retained art hashes, cold load before enrollment, mapped Square save and rewind through the P2-owned menu with native control recovery. F33 passes bounded; F91 is now failed after actual story intro begins during unfinished Both-door traversal. **98 passed, 6 failed, 130 unverified; 136 open of 234**. All 20 CTests and the exact 618-buffer corpus pass. Combined current-candidate loopback passes 264 unique core/mod CRC samples through 8480 (resolved 8501). Missing enhancement/balance options, scenery rendering and stability remain open.

Latest native story-opponent milestone:915D0D20 fixes actual nearestX/P2 projection deleting Zero-storyclass40. Original dialogue/HP66/AI/art and actual Xbuster66->65, Zero ground65->63/air63->58 close F82/F85/F87/F117. ActualP1fatal leavesX/P2 abletohit58->57, deliver real positive1HP finalhit and reach nativechapter20/CP14. P1Cross advances16pages; fullchapter/destination/ending deferred after160s. **97 passed,6 failed,131 unverified;137 open of234**.19/19 CTests2.42s. Same-session save8 proves F33capacity failure:42,127,760spritewirebytes>16MiB. Current correctedE61EFF70 losslessschema5 passes20/20CTests2.31s and exact618buffer corpus; real native storyentry/release passes, warmnative snapshots pending.75C0B160 stage18Bothdeferral stalled actual aperture/nativeinit and was removed; F91 is now failed from the corrected native observation. Late915chapter watchdog4168215us is preserved with its exactring; stabilityopen.

Previous arrival/exit evidence reuse: read-only checks of immutable native receipts close F7/F8/F44 together: actual P2-led original pad/Grizzly arrival with both grounded feet and controls; previously hidden living partner returns after the native conversation; actual distant P2-led2/0->2/1 exit with safe next-room controls. **93 passed,6 failed,135 unverified;141 open of234**. No new runtime/variant. Current915D0D20 additionally fixes an actual newly observed story-opponent deletion: nearest X/P2 projected into the Zero-campaign class40 availability check could invoke native2D6FC clearing. The guarded branch-register predicate now uses canonical campaign identity; no actor/resource/AI-target rewrite or new state. Build/regeneration and19/19 CTests2.42s pass; native runtime qualification is in progress. Story rows stay unverified until their actual checks pass.

Previous Both-pad/diagnostics milestone: BBCFCBC1 implements the missing Both-player refight-pad gate at original idle admission, preserving native animation, collision and control recovery. F123 passes actual P2 wait/abort, both contact transport and native withdrawal fallback with distinct resources. F70 passes actual current-build save/load in an isolated diagnostics-off profile with no recording folder/REC. **90 passed,6 failed,138 unverified;144 open of234**.19/19 CTests pass2.16s; one current combined netplay batch passes274 unique core/mod CRC samples through8768(resolved8796), including P2 scene-owner fatal/hidden survivor/release. Actual online Both-pad admission, launcher Host/Join/START offers and same-process rematch remain unverified. Product docs/screenshot are updated; F67 stays unverified pending actual bundled UI/controller walkthrough. Current325 snapshot capacity follow-up does not reproduce prior9FF refusals; F33 remains failed. Source/code and evidence below remain local.

Previous elemental/world breadth milestone:32568930 fixes actual Zero/P2 deferred landing3F art ownership and original palette/drawing (F27), with earned native rising/dive/landing/expiry/control evidence and19/19 CTests3.33s. Native opposite-side enemy initialization/live offscreen retirement/re-entry closes F119/F163 in one encounter. Unchanged9FFB6E38 closes actual secondary boss target F118 and distant P2-led section exit F74. **88 passed,6 failed,140 unverified;146 open of234**. Failed assertions, setup limits and later pit fatal are retained; no third approach or configuration matrix. The same32568930 current loopback passes159 unique core/mod CRC samples through5152(resolved5154), both private menus, charge/release, withdrawal/return/resources, diagnostics and Alia suppression;858/857 restoration checks have no faults. Actual launcher Host/Join/START offer, same-process rematch and active elemental network ownership remain unverified.

Previous native breadth batch: unchanged9FFB6E38 closes F92/F98/F147 in one fresh owned process: native Armor movement/safe dismount with distinct resources/ground controls, composed moving layering, and actual pad entry during P2 withdrawal through native loader/boss release. **83 passed,6 failed,145 unverified;151 open of234**. The stale52780 shutdown did not prevent a new boot or gameplay checks. No rebuild or configuration matrix.

Previous evidence reconciliation: existing84EDFD5A native P2 refight admission/full boss release closes F5/F124 together; existing9796C215 actual Chaser-stage boss final hit/paired victories/native departure closes F134. No new runtime or build. Source audit confirms applicable F37/F63/F99 options missing; mark failed individually. **80 passed,6 failed,148 unverified;154 open of234**. F22/F56, exact dash colors and remaining Armor obligations stay explicitly unverified. Current-build snapshot correctness remains required; backward save-state compatibility/migration is never required.

Previous breadth milestone: unchanged9FFB6E38 closes legacy13/F42/F80 with actual P2-led enemy initialization/hits at two authored locations and native shared-menu/private-view recovery. F175 now fails: original scenery clocks run, but P2 visible background colors stay frozen across a full recorded cycle. **77 passed,3 failed,154 unverified;157 open of234**. Nearby fade/auxiliary-effect/special-group and sky-gap diagnosis remain explicit deferrals; no source fix, rebuild or configuration matrix.

Previous comparison milestone: unchanged9FFB6E38 adds F26/F129/F174 bounded passes: real native weakness damage/reaction, recovery rejection/fresh acceptance, P2-owned guided F-Laser motion/children/visible art. F33 reopens after actual save/load refusals. **74 passed,2 failed,158 unverified;160 open of234**. Persistent normal7/refight setup is deferred; no shot fired. Owner steering prioritizes breadth before depth and defers ordinary difficult setups.

Previous comparison milestone: unchanged9FFB6E38 closes legacy16/F15/F188/F11 in one shared Chaser setup: actual once-only READY/retirement, clear native P2 dash/independent acceleration, native victim/car cleanup, survivor controls, real once-only team retry and both fresh native controls. No rebuild or CTest repeat. F16 exhaust ordering, F93 original entrance-speed baseline and legacy18 actual road handoff stay unverified. Failed setup receipts and corrected original camera-field semantics are retained below.

Previous comparison milestone: capsule bundle9FFB6E38 closes F154/F167 together: native X/P2 admission, correct original recipient dialogue, genuine program grant, normal return, mid-dialogue save/restore and owner-fatal survivor recovery. A real native Save(B1E5473E)/cold Load(9FFB6E38) retains program40. It fixes P2 singleton capsule gates, a committed-return fatal softlock and Zero-text resource binding for X. Debug19/19 CTests pass1.50s. F55/F52/death presentation stay partial; no completed-armor, route, production or current-netplay claim.

Previous comparison milestone: unchanged84EDFD5A passes actual pad-owner fatal
while a living passenger is hidden (F103): one native recovery, real destination
and boss intro/release, retained survivor resources and actual movement.
Overlapping particle/color/pose checks stay unverified: no particle allocation
was observed before the room change. No rebuild or new configuration variant.

Previous follow-up: candidate84EDFD5A completes one actual P2-led Grizzly refight
entry/fight/defeat/automatic hub return, closing legacy21/F47/F156/F165/F206
together. It fixes a dynamic-pad arrival stall at the actual contact boundary
and a P2-owned continuation that copied P2 HP/ammo into P1. Both heroes retain
their own resources and actual pad feet/control; debug19/19 CTests pass1.36s.
Current production/netplay remain unverified. See the refight milestone below.

Previous candidate94AA9338 fixes a traced deferred weapon5 graphics
read that overwrote native PLAY and returned to title. Real card91 cold Load,
full earned X/P2 cycle0->1->5->8->0 and guest menu/resume now pass, with the
original X resource directory and restored canonical Zero bank verified.
19/19 CTests pass2.86s. The same candidate adds actual stage4/6/7 final
Both doors, native defeats/rewards/Saves/selector returns and distinct stage6
recovery/stage7 attack-child checks. All eight ordinary Maverick defeat flows
now have bounded named-candidate evidence; full routes/mechanics remain open.
The next named-row batch qualifies normal8 tornado graphics/contact/cleanup
and exact original pixels in both panes, closing F50/F59 bounded. Actual second
Dynamo defeat/Save and cold native Load also pass on the same candidate.

Previous bounded milestone: candidateC0DF29A5 fixes the native P1-death pause
lock and the distinct later stage8 support controllers. P2 native pause/resume
works after an ordinary P1 fatal, while a genuine held script lock survives
until its native release. Later P2 contact/position admission, owned motion,
private camera bounds and a bounded moving-support save/restore are qualified.
The actual final Both door, class37 HP fill/final hit, paired victories,
rewards, real Save(checksum6046,mask91) and selector return also pass.

The comparative ledger is110 bounded passed,7 failed,117 unverified
(234 total;124 open). F159 closes for later-controller admission/usable
collision, not a complete platform route. The CP2 full-ride probe is still
failed: native tile calls precede P1 losing floor, then P2 dies near a ceiling;
the correct route/obstacle handling and failure cause remain unverified.
C0DF29A5 passes19/19 CTests in2.53s, including retained controller snapshot
state. The loaded-cycle/title return is fixed by the94AA9338 follow-up above.
Current production/netplay, water/timed supports, native intro/final-story
branches and stability remain open. Everything stays local; older owner playtest payload unchanged.

| Comparative checkpoint | Passed | Failed | Unverified | Open |
| --- | ---: | ---: | ---: | ---: |
| Incoming-arrival correction |21|0|213|213|
| Health camera / entry-table milestone |26|0|208|208|
| Native Quit / re-entry milestone |29|0|205|205|
| Native boss rewards / save / return |31|2|201|203|
| Paired native victories, each owner |33|0|201|201|
| Ordinary dash exhaust / native card reload |34|0|200|200|
| Both-door clearance before native boss admission |35|0|199|199|
| Additional native bosses / portraits / post-victory cleanup |39|0|195|195|
| Private E policy / confirmed missing feature contracts |40|2|192|194|
| Offline snapshots / mapped save-load and rewind |41|1|192|193|
| Falling-object P2 trigger / survivor lifetime |43|1|190|191|
| Personal menus / post-menu attacks / native selected-weapon use |46|1|187|188|
| CShot original pixels / native Chaser entrance enrollment |48|1|185|186|
| Native factory boss / first Dynamo progression |49|1|184|185|
| Native lift admission / return / carry / cameras / survivor / retry |57|1|176|177|
| Later controller admission / survivor pause / actual stage8 clear |58|1|175|176|
| Earned special family / exact original composed pixels |60|1|173|174|
| Actual refight admission / private cameras / defeat / safe hub return |65|1|168|169|
| Actual pad-owner fatal / hidden passenger / native recovery |66|1|167|168|
| Actual capsule text / grant / restore / owner-fatal recovery / native card persistence |68|1|165|166|
| Shared Chaser READY / native dash / acceleration / fatal cleanup / actual team retry |72|1|161|162|
| Native weakness / rejected contact / guided owner; snapshot regression reopened |74|2|158|160|
| Shared menu recovery / ordinary and later P2 placement; scenery animation failure |77|3|154|157|
| Existing native refight admission / vehicle-stage victory evidence; three missing options confirmed |80|6|148|154|
| Native Armor resources / grounded dismount / moving layering and withdrawal-pad race |83|6|145|151|
| Native secondary target / distant section exit / elemental fix / live enemy re-entry |88|6|140|146|
| Actual Both refight waiting / withdrawal fallback / diagnostics-off state restore |90|6|138|144|
| Reused actual platform arrival / missing partner return / distant exit receipts |93|6|135|141|
| Native story-opponent art / ground-air hits / recovery / survivor combat |97|6|131|137|
| Large-cache snapshot repair / cold load / mapped rewind; premature story admission confirmed |98|6|130|136|
| Native phase-specific weakness / repeated-attack finite recovery |100|6|128|134|
| Post-scene/door dash and reused unavailable-gate / old-corpse camera |104|6|124|130|
| P2-only native midboss admission / original initial arena camera |106|6|122|128|
| Original story producer / P2 dialogue palette; counterpart art corruption exposed |107|7|120|127|
| Native arena collision / continuous wall-to-wall movement |108|7|119|126|
| Reused actual authored falling-subtype eligibility / native retry |109|7|118|125|
| Distinct native proximity hazard / P2 solid admission / preserved P1 preference |110|7|117|124|
| Native support arrival / survivor controls / targeted grab recipient repair |118|8|108|116|
| Consecutive actual native Both doors / Unified health / no second warp |120|8|106|114|
| P1 preinit death / P2 native shared-color initialization / actual survivor controls |121|8|105|113|
| First P2 death / layered private cameras / actual native team retry and controls |122|8|104|112|
| Reconciled original second-controller start / reused native P2-only admission and carry |123|8|103|111|
| Shared-atlas repair / survivor GPU palette uploads; separate scrolling still open |125|7|102|109|
| Native owned Chaser-stage weakness / actual recovery |126|7|101|108|
| Native Zero chain / finisher expiry / fresh followup in one encounter |129|7|98|105|
| Mounted-scene hidden-return repair / equipped-form prerequisite audit; full comparisons retained open |129|7|98|105|
| Current Split performance / layered peer backgrounds; launcher and special-arrival gaps retained |131|7|96|103|
| Paired native stage timing / safe spacing / support contact ordering / current arrival restore |132|6|96|102|
| Native Ultimate Nova / private camera and partner control recovery |133|6|95|101|
| Native shared quake commands / both private cameras |135|6|93|99|
| Native death own views / poses / glyphs / actual approach recovery |137|6|91|97|
| Native story Both clearance / original dash working colors |139|5|90|95|
| Native train spatial clock / panorama wrap / same-candidate atlas |141|5|88|93|
| Original story sheet / rising attack recovery |143|4|87|91|
| Automatic native forms implemented; remaining mechanics unverified |143|3|88|91|
| Actual launcher options / native Black Zero checks; post-door artwork failure retained |144|4|86|90|
| Native lift charged-child contact/expiry and exact Zero/P2 endpoint release |146|4|84|88|
| Native atlas prerequisite correction; ordinary Black Zero Saber qualification |146|3|85|88|
| Native survivor scrolling / owned Ultimate aerial dash; focused final-door fix |148|3|83|86|

Counts track the234 frozen original X4 reports, not new test variants or a
weighted percentage of playtest readiness. Failed rows stay open.



Current primary buckets(2026-10-22): count each open report once by its first
mapped P-code. Cross-cutting obligations remain on each row. These organize
reporting; sessions still overlap buckets, and counts are not playtest readiness.
The earlier93-open table and inferred bucket deltas were stale; this table is
recomputed from all86 current open source IDs with the explicit grouping below.
Historical milestone summaries retain their original reported counts.

| Primary bucket | First P-code group | Open |
| --- | --- | ---: |
| Netplay and controller setup | P05, P31, P34 | 7 |
| Controls, death and transitions | P06, P10-P15 | 13 |
| Stage mechanics and environment | P24-P26, P33 | 24 |
| Doors and warps | P16-P17 | 10 |
| Combat, weapons and boss damage | P07-P09, P21, P23, P29 | 22 |
| Vehicles | P27-P28 | 3 |
| Menus, pickups and HUD | P18-P20, P32 | 3 |
| Story and progression | P22, P30 | 3 |
| Diagnostics, snapshots and delivery | P01-P04, P35-P37 | 1 |
| Total | Every open row exactly once | 86 |

### Batches selected from open comparisons (owner steering,2026-10-21)

Breadth-first steering(owner,2026-10-21): one ordinary initial attempt and at
most one source-backed setup correction, then defer with evidence and a next
prerequisite. Prioritize new locations/mechanics before deferred reproductions.
Serious crashes/progression blockers retain priority. Frozen scope stays234.

Begin the next runtime session with named open rows and their decisive
observations. Add content checks that fit the same route/state; record extra
content coverage separately when it does not complete one of those rows.
The first-lift session closed8 comparisons together; the later boss flows
increase content coverage and fix X5-specific failures without inflating that
ledger. One batch may contain sequential cases where a distinct death/ownership
boundary requires it; do not multiply roster, seat, camera, loader or network
variants without an actual different contract or failure.

| Next shared setup | Individual open comparisons / required evidence | Natural overlapping checks / limits |
| --- | --- | --- |
| Armor ownership/movement/dismount -> native withdrawal/pad race | F92/F98/F147 pass on current9FFB6E38 with actual native grounded/resource/moving-layering and LEAVING/scene overlap/loader/release evidence | One process; initial lava setup, one safe-platform correction and same-encounter assertion continuation disclosed. Mounted transfer/damage/current peers remain open. |
| Reconcile existing native encounters and remaining elective contracts | F5/F124 passed together on84EDFD5A actual P2 pad2/Grizzly release/controls; F134 passed on9796C215 native class32 final hit/paired victories/report. F37/F63/F99 failed applicable missing options | Read-only evidence reuse adds no runtime/build. F12 later qualifies ordinary native dash colors; forced Armor transfer stays open; the subsequent native batch closes F92/F98. F22/F56 source absence is not proved. |
| Native menus -> ordinary/later P2 placement -> scenery clocks | legacy13/F42/F80 pass on unchanged9FFB6E38: native shared-menu/private-HUD recovery; original0E/2A pre-absence/admission/origin/init and actual P2 buster16->13/32->29 | F175 fails actual visible P2 color animation despite correct native clocks. F18/F144 and special-predicate/group rows deferred with receipts; sky rectangles observed/cause unverified. No further variants this milestone. |
| Actual refight-pad owner fatal with hidden passenger | F103 passed bounded on84EDFD5A: exactly one recovery, original pad departure/loader and next boss intro/release, retained survivor resources/retries and native movement | F4/legacy3/F133 targeted together but stay unverified: no particle allocation before this native room change. Reuse ordinary-fatal evidence/source references for a later naturally emitting death; no additional pad configurations. |
| Actual X/P2 capsule grant -> native return -> native save/reload | F154/F167 passed bounded together on9FFB6E38; complete native recipient text/grant/normal return, mid-text snapshot, actual grant-owner fatal recovery, B1 native Save and9FFB cold Load | F55 stays partial: one earned program does not prove full armor equipment/art/abilities. F52 capsule boundary passes, but intro/water remain open. The same fatal adds finite44+32 native allocation evidence; color/pose/complete cleanup remain open. No more capsule setup variants. |
| Native Chaser opening -> vehicle interactions -> death/retry/road handoff | legacy16/F15/F188/F11 passed bounded together on unchanged9FFB6E38: real READY child retirement, native P2-only boost poses/speed, victim car/rider cleanup, survivor input and actual once-only team retry/both native controls | F16 exact exhaust ordering and F93 full original entrance-speed baseline stay partial. Legacy18 actual same-raw5/0 road endpoint stays unverified after the late-position setup misses its trigger; no more approach variants this milestone. F133 receives vehicle cleanup evidence only; current peers remain F75. |
| Earned native weakness -> accepted hit/reaction -> separation/re-contact | F26/F129 pass together on9FFB6E38: real normal6 damage5/result7/phase0A, genuine rejection7F without HP loss, original recovery/fresh re-admission | F130/F131 deferred: original pad2/CP2 passes, but no normal7 shot fired. Preserve approach failures and source-proved fixture12/hurt-timer interference. Future genuine pre-hurt/native entry with HP-only protection; no pose/timer writes. Air/wall/final-phase/scaling stay open. |
| One current candidate: loopback start -> named shared/private scenes -> restart | F1/F32 peer gameplay; F40 complete option/compatibility plan; F39 actual restart/allocation lifetime; HUD/second-door rows only when their scene is exercised | Compatible changes qualify together with core/mod CRC and actual behavior. Loopback does not prove Discord/service/other-OS connectivity. Preserve default diagnostics/hint suppression; no network-condition matrix. |
| Intro warning -> distant views -> owner fatal -> survivor boss finish | F41 native warning/admission death cannot loop; F54 actual X/P2 intro final hit preserves campaign/next screen; F66 world-relative boss/projectiles and real interaction; F4/legacy3/F133 death burst/origin/color/pose/retirement only when their complete observations are captured | Capture visibility/projectiles before fatal, then original death and actual surviving X finish/return. Keep setup/branch prerequisites explicit. Do not close a death/presentation row from counters or a loaded boss alone. |
| Original refight pad -> active fight -> actual defeat -> native return | legacy21/F165/F206/F156/F47 passed bounded together on84EDFD5A: P2pad2, actual room controls/private camera follow, native defeat/automatic CP11 return, both feet on its actual supported pad and retained independent resources | The same actual encounter covers all five rows. Original X5 returns automatically; no new clickable portal is expected. F103 has since passed the distinct owner-death-during-arrival case above. Other refights remain open. |
| Earned P2-owned rising-tornado family, own pause/resume, real target contact and expiry | F50/F59 passed bounded together on94AA9338: own art/segments, actual enemy damage, retirement/control and exact original pixels in both composed panes; preserve earlier CShot/menu evidence | One shared setup with clear-space and actual-target attacks. F174 remained unverified at this tornado milestone; the later normal6 guided-owner evidence below now qualifies its bounded analogue. Broader untested weapon families stay open. |

Each row keeps its own passed/failed/unverified/inapplicable disposition and
source traceability. Source recipes are preparation, not passes. Review the
current bundle's remaining rows before choosing another location; timebox
ordinary failures and carry an unresolved mechanic forward without restarting
whole sessions or building diagnostic variants.

Resumed at the owner's request after local checkpoint `3d5abaa` on 2026-10-19.
The deferred-buster candidate passes all17 CTests in debug and production.
On2026-10-20, the fatal-entry cleanup passes17 tests in debug (AA1105D6)
and production (0F7115D7). Runtime qualification below names loading controls
explicitly; the intermittent watchdog remains unresolved, including a resident-loading-off exit.
The personal-effects candidate also builds and passes17 tests in debug
(3FE6E7DC) and production (7B12728E). Runtime evidence remains bounded below.
Latest qualified door-recovery candidate532A9498 (productionCEC9E5D9) passes17
CTests in each build. Both rosters pass six actual Grizzly door cases with
default loading/local Split, including either owner fatal at two requested
timing points. Native post-door X/P2 survivor glow/shot release and inventory
also pass. Coupled stand-lock/native boss regressions pass; P37 remains open
after capture-write failures and a watchdog abort. Earlier E97 same-build
loopback evidence is295/275 matching core/mod samples; it does not qualify
actual doors online on the new candidate. The separate default-on Alia hint candidate44289580 (productionFCD10A2D)
also passes17 tests each plus solo/local trigger controls and a286-sample
same-build title-default loopback scene sequence. Full X4 parity remains
unfinished; evidence below is bounded by case.

Local milestone31fa682 adds effective diagnostic mode/version history and
finishes individual plans for all234 frozen X4 reports. Normal952B028F passes
native local Split logger qualification and26 jump/landing probes across13
entry tables. The two sweeps load15/26 tables: stage11 watchdog-aborts and
stage13 stays in native loading mode;9 tables remain untouched. A focused
water-entry check reproduces P2 crush death. Production7668995D builds
but its F-path writer check times out twice; the identical writer executable
passes a15s-bounded C-path control in0.984s. These failures remain recorded.

Incoming-arrival checkpoint0D15A16C passes all17 debug CTests and the
original water-entry reproduction. Native Select return while P1 walks219.5px
also passes with exactly one incoming start and retained P2 HP17/ammo73.
At that checkpoint the comparison has21 bounded passes and213 unverified rows;
the earlier water failures remain retained. ProductionA1F0FFC7 also builds and passes all17 CTests in60.42s.

Base: X5 co-op alpha `d6c0907`, merged with the latest local main `18dc0b4`
to retain the working adaptive authored-map renderer and resident loading.
Reference frozen through committed X4 Playtest42 at `50593bca772b600de6911cbfecd40b0f08721147`,
replacing the initial Playtest39 `cf6ce95` audit. Uncommitted X4 work is excluded;
see `COOP_X4_REFERENCE.md` and its source/blob/SHA inventory.
Framework candidate: X4's `6f0ccbc6`; UI `b964eabe`. Shared dependency trees
are read-only build inputs through local junctions. Original game assets,
generated code, cards and captures remain ignored and local.

## Interim owner playtest gates (2026-10-20)

The next handoff targets a useful local co-op playtest while the full234-report
campaign continues. This is not full X4 parity acceptance. Both-player doors,
remaining content/progression and snapshot parity remain required campaign work.

| Gate | Current state | Evidence / remaining work |
| --- | --- | --- |
| Core local co-op | Bounded checks passed | Native controls, menus, Either doors, death/retry and resource checks below. |
| Incoming landing correction | Committedddf7284 | Original water crush and moving return pass; debug/production17/17. |
| Duplicate asset read | Both native source paths passed | BC1F66D0 enrolls both heroes with resident reuse and with resident loading disabled/disc fallback. Both builds pass17/17. |
| Prior owner-candidate local smoke | Passed bounded | Ownership/pickup/tank, moving return, actual P2 door, hidden-passenger owner-fatal recovery and original water-entry chain pass on BC1F66D0. |
| Clean owner payload | Staged and verified | G:/Temp/Codex/psxrecomp/x5-parity/local-playtest-20261020. Production15423EB5, windowed settings, normal audio launcher, two controller seats, default diagnostics/hint suppression, filtered cache/catalog. Owner launch pending. |
| Netplay regression | Passed bounded loopback | BC1F66D0 has159 unique matched core/mod samples through5088 across gameplay, hints, effects, withdrawal/rejoin and both menus. Broader network/campaign coverage remains open. |

Owner estimate at this checkpoint:45-90 minutes to an interim local handoff if
the current startup candidate passes. Further failures change that estimate.
The21 bounded passes/213 unverified comparative rows are an audit inventory,
not a weighted readiness percentage. Prioritize the local handoff gates next;
continue the full burndown afterward.

## Evidence rules

`unverified` means no adequate current-candidate behavioral evidence.
`failed` retains the trigger and failing evidence until rechecked after a fix.
`passed` always names the specific check and its limits. `inapplicable` requires
a title-specific reason; a differently named stage is insufficient.
Compilation, registered hooks, screenshots and loading do not pass gameplay.
Setup cheats may position/protect actors, load native checkpoints or change
enemy HP. Never set the completion/progression flags being tested. Record any
bypassed prerequisite. Automated launches are hidden and muted while retaining
SPU emulation. Owner handoff builds use normal audio.

## Burndown

| ID | Area and X4 edge cases to exercise | X5 status / evidence |
| --- | --- | --- |
| P01 | Latest co-op/main integration; preserve renderer and loading | Merged debug build and all15 CTests pass; bounded X/Zero intro control checks pass. Broader renderer/loading QA unverified. |
| P02 | Default playtest recording, both accepted pads, starting cards, mod settings, build identity | Passed recorder unit coverage and native recording/replay2704 inputs. `qa-hud-present/present.png` and `inputs.png` inspect both P1/P2 REC rows in actual host composition. |
| P03 | Logger durability: GUI/no-console, unwritable output, bounded queue, orderly close, failure visible | Passed adversarial writer, no-console, unavailable-directory and pointer-file tests. Real native run produced startup and session files. |
| P04 | Offline replay: exact build/settings, mid-frame P2 input, divergence, private card copies; refuse network replay | Passed unit/launcher checks and real native2704-input replay in `qa-replay-first-x`. New fixture-candidate stream needs its own replay. |
| P05 | Co-op off native baseline, local two-controller UI, netplay lobby/start, controller source changes | unverified |
| P06 | X/Zero in either seat; independent movement/dash/jump/wall contact; native arrivals | Bounded X/P1 and Zero/P1 intro movement/attack smoke pass (`qa-first-x`, `qa-zero-owned`); native counterpart arrivals observed. Dash/jump/wall/contact and broad stage combinations unverified. |
| P07 | Independent ordinary/charged/special attacks, trails/glow, delayed effects, parent-slot reuse, texture capacity | CandidateAA1105D6 passes nine full-charge releases in each roster, including both owned menu returns, exactly one native child allocation per release, the other hero's counter preserved and expiry (`qa-20261020-charge-initialized-native-zero`, `qa-20261020-charge-initialized-native-x`). Base X/P2 uses subtype18/private initialized assembly; Fourth Armor/P1 uses subtype20. Pending-emitter cancellation at fatal entry also passes in each X seat (`qa-20261020-buster-fatal-fixed-native-zero`, `qa-20261020-buster-fatal-fixed-native-x`). Explicit resident-loading-off controls. Candidate3FE6E7DC additionally passes native P2X glow/trails before/after both menus and rejoin, charged withdrawal cleanup, and survivor charge rendering (`qa-20261020-personal-visuals-withdraw-zero`); exact original DAT assembly identities and once-only world clocks checked, composed captures inspected. Zero/P2 trails also pass both menus and rejoin in local Split (`qa-20261020-personal-visuals-x-split`). CandidateF47573A8 also cancels one pending X/P2 charged-shot emitter at native refight passenger reset with zero later original child allocations (`qa-20261020-refight-pending-passenger-zero-split`). E97D2934 additionally passes X/P2 charge after P1 is fully fallen/hidden with default loading (`qa-20261020-hidden-survivor-zero-default`), with the composed image inspected. Other passenger/combat combinations, specials, damage and broad reuse remain unverified. Current82084041 actual earned C-Shot/native Triangle spends only P2 ammo; native cycle back to buster preserves both bodies/HUD and admits a subsequent native shot. Original weapon-menu banks match72704 words. Special projectile appearance/travel/contact remains unverified (not passed from ammo spend). CACCC785 post-victory stage2->native stage3 P2 dash trails pass own assembly, actual draws, expiry and neutral P1 control; see the new native-boss milestone. Additional9FFB6E38 actual normal6 P2 steering/private primary and child ownership/composed art passes F174 bounded; exact live color-animation/all-weapon claims remain open. |
| P08 | Incoming damage before/after either pause; consumable projectile hits once, persistent-hit tokens per seat | Normal44289580 passes native P2 intro enemy contact before menus and after each seat owns/resumes its menu (qa-20261020-broad-baseline-x/incoming): HP32->27,23->18,18->13, one accepted original contact per named window and P1HP32 unchanged. Wrong-seat resume rejected; world stays frozen in the menu. Other damage/projectile/token classes remain unverified. |
| P09 | Enemy/boss targeting, ranged homing, grabs/capture, simultaneous capture, release after death, weakness owner | Incoming/leaving actors are excluded from Split AI and new script ownership. Baseline3FE6E7DC reproduces native refight arrival deadlock; preliminary9124B331 passes all eight X/P1 pad arrivals. FinalF47573A8 retains ordinary native hidden-pose AI updates and currently passes Zero/P1 pads2-9. Enemy targeting, homing, grabs and weaknesses remain unverified. Additional9FFB6E38 actual normal6/class39 weakness damage5/result7/phase0A, genuine rejection7F without damage and fresh acceptance pass F26/F129. Persistent normal7 preparation is deferred; no F130/F131 pass. |
| P10 | Death origin/color and duration; P1 then P2 and reverse; one retry only on wipe; Game Over | Controlled fatal-entry checks pass both orders, survivor controls and one retry per wipe (`qa-lifecycle-final-boundary-x`). Seeded retry budget/contact grace explicitly bypassed. Ordinary damage, death-art comparison and Game Over unverified. |
| P11 | Surviving player controls camera/world without fallen P1 dragging/killing it; authored pit/crush limits | E97D2934 has bounded Zero/P1 fatal -> X/P2 survivor controls and visible local Split panes after controlled and actual Grizzly scenes. Native foot alignment prevents the reproduced one-pixel entry-step blockage. The opposite roster also passes with default verified loading. Other routes and authored pit/crush limits remain unverified. |
| P12 | Select withdrawal/rejoin: hold/edge, safe landing, preserve HP/ammo, lock in grabs/scripts/mounts/vehicles | CandidateF47573A8 passes native P2 Select hold, withdrawal, spectator camera, return and retained private HP/ammo in local Split loopback (`qa-20261020-refight-eligibility-netplay-zero-withdrawal`). Hold observations span102/101 world updates on the two peers;184 core/mod CRC samples match across the full charge/menu/withdrawal sequence. On0D15A16C, native moving-owner Select return passes on the intro floor:219.5px leader motion, exactly one incoming start, retained HP17/ammo73 and no reload (qa-20261020-storage-control/qa-incoming-fixed-zero/moving-owner-return). The reproduced water-opening crush also passes after the incoming-target fix. Grab/script/mount locks and broader landing/resource combinations remain unverified; Chaser lock has separate P28 evidence. 81262804 additionally fixes the split-camera Y=0 outgoing-beam stall; actual native departure toY-69, absent-player door fallback and nonfull grounded return pass. See the door eligibility milestone below. |
| P13 | Catch-up teleport preserves leader/survivor and avoids checkpoint reload, hazards, occupied Armor | unverified; inspect X5's bounded shared-camera policy versus X4 catch-up policy |
| P14 | Same-stage continuation and new-stage roster; alive/absent/dying/fallen/incoming/outgoing; deaths during transition | Same-area alive/absent/fallen P2 continuation passes (`qa-continuation-contact-x`). Corrected actual stage2 forward exit preserves withdrawn P2 and HP17/ammo73 (`qa-area-fixed-zero-p2_absent`). Other states/transitions unverified. |
| P15 | Fallen teammate recovery at forward area boundaries versus same-area reload; no accidental resurrection | Actual stage2 forward exit recovers either fallen seat without a retry, roster change or boss-flag change (`qa-area-fixed-x-split-p1_fallen`, `qa-area-fixed-x-split-p2_fallen`). Same-area fallen P2 remains fallen (`qa-continuation-contact-x`). Other boundaries unverified. |
| P16 | Either hero leads normal/boss doors, both-player door option, nearby passenger, wrong approach, nested dialogue | E97D2934 passes either leader on the actual Grizzly door in Zero/P1 default-loading local Split: native collision/controller, checkpoint3->4 and both returns/controls, detailed below. 532A9498 additionally passes all six healthy/fatal timing cases in each roster with default loading. D5088CD1 also passes one experimental Both foot crossing, both menus, exact P1-left reversal and native owner death/handoff. 81262804 passes pre-admission withdrawn-P2/fallen-P1 fallbacks and native return. CACCC785 fixes stage8's premature boss-init lock: both physical clearances precede native warning/HP56/control release; current Grizzly Both menus/reversals/resources regression passes. Feedback61 now has a bounded pass. Mounted/abort behavior, other doors, exact checkpoint timing and current netplay remain unverified. |
| P17 | Door/warp owner dies mid-sequence; passenger lives, lock recovery, committed safe grounded arrival | 532A9498 passes six actual Grizzly door cases per roster with default loading/local Split: either healthy leader and either owner fatal after acquisition or hidden passenger. Original checkpoint3->4, footY96, survivor HP17/ammo73/inventory/retries/boss flags and native controls remain correct; handoffs exactly0/1. E97/D512 failure traces and the native carry/contact-order correction are retained below. Coupled native boss/held-script checks also pass. Actual door netplay, Unified, other doors/warps, nested locks and mounted passengers remain unverified. Additional9FFB6E38 capsule-return owner fatal passes exactly one recovery, genuine program retention, actual grounded survivor release and native input; refight-owner fatal is separately qualified on84EDFD5A. |
| P18 | Per-seat pause menu/inventory, tank healing once, optional private tanks, quit/cancel ownership | 893D5C8A qualifies both owned character menus in Zero campaign, wrong-seat resume/guest Quit rejection, independent native movement, selected CShot actual hit/own ammo/expiry and primary ZBuster/saber after pause; legacy8/F10/F179 passed bounded. Current9796C215 now matches original CShot pixels/palette exactly in both panes; F150 passed bounded. Pause fades remain unverified. qa-chaser-20261021/qualification.json. Controlled P2 shared-tank/menu ownership passes (`qa-ownership-contact-x`).82084041 passes both personal menus, guest Quit rejection, actual P1 Quit/new-game, opposite-campaign P2 menu/resume and independent control; native13/11HP and73/91 energy remain private before Quit. Actual Grizzly rewards then supply native C-SWORD/C-SHOT; both earned menus and P2 selection/resource admission are checked. 856484F5 additionally passes actual native card Load, earned CShot and the guest menu; explicit native Zero choice also passes in qa-capsule-content-20261020/native-zero-load. Private tanks and full special projectile presentation/contact remain unverified. Additional9FFB6E38 captures both native menu owners/frozen-world/recovery and29 nearby fade images. F18 continuous transient visibility/current Quit-page condition remains unverified; do not infer it from retained body flags. |
| P19 | Health/ammo/heart/tank/armor pickups: collector ownership, delayed heal, no teleport, campaign preserved | EBA84D1C fixes native large-heal camera collapse. Three bounded Grizzly cases pass: P2 large health11->27/auxiliary energy91->187, P1 small health13->17/energy73->97, and P2 native ammo refill91->139 in unselected weapon slots. Noncollector resources remain unchanged; native input recovers. Distinct composed views and original HP/auxiliary-energy rules are checked. Locked ammo was seeded; actual weapon acquisition/selection, permanent items, tank policy and campaign saves remain unverified. See the everyday-interaction milestone below. Additional9FFB6E38 regular capsule5 accepts X/P2, grants actual program40 and returns both heroes; B1 native Save and9FFB cold Load retain40. Completed armor/equipment, other permanent items and route entry remain open. |
| P20 | Stacked/side-by-side HUD and portrait/ammo colors, private view HUD, quit/menu suppression | Modern packet anchoring fixes missing P2 meter; composed entry/pickup captures inspect both meters and X/Zero portraits. Side-by-side entry/area captures pass with Zero/P1 in `qa-side-by-side-maverick-zero`. Local panes show own HUD in `qa-local-pair-armor-qualified-x`. Other cases unverified. 82084041 also visually suppresses gameplay bodies/life HUD on both original P1 Quit confirmation pages; native post-reentry menu and gameplay/HUD restoration are inspected. |
| P21 | Per-character voice and simultaneous jump/hurt/attack SFX; sample residency and independent native channels | All85 original VAB/tone calculations and owned-bank/native-state boundaries pass. CandidateFC6F8C15 passes eight X/P1 cases (`qa-private-audio-native-idle-x`). Candidate7C3E87D3 fixes reverse-roster deferred buster ownership and passes eight cases including exact X/P2 buster/jump tones, overlapping slots and both owned menu returns (`qa-buster-owner-exact-tone-zero`). Debug/production17 CTests pass. 856484F5 additionally qualifies native Zero/P2 hurt/death tone identity and independent hurt/jump lifetime, with the exact limits below. Full music, other hurt/death paths, DSP/ear review and sustained playback remain unverified; see `COOP_AUDIO_PARITY_EVIDENCE.md`. |
| P22 | Shared dialogue input ownership, Alia/capsules/rewards, both living heroes participate in victory, persistent unlocks/save/return | Default-on Alia hint suppression passes solo/local native trigger controls and286-sample title-default loopback despite opposite offline preferences (beads-eio.10.7).82084041 passes actual P1 Quit/new-game and P2 menu recovery, then actual P2 Grizzly final hit -> P1 rewards -> native card save -> Continue/story -> real selector/revisit with correct roster. Post-intro/old-story-queue and route/contact prerequisites were explicitly bypassed. 4D0376A8 fixes the living-passenger victory failure: both composed native start/held poses and real return pass with each scene owner. Native save reload/snapshots, capsules, other rewards/story branches and endings remain unverified. CACCC785 adds actual stage2 and3 P2 final hits, paired victories, native reward/Save/selector returns, immediate cleared portraits and retained campaign1. Mask2->6 persists in real slots; current coldLoad6 remains unverified. Additional9FFB6E38 capsule5 uses the complete original X recipient text, real Cross page input/grant/return and mid-text snapshot restore; B1 real Save/9FFB cold Load retain40. Full armor, other capsules, broader story/endings and current netplay remain open. |
| P23 | Boss scaling: independent hit cooldowns and fractional damage; ordinary/air/finisher saber duplicate tokens | unverified; elective X4 option absent from X5 Current94AA9338 independently proves native class35 P2 hits56->55->54 with actor94 recovery30->0 and damage58 restoration; this is unscaled native evidence only, not the missing elective feature. |
| P24 | Moving/breakable/timed solids: per-seat histories, trigger/expiry by P2, lifts, stairs, hatch clearance | D7BAE910 passes intro falling-solid P2 arm/native fall/retirement, lifetime after P1 death and fresh trigger after actual team wipe/retry. F168/F112 pass bounded; no rideable lift/support, signed-negative release or charged-shot-on-moving-floor pass. qa-environment-20261021. C0DF29A5 adds actual later class12/sub1 contact admission/motion and sub2 position admission/upward64.5px carry with P1behind, plus bounded mid-motion save/load ownership. CP2 complete ride probe and obstacle handling stay failed/unverified; no full stage/platform blanket pass. |
| P25 | Stage controllers: P2-only activation, camera bounds/dead zones, scripted drops/scrolling, freeze/virus/water | D7BAE910 adds native intro falling-object P2 proximity activation/survivor lifetime; water effects remain unverified. Corrected reserved-controller ownership passes stage2/area0 P2-led exit with both alive, Zero/P1 Unified and X/P1 Split (`qa-reserved-exit-zero-p2`, `qa-reserved-exit-x-split-p2`, executableD6206EB1). Split leaves P1 at its native checkpoint platform. Native loader preserves retries/roster/boss flags. Other controllers unverified. Additional9FFB6E38 original raw1/17 P2-only placement initialization and actual enemy damage pass legacy13/F80. Raw17 native palette scripts run once but P2 visible scenery colors freeze; F175 is failed. Stage1 pre-menu sky gaps remain a cause-unverified finding. |
| P26 | All original stage/section/checkpoint records and optional routes derived from actual content tables | Read-only native inventory finds113 checkpoints in26 tables plus1535 placements in21 tables. Normal952B028F entry sweeps load15 tables and pass26 individual native jump/rise/landing probes across13 of them; water entry loses P2 and Chaser needs its dedicated check. Stage11 watchdog-aborts; stage13 remains in native loading mode, with9 tables untouched. Evidence: qa-20261020-storage-control/combined-entry-disposition.json. Original-code role qualification now identifies5 frontend tables among the raw26;21 candidate tables/108 records remain, including the still-unqualified stage14/section0 role. This removes invalid on-foot teleports from future coverage, not frontend/story obligations. Six broader mechanic categories, intervening routes, alternate branches and actual exits remain unverified. EBA84D1C adds16/0,17/0,18/0 and22/0 native entry loads and eight passing jump/rise/landing probes, with the other hero stationary. These are local entry checks.14/0 role,11/0 watchdog and placement-only11/1 entry remain queued; every intervening route/exit and most other mechanics remain unverified. Current9796C215 adds four actual independent native jump/rise/landing probes at5/0/CP2-3, grounded CP4 retry, actual second door and class32 defeat/report/Save/selector. The road, intervening puzzles, first door and other factory placements remain unverified.  Current94AA9338 adds authored final checkpoints4/1/CP1,6/1/CP4 and7/0/CP6, actual native arrivals/floors and forward Both doors/intro/defeat/reward transitions. Prior routes/alternate exits are bypassed, not passed. Additional9FFB6E38 raw1CP0 and raw17CP0 authored placements have actual P2-led initialization/interaction evidence, not merely loads. Raw17 four native palette clocks/wraps are measured but P2 visible animation fails. Routes, native reachability and other sections remain open. |
| P27 | Ride Armor: P1-only policy, native mount/dismount, collision/attack/damage/destruction, survivor/door transfer | `qa-parity-armor-zero` passes Zero/P1 mount/control/dismount, X/P2 remaining on foot, and P2 survivor control without inheriting Armor. AA1105D6 also passes X/P1 with Zero/P2 (`qa-20261020-armor-native-x`, resident loading off). Authored vehicle/position and HP-protection setup bypasses apply; composed mount/survivor captures inspected. Fatal entry occurs after dismount. Current9FFB6E38 closes F92/F98: distinct nonfull13/11HP/9ammo51/73, native grounded dismount/independent controls and composed moving-rider layering pass in qa-armor-breadth-20261021. Existing-vehicle safe-platform position/history correction and initial lava fatalities are disclosed. F100 actual forced dismount/transfer stays unverified. Mounted fatal entry, attack/damage/destruction and doors remain unverified. |
| P28 | Ride Chaser: both native cars/poses/facing, shared READY/clock, full road sections, Select lock, both fatal orders | CandidateAA1105D6 passes both fatal orders, native survivor movement/jump, one world-clock increment per world update, shared retry3->2 on each wipe and180-update Select lock (`qa-20261020-chaser-compact-both-native-zero`, `qa-20261020-chaser-compact-both-native-x`). Both rosters, resident loading off, explicit per-case retry budget and position/history fixture. Composed survivor captures inspected. Full road/READY/campaign behavior remains unverified.4D0376A8 now observes both native checkpoint0 entrances and independent P2 boost/velocity on one actual candidate (qa-victory-20261020/chaser-native-entry-dash). READY lifetime, unoccluded rider/exhaust art and current Chaser netplay remain unverified. Additional9FFB6E38 shared Chaser bundle qualifies one READY/retirement, clear native P2-only dash/acceleration, victim cleanup, survivor controls, actual team retry2->1 and both native jump/land controls. Camera-field source correction and stationary setup disclosed. Actual road endpoint/fallen-rider handoff, original speed baseline, exact exhaust ordering, remaining road content and current peers stay unverified. |
| P29 | All authored Maverick doors, eight bosses/defeats/rewards and actual selector transitions; refights and return pads | CandidateC66C2B20 passes all eight P2-led native pad arrivals with both X/Zero rosters, exactly one handoff each, with the sustained pad7 watchdog failure retained and its isolated retry identified below. P1-led pads2-9 transition and finish both arrivals with X/P1 on preliminary9124B331; finalF47573A8 passes Zero/P1 pads2-9. Explicit resident-loading-off, hub/checkpoint, HP/contact and approach-position bypasses. Native pad collision, checkpoint selection and loader drive each transition. E97D2934 also passes all eight P2-led pads with X/P1 and default verified loading after scene recovery/foot alignment. No boss-defeat/reward/return-pad/selector pass is inferred. Other authored Maverick doors and campaign transitions remain unverified; the actual Grizzly door has separate P16/P17 passes.82084041 now qualifies actual Grizzly final hit/death/reward/real save/selector/revisit on one current candidate; no seeded boss-completion or reward flag. 4D0376A8 passes both-living native Grizzly victory/departure with each scene owner; original failures retained. CACCC785 additionally passes actual rawstage2/1 and3/0 P2 final hits, paired poses/departures, report bits2 then6, real Save/checksums5035 then5552 and native selector returns. The previously earned stage2 portrait remains cleared after the second return; native Dynamo selection8 now owns the frontend. Five other Mavericks, actual Dynamo/refight defeats/return pads and broader progression remain open. Grizzly defeat evidence retains its earlier named candidate limits. Current9796C215 adds actual raw5/0 class32 second-door admission, original final hit/paired victory, earned mask01->11, native Save checksum5641 and selector return. The forced first Dynamo then actually loses its positive1HP to native buster, retreats through six P2-owned pages, reports and saves with unchanged mask11. Four Mavericks(4/6/7/8), all refight defeats/returns and later story branches remain open; prior named-candidate evidence for1/2/3 is retained.  C0DF29A5 adds actual stage8/0 final Both door, native class37 HP64/controller-ready gate, one positive64-to1 setup then native X/P2 final hit, paired held victories/departures, original report mask11-to91, real Save checksum6046/card2 unchanged and native selector. Stage4/6/7 Mavericks, refight defeats/returns and later story remain open. Current94AA9338 adds actual4/6/7 authored final Both doors, original final hits/deaths/rewards/real Saves and selector returns99/B9/F9. All eight ordinary Maverick defeat flows have bounded named-candidate evidence; full routes/distinct mechanics/refights/finals remain. Stage6 paired composed poses were missed and remain unverified; stage4/7 paired start/held poses inspected. See qa-remaining-mavericks-20261021/qualification.json. FBE91E18 adds same-encounter Chaser class32 earned Zero/P2 ice-type22 actual final hit, both victory once, report06->16, real Save/checksum6088/card2 unchanged and native selector. This is a current owned-ice-final-hit supplement, not another unique Maverick or full-route pass; qa-ice-weakness-breadth-20261022/native-final-save-qualification.json. |
| P30 | Fortress branches, alternate X/Zero encounter, Zero story removal/native availability, final boss/end sequence | unverified; optional opposing-player story mode needs title-specific design |
| P31 | Split netplay: distant spawns/lifetime, nearest AI, independent authored limits, private background clocks/palettes | Candidate3FE6E7DC passes Split/Zero with172 matched core/mod CRC samples, including active charge glow/release and both menus (`qa-20261020-personal-effects-netplay-split-zero`). Loopback Split/X passes native intro, both inputs/attacks and both owned menus with169 matched core/mod CRC samples (`qa-parity-netplay-split-x`, executable59F1E66D). Own HUDs inspected; private draw restoration passes without faults. Distant spawns/lifetime/AI and broader campaign remain unverified. Additional9FFB6E38 local Split shared-menu recovery passes F42 bounded; raw17 local P2 scenery colors fail F175. This does not qualify or diagnose the current peer-rendering path. |
| P32 | Local split: stable canvas through movies/menu/shared-scene/fallback/resize, require independent terrain renderer | Candidate3FE6E7DC rerun passes distant cameras, independent controls, three resize targets and P2 menu return with2352 restoration checks and zero mismatch/watchdog/device reads/VRAM or host-state leak (`qa-20261020-personal-local-pair-x-split`, explicit resident-loading-off control). `qa-local-pair-armor-qualified-x` passes distant authored terrain, independent input, both visible heroes/own HUD, equal guest cycle, three window sizes, P2 pause/resume and1164 restoration checks with zero mismatch/watchdog/device reads/VRAM or host-state leak. Renderer derives on with standalone preference off. Story/shared-scene coverage remains unverified. |
| P33 | Adaptive window/aspect changes, real margin tiles/entities, anchored HUD, both views, 4:3 native control | unverified |
| P34 | Deterministic checksummed setup cheats, core/mod CRCs after scenes/deaths/menu/transitions, mismatch rejection | CandidateF47573A8 has184 matched core/mod CRC samples on a local same-build Split pair through attacks, active charge/release, both menus and native withdrawal/rejoin. E97D2934 additionally passes295 samples through replicated qualification commands, original Grizzly P1 scene lock/fatal/controller release and survivor movement (`qa-20261020-scene-netplay-zero-p1-dialogue`). The opposite X roster/P2-owner fatal passes275 samples with30ms latency/10ms jitter. Other death/scene/area CRCs and mismatch rejection remain unverified; direct location/position fixtures remain offline-only and diagnostic-tagged. |
| P35 | Snapshot/rewind policy and safe native call boundaries | ADBB6965 passes bounded offline same-process/cold restores, mapped Square save/Cross load and rewind through a guest menu, controlled native scene/passenger restore and atomic schema rejection. Actual authored scene admission, physical keyboard event, current production/netplay and large-cache capacity remain unverified. Online user jumps stay blocked. See qa-snapshots-20261021. C0DF29A5 retains per-support admitting-seat override in checksums/snapshot provider schema2; unit mutation/restore and actual CP2 mid-motion save/load followed by P2-owned movement while P1 remains behind pass bounded. Full post-restore route still fails at later terrain/hazard observations; no cold restore/new netplay claim. Current9FFB6E38 reopens F33: real save5 succeeds; later save6 fails and load5 cannot capture rollback. Completion generations/flushed runtime.log retained. Cause/repair unverified and deferred for breadth. |
| P36 | Clean local playtest payload, diagnostics default on, co-op opt-in, normal handoff audio, forbidden loading packages absent | Passed bounded staging: production15423EB5 under G:/Temp/Codex/psxrecomp/x5-parity/local-playtest-20261020. Hash/BIOS/license,9 allowed packages,287 matching overlay DLLs, windowed settings, normal audio launcher, two declared controller seats and default diagnostics/hint suppression verified. SDK staged-catalog check passes. Owner launch/controller/human acceptance remain pending; this is an interim candidate. |
| P37 | Performance/resource stability in separated views and long-session texture churn | Deterministic cache stress passes5000 replacements/pool, both world buffers/HUD pins and all-pinned refusal. Runtime stability remains failed/unresolved. Earlier7C3E87D3/C66 watchdog dumps remain retained. On532A9498 with default loading and fresh private history, extended CP1 glow advances41/120 updates in15s with capture-write errors; a fresh post-door glow passes, then CP1 reaches the hidden-primary charge boundary and aborts after20.7s without heartbeat. Latest run/heartbeat/ring/large dump are retained in `qa-20261020-survivor-effects-zero`; no stall cause or I/O causal link is established. Watchdog stays enabled. D5088CD1 later also watchdog-aborts while idle after the owner-fatal door case (6.56s without heartbeat). The hard-freeze dump is truncated before main_stack; postmortem context alone does not establish a cause. See qa-door-ownership/idle-watchdog; no watchdog changes. A fresh81262804 intro abort also retains a complete hard-freeze main stack at frame3583 inside NtReadFile -> ISOReader::ReadRawSector -> cdrom_advance. The final abort occurs later at3701. Byte-identical G-disc runs advance through the pickup checks and four new entries without a watchdog exit; this is a storage control, not a general stability pass. |

## X5-specific findings

| ID | Observed failure | Bounded evidence and next check |
| --- | --- | --- |
| X5-01 | Stage3 water opening loses P2 before its first controlled jump | Fixed and passed bounded on0D15A16C after two retained952B028F failures. The one native beam follows the current grounded foot leader and commits collision after a safe foot target/terrain query. The identical stage2/section1 -> stage3 setup now lands both heroes at721,430/431, ground8, HP32/32; both native jumps/rises/landings pass. No fatal HP write in the retained filtered trace. Native ownership/Select and ordinary Ride Armor regressions pass; mounted-during-incoming and other geometry remain unverified. qa-20261020-storage-control/qa-incoming-fixed-zero. |
| X5-02 | Intro falling solid ignores P2 beyond its native threshold | Fixed bounded onD7BAE910 after ADBB6965 differential repro. Narrow F588C projection retains canonical parent combat/collision; actual P2 trigger, native falling/retirement, P1 priority, survivor lifetime and one native wipe/retry pass. qa-environment-20261021/qualification.json. |
| X5-03 | X/P2 C-Shot attached effect reads the other seat's empty projectile slot | Fixed bounded893D5C8A: native46900 now uses existing allocating-seat projection for exact effect5/SHOTS parent. Baseline original FX writes0,0; corrected effect follows native owned shot, accepted hit/own ammo/cleanup pass. Current9796C215 additionally fixes retained UV origin/FX drawing and qualifies F150 original pixels/colors. qa-chaser-20261021/qualification.json. |
| X5-04 | Stage8 first descending support ignores a leading P2 | Fixed bounded07854014 after9796C215 differential repro. Exact originalF48B4 admission runs once with P2 ownership; native shake/release/cameras/carry/stop and survivor/retry checks pass. qa-lift-20261021. |
| X5-05 | Passenger stays hidden at moving-support destination | Fixed bounded07854014 after canonical P1 actual descent stays scene1/3 at parent2128,2128. Native terrain70=0 despite current body71=8/parent72=8/76=1 and exact top contact. Scene return now accepts only current live seat-specific positive top carry plus the original integer geometry. Actual P2-owned return and retried return finish during descent. No generic airborne acceptance. |
| X5-06 | Native loaded-stage earned weapon cycle returns to title/story | Fixed bounded94AA9338 after direct C0DF29A5 causal proof: actual X/P2 weapon5 allocates deferred FX1B/sub3; unprojected50D04 uses Zero graphics+18=00A003F8, writes actor38=80B05FD8, then original182D8/18304 overwrites PLAY0A->00. Reuse existing allocation-owner scope for exact1B/50D04; no new ownership state/schema. Real native card91 cold Load/Zero roster/stage1, full earned0->1->5->8->0, original X+29FC0 upload/retirement, canonical Zero restoration and P2 menu/resume pass. Source card unchanged; no RAM/weapon/progression writes. qa-weapon5-owner-20261021 causal-proof.json and qualified-cycle.json. Other projectile mechanics/campaign branches remain unverified. |
| X5-07 | Surviving P2 Start does not open a native menu | Fixed boundedC0DF29A5. Source/native timeline proves38C08 sets death-only PLAY1C=1; existing living-partner fatal restoration omitted it. Preserve the pre-call byte with freeze state, retaining genuine script locks. Ordinary P1 fatal leaves1C0, P2 native owned menu/resume/Right pass; controlled native hold/fatal/release regression preserves1C1 until real3D2C4. qa-stage8-downstream-20261021/{survivor-pause-fixed,held-native-lock-regression}. |
| X5-08 | Later diagonal support ignores P2 contact and would return to idle under distant P1 | Fixed boundedC0DF29A5 after07854014 P2side-contact/no-phase failure and same-parent native P1 control. ExactF48F8 sees P2's real cached72 history; exactF3930 retains the admitting seat for native proximity/F3/FA/motion/camera. Actual native top admission/carry/motion and bounded save/load continuation pass. Complete ride reaches a separate unresolved terrain/ceiling failure; no full-route pass. |
| X5-09 | Later vertical support ignores P2's actual threshold/top contact | Fixed boundedC0DF29A5 after07854014 P2x3637.5/parentphase4 failure; same P1cross startsphase7. ExactF4968 admission and F4A5C motion/clamp use admitting P2/body/private camera once. Actual native Right/top admission,64.5px upward carry and P2target/currentY1200->1135 pass with P1cameraY1200 unchanged. Later cycles/exit remain unverified. |
| X5-10 | P2 stays in its incoming beam after a real refight return onto a supported hub pad | Fixed bounded84EDFD5A after94AA9338/B257 real CP2->11 failures. P1terrain70=0 but body71=8, live pad72=8/carry76=1 and actualfoot1576 exactly equals padtop1576. The existing exact support contract now qualifies incoming feet; its final commit runs after original world solid callbacks, without another animation/world tick. Actual P2 arrival/control and five coherent both-seat top-contact samples pass. qa-refight-bundle-20261021/{native-hub-stalled-support,actual-hub-contact-audit}.json. Manual Select admission on dynamic supports remains a separate unverified path. |
| X5-11 | P2-owned native negative exit copies P2 HP/ammo into P1's continuation | Fixed bounded84EDFD5A. Actual baseline P1 42HP/288ammo becomes P2's32/282 on native Grizzly return. Original20794 ran under P2's world projection and cached its singleton HP/ammo/selection. On actual mode10->9/policyFF exit, leave_second_context restores P1's continuation cache from its canonical context while preserving P2 separately. Actual refight return retains P1 42/288 and P2 32/282. Controlled native P1 continuations also retain independent resources and absent/fallen P2 without retry loss. qa-refight-bundle-20261021/qualification.json; no new simulation/snapshot state. |
| X5-12 | P2 cannot activate regular capsules or trigger their native top-contact grant sequence | Fixed bounded9FFB6E38. Original55798 proximity and55AD0 contact assume singleton P1. Exact native gates now project the eligible P2 while preserving P1 priority, campaign/shared palette and actual per-seat contact history. Original capsule owns conversation/animation/program grant; normal and fatal cases each have one proximity/one contact handoff. Native loader/rescan/position setup bypasses route/viewport admission. |
| X5-13 | Owner death after a genuine capsule grant strands the survivor in native command18 | Fixed bounded9FFB6E38 afterD425 actual fatal softlock. Native death overwrites F5; original37CB4 then waits forever on new survivorF5=0 because committed capsule phase11 never emits4 again. Existing owner recovery restores that committed native signal only when the original nearby capsule is phase11 with its real program bit. Exactly one recovery and actual survivor ground/control pass. No reward/phase fabrication or new mutable simulation fields. |
| X5-14 | X/P2 capsule dialogue reads Zero's original text bank | Fixed bounded9FFB6E38 after immutable source/live match: Zero90 text3F830B versus X88 text3F654B. Retain each original message member privately; scope original2385C binding to actual regular-capsule recipient, including queued replay. Shared campaign root is restored; deferred original text advances its private cursor. Full live X member69472B matches original, eight native pages and mid-text snapshot/actual return pass. Snapshot provider/schema3 validates immutable message identity/layout; complete mod guest memory uses the existing engine snapshot. Deliberate queued contention/primary Zero recipient/current netplay remain unverified. |
| X5-15 | Genuine offline save/load refuses after live native play | Fixed bounded at a5c9fab/E61EFF70 after actual915 capacity proof42,127,760raw spritewirebytes>16MiB. Lossless schema5 preserves all art and SDKcap; actual26.65MB raw nativecache save/load restores exact retained hashes/actors/menu/time, coldLoad and mapped rewind/control pass.20CTest+618buffer corpus and current264CRC bundle pass. qa-story-snapshot-fixed-20261021/qualification.json. Original failures retained; incompressible wire may still exceed unchanged cap. |
| X5-16 | P2 animated scenery freezes while native world controllers continue | FBE91E18 repairs the actual raw17 shared-atlas fault: BIN27 EF7E0/EF800 origins and EF7FC refresh were missing from P2 saved mode5 camera. Both living panes now animate7/7 and survivor8/8 with native-once clocks, correct source/GPU palette uploads and current combined regression. F175 remains unverified for its distinct continuously scrolling controller; no all-background claim. qa-shared-atlas-fix-20261022/qualification.json. |
| X5-17 | Rectangular black sky gaps are visible at raw1 opening | Present before menu and after private views restore, while terrain/HUD continue. Source background callbacks here are no-op5/5, so no scrolling-callback cause inferred. Original intended completeness/geometry/streaming cause unverified; preserve images and defer. qa-breadth-menu-world-20261021/menu-shared-fades-only. |
| X5-18 | Nearest X/P2 projection deletes the native Zero-story X opponent | Fixed bounded a5c9fab: exact F4D90 branch predicate retains canonical campaign0/1 without changing target body/resources. Actual915 native intro/art/AI/X and Zero ground/air hits, P1fatal survivor interaction and real finalhit->chapter pass; E61 current intro/Xhit regression passes. Full native chapter/ending remains deferred. qa-story-roster-fixed-20261021/qualification.json. |
| X5-19 | Both-door traversal overlaps native story control admission | Failed E61: class40 stand/text begins during doorphase2; opposite X-campaign class41 remains HP0/intro2/0 for180s because F8D08 requires stand14 while both heroes keep4. Native pause also blocked. Full-init deferral tried on75 instead stalls aperture and was removed. Exact init/camera versus control-admission ordering remains prerequisite; no more variants now. qa-story-other-campaign-20261021/qualification.json and qa-story-snapshot-fixed-20261021/native-story-entry/report.json. |
| X5-20 | Diagnostic session metadata omits both forms and boss-balance choices | Fixed D1B93AFC: all10 declared effective options, with typed booleans/choices, are captured through the existing runtime option API. Copied offline preferences can disagree with host choices. Regression first fails on the omitted field; fresh21/21 checks and both actual current peer metadata records pass. Existing v2 inputs, build/plan checks, defaults, baseline/replay restrictions and bounded writer remain unchanged. qa-logger-complete-options-20261022/observer-corrected/qualification.json. |



## Source review map

X4's original 26-item list is in `COOP_PLAYTEST_REGRESSIONS.md`; the expanded
numbered list and recurring reports are in `COOP_PLAYTEST_FEEDBACK_20261010.md`.
Later disposition/evidence documents qualify several previously checked items
only narrowly. Keep those qualifications when translating them to X5.
The [reference inventory](COOP_X4_REFERENCE.md) retains all234 entries (26 original,
208 expanded through Playtest42), including overlapping/recurring reports, separately;
none inherits an X5 pass from its X4 checkbox.

Review source by behavior: lifecycle and bosses (P10-19,22,24-30), combat and
balance (P07-09,23), audio (P21), split/texture/assets (P07,20,25,31-33,37),
diagnostics and startup (P02-04), debug protocol/netplay (P05,34-35).

## Current validation

The merged debug candidate builds and all15 CTests pass. `qa-first-x` verifies
independent controls/attacks and once-per-world accounting; captures were
inspected. `qa-replay-first-x` verifies2704 retained original-disc inputs on
that exact binary. `qa-zero-owned` now observes native Zero selection before
confirmation and verifies Zero/P1 plus X/P2 controls; captures inspected.

`qa-zero-ownership` fails its pickup-health wait. The run then has fallen P2
and a native pause; the dependent continuation check fails its two-live-actors
precondition. Preserve both failures, but do not infer a pickup/continuation
regression from this exposed intro setup. Current setup fixtures and the
location driver are being qualified before continuing stage-specific checks.

The corrected mode9 checkpoint fixture loads/samples Grizzly checkpoints0,1,4
in `qa-checkpoint-boundary-x`; original arrival and requested indices are
observed. The earlier mission-title camera prematurely consumed a checkpoint
request; retained `qa-checkpoint-smoke-x` fails that setup assertion.

Capture review exposed a measurement defect: the older helper selected
`screenshot_hires`, a pre-fit buffer which omitted P1's host-rendered meter.
Actual composed presents in `qa-hud-live` show the P1 meter and recording
overlay. Do not call the omitted P1 meter a game regression. P2's stacked meter
still needs investigation. The helper now prefers completed host presents and
labels explicit headless buffer captures with a sidecar instead of silently
claiming that they represent the whole presentation.

The candidate `ee51a200264c3c1c6ad8511b13f66e520e2f371a69a165f0636b5c1dee174ef1`
loads/samples all seven non-Chaser Maverick entries. The stage3 entry legitimately
changes checkpoint0 during native arrival; the audit now verifies a receipt from
the actual initializer, then separately records the later native checkpoint.
This avoids treating natural progression as a failed setup.

`qa-ownership-contact-x` passes controlled native health pickup, shared tank,
pause ownership, withdrawal/rejoin and scripted passenger resource preservation.
Contact grace is held explicitly for this setup; incoming damage, Armor contacts
and actual doors are not qualified by those checks. `qa-continuation-contact-x`
passes alive/absent/fallen P2 same-area continuation with separate resources and
no retry consumed. The suffix of Normal/Virus text was visibly separated in wide
captures; the shifted native glyphs now receive the same explicit left anchor,
with a fresh-candidate capture still pending.

Direct offline fixtures now enqueue provenance rows in diagnostic streams.
Input-only replay preparation rejects these traces even if the fixture occurs
in the final incomplete interval. Ordinary pad-only sessions retain replay.

`qa-side-by-side-maverick-zero` samples stage1/area0 and stage2/areas0,1 with
Zero/P1 plus X/P2 and side-by-side meters; composed captures inspect both
portraits and intact native Normal text. `qa-local-pair-x` is retained as a
failed timing assertion: restoration verification substantially slows private
draws, so a fixed200ms tick threshold was inadequate. The driver now waits for
eight actual world updates. Subsequent inspection in `qa-local-pair-restoration-first`
observes3674 reversible draw checks with zero mismatch/abort/device reads.
The first pair at4:3 still used the retail tile ring because the wide-view
anchor was unavailable; that is not evidence for distant terrain. The authored
renderer now handles that case, with a new candidate check pending.

`qa-local-pair-post-midboss-x` verifies two settled visible heroes, independent
right inputs, distant authored terrain, equal guest-cycle pair capture, host
resize and P2 pause/resume. It runs5882 restoration checks without mismatch,
watchdog, device read or VRAM leak. Visual inspection then finds a real defect:
P1's distant sprite wraps by2048 pixels into P2's pane. The current source adds
X4's conservative per-view draw culling before GP0 truncation; a fresh binary
must remove the ghost before this behavior receives a pass. The earlier
`stereo_stats.eye_hashes` assertion was a driver error: those are restored
machine hashes, not pixel hashes. Composed pane hashes are recorded separately.

Presentation setup after checkpoint0 also bypassed the midboss and its narrow
camera-region triggers, leaving P2 above the inherited camera. That fixture
does not prove a normal-traversal camera bug. The later fixture derives a native
settled landing at checkpoint2 and begins at checkpoint1 after the midboss.
Likewise `qa-area2-p1-leads-local` places its leader in lethal terrain before
the exit test; retain it as failed setup, not a failed exit-controller claim.

The lower approach atY384 succeeds in `qa-area2-p1-lower-approach`: P1's
native controller requests the exit, the original loader advances stage2 from
area0 to area1, both actors arrive, and boss completion flags remain unchanged.
Position/contact setup bypasses traversal and hazards; this is a bounded real
transition check, not full stage clearance. P2 leadership and retained-seat
cases are being checked separately.

`qa-area2-p2-native-waiting` confirms the corresponding P2 failure with both
heroes alive: P1 remains at native checkpoint1 (3472,363), P2 reaches5432 and
the original controller still reads P1. The current dispatcher selects P2 only
at that native exit boundary, with exact stage/overlay/type/opcode guards; the
original BIN2 controller and original loader produce the exit/area flags.
`qa-area2-absent-before-fix` confirms that the native forward spawn policy0
unintentionally rejoined withdrawn P2. Current source retains that deliberate
absence and private resources across a forward section while allowing native
fallen-player recovery. `qa-area-fixed-zero-p2_absent` passes the actual forward
transition with P2 still absent and HP17/ammo73 preserved. Both fatal orders
pass native forward recovery in `qa-area-fixed-x-split-p1_fallen` and
`qa-area-fixed-x-split-p2_fallen`; no retry, roster or boss-flag change occurs.
P2 leadership passes on executableD6206EB1 in `qa-reserved-exit-zero-p2`.
The original exit advances the actual area and preserves retries, roster and
boss flags. A call trace locates its controller
in the reserved actor pool at800C94A8 via228D8/22920, rather than the ordinary
stage pool; the first wrong-pool attempt and failures remain retained.

The first sprite-cull attempt also skipped P2's injection boundary when P1 was
offscreen; that boundary is now retained while each body is culled separately.
Native24698 additionally calls27370 after PLAYER for Fourth Armor, which emits
through26CAC instead of the normal body renderer. That separate path caused
the surviving blue ghost. It now has its own exact native hook and view cull.

`qa-local-pair-armor-qualified-x` confirms that the Fourth Armor ghost is gone
in the composed distant view, with both heroes visible on different authored
terrain. Independent control, native menu return, resize and1164 reversible
checks pass on executableD2A54FAF. `qa-continuation-checkpoint-fixture` separately
qualifies exact original checkpoint receipts for Grizzly1,2,4, using the native
same-area continuation after the first load. These are loader samples, not
passes for checkpoint activation, midboss prerequisites or stage mechanics.

The debug-OFF, qualification-cheats-OFF build compiles in
`build-parity-production` and all15 CTests pass. Its staged catalog contains
eight declared packages and excludes CD Speed/Fast Loading. A final playtest
archive, production runtime gameplay and campaign coverage are still pending.

The reserved-controller candidateD6206EB1 passes all five stage2 forward-exit
cases with Zero/P1 in Unified: either leader, either fallen seat and withdrawn
P2 (`qa-reserved-exit-zero-*`). Composed destination captures inspect both
heroes and meters. Both debug and production configurations pass all15 CTests.

The full checkpoint sampler first reaches intro checkpoints0,1,2, then fails
the native control/arrival wait at checkpoint3 (`qa-content-sweep-local-x`).
Both heroes fall during this late intro setup and reach native Game Over.
The checkpoint fixture bypasses earlier intro events; retain this failed setup
without claiming a checkpoint, terrain or boss pass. The remaining locations
still require sampling and separate mechanics checks.

`qa-reserved-exit-x-split-p2` passes the actual P2-led exit with P1 still on
its native checkpoint platform. The Split netplay loopback run on59F1E66D
passes five boundaries with169 matched core/mod CRC samples; actual host
captures inspect each peer's own meter and camera. This qualifies that exact
build on this host, not cross-build or cross-platform compatibility. The test
uses the full staged catalog, muted output and completed host captures.

The next sampler reaches all five Grizzly checkpoints and all five stage2
checkpoints, then loses both heroes during stage3 entry before control is
available (`qa-content-sweep-maverick-local-x`). Isolated Split and Unified
loads are being compared before attributing the failure to gameplay or setup.
Isolated stage3 Split entry passes, as does stage3 after a fresh stage2 checkpoint2
load. Unified samples all seven stage3 records and all five stage4 records.
The sustained pass reaches Chaser entry before both heroes fall; the next
request is correctly rejected during native death (`qa-content-stages4-plus-unified-x`).
These are samples, not Chaser survival or damage passes. A same-area continuation
leaves native spawn policyFF; a fresh mission normally resets it at20D10. The
mission-entry fixture must reproduce that reset before continuing dense audits.

The before-fix control sequence stage2/area1/checkpoint1, same-area checkpoint2,
then stage3 reproduces the failure (`qa-stage3-after-continuation-control-x`).
The corrected fresh-entry fixture clears checkpoint/spawn policy exactly as
original20D0C/20D10; the identical sequence now loads/samples all three records
in `qa-stage3-fresh-policy-split-x`. This is an audit-setup fix; ordinary native
mission progression already owns that reset. All204 reference rows now have
explicit symptom-based family mappings, with12 links to bounded X5 evidence.

The second loopback run uses Zero/P1, Unified and30ms delay with10ms jitter.
It passes both owned menus and controls with156 matching core/mod CRC samples
(`qa-parity-netplay-unified-zero-jitter-retry`). The first run is retained as
a3-second debug-socket timeout; the helper now allows10 seconds for native
blocking phases while retaining its separate simulation-progress deadline.

The corrected-fixture sweep samples five of seven stage6 records, all eight
stage7 records, all five stage8 records and stage9/10 entries. Stage6/area1
checkpoints2/4 kill both heroes; original Game Over recovery is retained as
a separate setup bypass. Stage11 entry reaches mode10 with no native bodies
and no co-op enrollment, then cannot reach a recoverable native menu. Remaining
reused/scripted tables and the failed prerequisites remain unverified
(`qa-content-stages6-plus-split-x`). No boss/exit or terrain pass follows from
these samples. Raw stage11 records include zero coordinates; original1368C..136C4
has a checkpoint-specific loader using8006FD80 before the ordinary overlay.
These records have not been classified as ordinary playable fortress stages.

The private character-audio adapter now owns encoded banks before DAT release
and uses original X5 tone calculations with private P2 channels. Bounded X/P1
runtime jump/attack/overlap/menu checks pass in both rosters on candidate7C3E87D3.
The deferred-effect ownership fix qualifies exact X/P2 record8 buster samples,
native pitch/ADSR and independent slot45, with both menu returns. Original
record1 jump identity and simultaneous overlap also pass. Debug and production
both pass17 CTests. See `COOP_AUDIO_PARITY_EVIDENCE.md` for exact limits.

The charge harness distinguishes original subtype18 (base X/P2) from20 (Fourth
Armor/P1) at4F21C..4F278. Nine base-X/P2 repeats pass; older Fourth Armor runs
retain failures from the subtype assumption and a missed short counter window.
The revised check traces original2D078 allocator returns from4F768 directly;
its first run is interrupted by the separate watchdog exit, so it has no pass.
Native enemy damage, special weapons and visual charge/trail parity stay open.

The Chaser harness now retains incremental/error reports even after a runtime
disconnect, waits through original retry arrival before a new mission, records
any seeded retry budget, and keeps Right held through the setup chord. Its
position fixture moves existing rider/car positions and histories only, under
qualification cheats, with diagnostic provenance; velocity/contact/scene clocks
and success/retry flags remain native. No additional car mechanic is passed
merely by adding this fixture. Failed attempts and cleanup/parking errors remain
under their individual `qa-parity-chaser-*` directories.

No campaign parity is claimed. Detailed evidence is under ignored `build-parity/qa-*`.

## 2026-10-20 continuation

CandidateB7358CFD exits through the enabled starvation watchdog during a paced
boot (`qa-20261020-vehicle-zero`) and unpaced native intro
(`qa-20261020-unpaced-zero`). Both postmortems are retained beside their reports.
A same-binary control with resident archive loading disabled reaches native
Zero/P1 + X/P2 and remains responsive (`qa-20261020-native-loading-zero`). This
comparison does not establish a root cause or qualify default loading settings.
The boot helper now records this setting and preserves process postmortems.

That control reproduces an X4 delayed-attack edge case: X/P2's pending
type2/subtype18 emitter calls original projectile allocator8002D078 from4F768
at frame6805, after fatal-fixture acknowledgement at frame6804
(`qa-20261020-buster-fatal-control-zero`). The probe completing is evidence of
the defect, not a mechanics pass. Owner-specific cleanup now clears shared
emitters at withdrawal, passenger attack reset and native fatal entry. Its
focused runtime regression passes onAA1105D6 with resident loading disabled:
one pending emitter is canceled, zero later native allocator calls occur, and
P1 remains alive (`qa-20261020-buster-fatal-fixed-native-zero`). Nine charge
releases also pass with one original child allocation each, private initialized
assembly, the other hero's counter preserved and expiry before/after both menus
(`qa-20261020-charge-initialized-native-zero`). The prior charge attempt caught
a newly allocated mode0 emitter with its expected zero assembly pointer;
the harness now waits for original initialization before checking that pointer.
Composed charged captures show both heroes/meters after P2's menu, without
qualifying charge-glow animation or trails. The reverse roster also passes nine
Fourth Armor/P1 subtype20 releases and pending-emitter cancellation with P2
remaining alive (`qa-20261020-charge-initialized-native-x`,
`qa-20261020-buster-fatal-fixed-native-x`). Withdrawal/passenger reset paths
remain unverified.

Default-loadingAA1105D6 boot reaches gameplay, but the dependent Grizzly reload
exits through the watchdog before the fatal regression reaches setup
(`qa-20261020-buster-fatal-fixed-zero`). Retained overlay-capture write errors
are a separate open observation; neither their cause nor their connection to
the watchdog is established. The failed prerequisite is not a cleanup-test pass.

Chaser setup attempts retain failures from an early Start edge at the native
pause gate, a depleted retry budget and a wait for PLAY+1C to clear. Original
20DA8..20DB0 uses that byte to gate pause; it remains set while native car
controls work. It is not a retry-arrival completion flag. Separate one-read
probes observe P2 Cross as native bit80, jump mode5=2 and upward speed6.5 both
normally and after P1 fatal entry (`qa-20261020-chaser-input-normal-zero`,
`qa-20261020-chaser-input-p1-fatal-zero`). Those probes qualify control only.

The corrected harness takes a coherent diagnostic read, removes the invalid
pause-gate retry wait and records each seeded budget. CandidateAA1105D6 passes
the whole bounded sequence in `qa-20261020-chaser-compact-both-native-zero`:
180-update Select lock, both initial fatal seats, native survivor forward
movement/jump, once-per-world clock and a single3->2 shared retry per wipe.
The existing car/rider position fixture is accepted for both survivor seats;
it preserves native velocity/contact/scene flags. Both composed survivor
captures inspect the rider pose and facing. Resident loading is disabled,
three retries are seeded per scenario, and survivor positions/histories are
repositioned. Full road traversal, READY completion, damage and campaign
transitions remain unverified. The verified owned process was stopped after
the report, with no cleanup error. Earlier failed runs retain their failures.

AA1105D6 also passes the eight original-tone audio cases with X/P1 + Zero/P2
(`qa-20261020-audio-cleanup-native-x`), giving both roster orders on this
candidate. The following Ride Armor check passes native X/P1 mount, movement
and dismount, Zero/P2 staying on foot, and P2's survivor world/clock after a
dismounted P1 fatal fixture (`qa-20261020-armor-native-x`). Its composed mount
and survivor captures were inspected. This retains the explicit authored
mount-position/HP protection bypasses and resident-loading-off control;
mounted fatal entry and Armor damage/destruction are not qualified.


The reverse-roster Chaser sequence also passes onAA1105D6 with the same explicit
loading/position/retry controls (`qa-20261020-chaser-compact-both-native-x`).
This extends the bounded two-fatal-order result to both rosters.

The content continuation samples all13 original records in stage IDs16,17,18
and22 (`qa-20261020-content-late-native-zero`, AA1105D6, resident loading off).
These remain observations, with terrain, enemies, audio, checkpoint/death,
bosses and exits individually unverified. Stage12 checkpoints0/1 sample, but
checkpoint2 times out with P1 idle and P2 JOINING at760,-16; native menu recovery
also times out (`qa-20261020-content-stage12-native-zero`). Scene owner/phase
are both zero. The direct checkpoint bypasses prior refight-pad prerequisites;
the cause is unassigned. The other19 stage12 records were not attempted.

Original personal pools800942A8 (three50h glow actors) and8009A2F8 (three60h
trails) exposed a real missing P2 update/render path. The old composed charge
probe retains P2X at2464,234 while shared glow slots track idle P1Zero at2528,234
and stay hidden (`qa-20261020-charge-glow-pool-zero`). The personal-effects
change captures/projects both pools, runs only their original late update
functions22EDC/22F3C once for P2, and renders their native actors with private
assets in ordinary and survivor passes. Reset paths hide state while preserving
the permanent native slot templates needed after rejoin.

Candidate3FE6E7DC passes four native P2X dash/charge pairs (before menus, after
each owned menu, after rejoin), charged withdrawal cleanup and survivor charge
(`qa-20261020-personal-visuals-withdraw-zero`). Exact expected body/trail and
glow assembly pointers derive from original DAT headers: native3BD64 uses
header+0, while3BE30/3BE50 uses header+70. The first regression's exact glow/body
equality assertion was incorrect and its failed report is retained. Corrected
checks also verify own-hero positions, preserved P1 state, expiry/release and
once-per-world/native timers. Composed glow/trail captures were inspected.
This is an explicit resident-loading-off control, with checkpoint, HP/contact
and position/history setup bypasses; broader combat/campaign remains open.


The personal-effects candidate passes the nine charge allocation/expiry cases
and pending-emitter cancellation again (`qa-20261020-personal-effects-charge-regression-zero`, `qa-20261020-personal-effects-fatal-regression-zero`). Fatal
entry additionally leaves no visible private glow/trails. The local two-peer
Split/Zero run passes seven boundaries with172 matched core/mod CRC samples,
including active native charge glow, release and both menus
(`qa-20261020-personal-effects-netplay-split-zero`). This qualifies that exact
build on this host; no cross-build/platform compatibility is inferred.

Zero/P2 trails also pass four phases in local Split, with menu/withdrawal/rejoin
and exact original character assembly identity (`qa-20261020-personal-visuals-x-split`). The following distant-view check passes independent controls,
three window sizes, P2 pause/resume, equal guest cycle and2352 private render
restoration checks with zero mismatch, watchdog, device reads, VRAM leaks or
canonical host-state leak (`qa-20261020-personal-local-pair-x-split`). Standalone
renderer preference is off; local Split derives the authored terrain renderer.
Composed distant/resize and Zero-trail captures were inspected. Explicit
resident-loading-off and checkpoint/position/HP/contact controls still apply.


The reverse-roster local Split fatal regression passes pending-emitter
cancellation, no later native child allocation, hidden personal glow/trails
and a living P2 (`qa-20261020-personal-fatal-x-split`). The first Split/X jitter
run matches164 CRC samples through the P2 menu but stops at P1 menu closure: one
asynchronous observation sees peer0 minor0 and peer1 minor2. The fixed60-tick
wait is insufficient to assert a shared native phase boundary; that failure
is retained (`qa-20261020-personal-effects-netplay-split-x-jitter`). The helper
now waits for both original menus to become interactive and both to close
within bounded deadlines. The same-binary retry passes all seven boundaries with164 matched core/mod CRC samples (`qa-20261020-personal-effects-netplay-split-x-jitter-boundary`). Final probes also check the complete charge/menu sequence:434/435 restoration checks with zero mismatch, watchdog, device reads, VRAM or host-state leak. This qualifies local same-build peers with30ms delay/10ms jitter; cross-build/platform agreement remains unverified.

## Native refight arrival correction

The actual hub pad reproduces a Split scene deadlock on3FE6E7DC
(`qa-20261020-refight-native-pad-contact-x-split`). Original kind4/type23
root800EF65C calls collision helper800EF57C, selects checkpoint2 and reloads
through mode9. The boss then acquires incoming P2 as scene owner2/phase3. P1
is beamed out at736,-53 while P2 remains JOINING at760,8; world ticks continue.
This is native warp evidence, separate from the older direct-checkpoint setup
failure whose cause remains unassigned. The first approach80px above the pad
did not trigger it; the native collision tables identify the accepted approach.

`mmx5_coop_living` includes joining/leaving actors for lifecycle and camera
purposes. AI selection now separately requires ALIVE, and script acquisition
rejects incoming/outgoing owners. Ordinary native teleport/victory poses keep
their AI context. The preliminary9124B331 candidate passes X/P1 pad2 and pads3-8
(`qa-20261020-refight-pad2-fixed-x-split`,
`qa-20261020-refight-other-pads-fixed-x-split`). Its pad9 setup instead places P1
beyond the platform and falls before activation; that failure is retained.
An inward approach passes pad9 (`qa-20261020-refight-pad9-inward-fixed-x-split`).

The final eligibility candidateF47573A8 and production85EF45B0 each build and
pass17 CTests. Its Zero/P1 pad2 regression also observes and cancels one pending
X/P2 charge emitter at passenger reset, with zero subsequent original allocator
calls from4F768 (`qa-20261020-refight-pending-passenger-zero-split`). Both actors
finish native arrival and script unlock. All eight Zero/P1 pads now pass (`qa-20261020-refight-other-pads-zero-split` for3-9); these are
arrival checks, with native boss defeat/rewards/return pads still unverified.

The same final candidate passes nine loopback Split/Zero boundaries with184
matched core/mod CRC samples (`qa-20261020-refight-eligibility-netplay-zero-withdrawal`): native Select hold, absence/spectator, rejoin/private HP/ammo,
active charge/release and both owned menus. Final restoration checks839/838
have zero mismatch, watchdog, device reads, VRAM or canonical host-state leak.
The hold driver waits for the native lifecycle boundary; network ticks alone
do not represent the90-world-update threshold. Local same-build evidence does
not establish cross-build/platform agreement.

FinalF47573A8 cumulative refight render restoration passes4714 checks with zero mismatch, watchdog, device reads, VRAM or host-state leak (`qa-20261020-refight-restoration-zero-split`). This measures private-draw isolation; campaign/boss/return-pad checks remain open.


## P2-led native refight pads

The first-contact control onF47573A8 passes with P1Zero, while P2X remains
alive at3584,1555 for20 seconds without acquisition (`qa-20261020-refight-first-contact-p1-zero-split`, `qa-20261020-refight-first-contact-p2-zero-split`).
The earlier inside-box P2 approach falls before activation and is retained as
a failed setup. The refined fixture uses the original box extents to touch
the pad from above, preserving its one-way platform prerequisite.

CandidateC66C2B20 (production15EBAE4A) adds a guarded handoff at original
kind4 dispatch225D8 for stage12/BIN18 idle refight pads2-9. OriginalEF57C
geometry keeps P1 priority and tests P2; the existing world scope runs the pad
once under P2. Native3D29C, pad timing, checkpoint choice, fade and mode9
loader still perform the transition. Both configurations pass17 CTests.

All eight P2-led pads have bounded Zero/P1 + X/P2 arrival passes, each with
exactly one P2 handoff (`qa-20261020-refight-p2-pads2-3-zero-split`,
`qa-20261020-refight-p2-other-pads-zero-split` for4-6,
`qa-20261020-refight-p2-isolated-zero-split/pads7-9`). The P1-led pad2 control
retains zero P2 handoffs (`qa-20261020-refight-p1-retained-zero-split`). The
isolated P2 stage2 area-exit regression also passes after those warps
(`qa-20261020-refight-p2-isolated-zero-split/area-p2-regression`), retaining
the original reserved-controller path. Explicit checkpoint, HP/contact and
position/history setup bypasses remain; boss defeats/rewards/returns and
script-owner fatal cases are still open. The reverse roster X/P1 + Zero/P2 also passes all eight P2-led pads on the
exact C66 binary (`qa-20261020-refight-p2-reverse-x-split/pads2-9`), one handoff
per pad and3482 cumulative private-render restoration checks with zero faults.
The pad8 composed arrival capture was inspected. This is another explicit
resident-loading-off control; no boss outcome or long-session stability pass
is inferred.

## Watchdog and capture IO

The sustained C66C2B20 pad7 run reaches both native arrivals and one P2 handoff
before its present capture times out and the process exits via the enabled
watchdog after6.391704 seconds without a heartbeat. Pad7 remains failed in
that report; pads8/9 were not attempted. Postmortems, startup dump and exact
C66 symbols are retained beside the failed report. This run has resident
loading OFF, so that preference cannot explain all P37 failures. Repeated
additive-capture write errors precede the exit; their connection is unassigned.
The named contribution exists as a regular15764-byte Archive file.

The isolated same-binary retry of pads7-9 and subsequent area exit passes
with resident loading OFF and a fresh working directory, after the other
owned runtime was stopped. This changes both history and concurrent load;
it does not establish a root cause or close the retained watchdog failure.

An independent C66 default-loading X/P1 + Zero/P2 run passes boot, Grizzly
reload, all nine charged-release/menu cases and a P1-led native pad2 arrival
in Unified (`qa-20261020-default-loading-c66-x`). Its verified49319936-byte
resident pack serves33 of35 observed archive requests; the raw service ledger is
retained. These are bounded default-setting passes, not long-session stability.

Local framework issue `beads-eio.3.370` tracks capture IO diagnosis. Independent local
copies of overlay_capture.c and overlay_loader.c use operation/error/latency,
lock/join and manifest-scan timing through an ignored CMake source override.
The first capture-only probe352A8FCC passes17 CTests and a complete original
P2-pad replay (2-3, P1 control2, then P2 pads4-9) without reproducing abnormal
IO or the watchdog exit. A successful600ms commit and3500ms close occur on
a capture worker thread; those waits do not establish main-thread starvation.
The expanded capture/loader probeC3CDE9A3 also passes17 CTests and the same
complete replay (`qa-20261020-overlay-io-loader-probe-zero-split`). It records
126 timing/error rows with no abnormal operation failure. Expected missing
read probes are distinguished from write failures. Its longest initial scan
is2997ms before gameplay; gameplay lazy loading peaks at471ms. Capture-worker
operations reach1607ms for replacement,1416ms for commit and1269ms for close,
with no measured mutex wait or worker join at least100ms. Successful operations'
stale errno values are not failures. No watchdog exit is reproduced, and these
instrumented, isolated runs do not resolve the earlier uninstrumented failure.
Shared framework/UI trees remain
unchanged. This diagnostic build is separate from the C66 gameplay evidence
and production build; no watchdog setting is altered. No remote sync/publication.


## Native door setup limits

The first Grizzly door attempts onC66 remain failed setups under
`qa-20261020-door-native-x-split`: `p1-control` loses P1 before the door is
active; `p1-checkpoint4` attempts to rewind behind already committed camera
bounds; `p1-checkpoint3-y75` and `p1-native-camera-route-foot` stop before a
valid original door acquisition is observed. No door or owner-fatal pass is
inferred from these attempts. All positions and native actor/camera samples
are retained. Original camera-region rectangles11/16 identify an approach,
but placement activation and settled native player control still require
explicit boundary checks. The driver is being tightened before repetition.

The separate controlled original3D29C stand-lock /3D2C4 release check below
compares normal passenger return with native owner fatal entry. It does not
qualify actual door collision/controller behavior or a committed destination.


## Controlled script-owner fatal recovery

The first two stand-lock controls stop during setup because they incorrectly
require current/cached P2 ground bits after the constructed arrival
(`qa-20261020-scene-fatal-zero-split/control`, `control-native-contacts`).
The corrected control validates ordinary original movement, jump and landing
instead (`control-native-landing`, C3CDE9A3). It passes original3D29C lock,
P2 native departure, original3D2C4 release and private HP17/ammo73.

On the same binary, `fatal-owner` accepts a native fatal Zero/P1 at
scene1/phase3 while X/P2 remains alive and hidden in outgoing body state3.
The next survivor-world projection lets original20E48 interpret that body as
death: retries2->1, native checkpoint reload, P2 HP17->32 and ammo73->288.
No P2 fatal entry is requested. This is an unwanted team retry, separate from
the earlier incoming-owner refight deadlock. No actual door is qualified.

CandidateF69A15A1 (productionFB959240) restores a living foot passenger and
hands the native command/door initialization to it before that shared stage
pass. It preserves the committed script position/camera and private body
resources, clears the dead owner's script locks, and lets native death finish.
Mounted passengers still require their vehicle transfer/dismount path; that
case remains open. No new persistent host state is introduced.

Both configurations pass17 CTests. The uninstrumented Zero/P1 + X/P2 Split
control and fatal regression pass (`qa-20261020-scene-recovery-zero-split`),
with exactly0/1 handoffs, unchanged retries/boss flags, retained HP17/ammo73,
real survivor movement/jump/landing and zero private-render verifier faults.
Both healthy P1/P2 original pad2 regressions pass afterward. The additional
`fatal-owner-held` case retains the native script lock until P1 finishes dying,
then explicitly releases it and passes the same native controls/resources.
Its first capture follows native immunity expiry, but the hero is absent in
the sampled present image; visual presence is being checked separately. The
earlier capture may be an immunity blink. The composed captures were
inspected. The pre-extension harness source is retained beside the reports. This is a resident-loading-off,
controlled native stand-lock fixture; actual door/controller/warp commitment,
P2 owner fatal, reverse roster, mounted passengers and netplay remain unverified.
The capture/loader source override has been removed from the normal debug
configuration; the C3 diagnostic executable and its exact symbols are retained.


The F69 native-control passes do not qualify survivor visibility: repeated
composed captures omit P2 even after immunity expires. Native24698 checks
P1's draw byte before calling24C60, while the private-view partner append is
attached to that skipped call. An explicit partner append after the native
private redraw now covers a hidden primary body; ordinary world drawing
already runs with its survivor context. Candidate0B3A9238 (production76AC9F40) passes17 CTests in both configurations
and is running the runtime regression. The regression now requires actual P2 body quads during private redraws
after P1 is fallen, plus a delayed immunity-free composed capture.


On0B3A9238 the private redraw emits958 native P2 body quads after P1 is fallen,
but the composed image still omits the hero. Original24698 calls24B88 to link
actor groups into the submitted ordering table before returning. The append
must run before that linking step, rather than after272B4. The next revision
uses the already configured24B88 entry (return address24B70) inside private
redraws. Sprite emission by itself does not establish presentation.

The same0B3 candidate passes actual native Grizzly P2-led encounter control
and a P2-owner fatal case (`qa-20261020-scene-visibility-zero-split/native-boss-control-p2`, `native-boss-fatal-p2`). Original BIN1 boss acquisition hides
P1, native fatal entry kills P2, exactly one handoff selects living P1, and the
original boss controller finishes through native confirmation input. P1 keeps
HP17/ammo73, retries/boss completion flags stay unchanged, P2 finishes falling,
and P1 regains native movement. The composed image visibly contains Zero/P1.
The corridor/checkpoint4 bypasses the preceding door; no boss defeat/reward
or door pass is inferred. Red background bands in the encounter image require
a native-renderer reference comparison before terrain/palette qualification.


CandidateC0909A28 (production01CC2E1E) passes17 CTests in each configuration.
Its controlled Zero/P1 fatal/held-lock regression passes and the composed
capture visibly shows X/P2 in both panes after P1 falls
(`qa-20261020-scene-sort-zero-split/fatal-owner`), with938 native P2 body quads
and zero restoration faults. The native P1-led boss fatal case reaches one
handoff and controller release with retained resources/retries, then fails
rightward movement at the entry step. That report remains failed.

The native boxes800748D0/D4 are00020B13 /00030B13: X's bottom is center+21,
Zero's center+22. Copying dead Zero's Y378 leaves X's feet one pixel above the
Y400 floor. Native Left moves X within9704.5..9718, while Right stops at9718.
A diagnostic position/history-only correction toY379 permits native Right
movement to9729.5 (`foot-alignment-probe.json`); no contact or success flag is
written. This bypass diagnoses the cause and does not pass the failed case.
The next revision aligns native foot boxes for enrollment/rejoin, ordinary
script return and fatal-owner recovery, with no new persistent host fields.


CandidateE97D2934 (productionEA26BC60) passes17 CTests in each configuration.
Its complete Zero/P1 + X/P2 resident-off Split sequence passes
(`qa-20261020-scene-foot-zero-split`): normal stand-lock return, owner-fatal
return with the lock held until native death finishes, actual native Grizzly
P1-led scene control, actual P1-owner fatal, actual P2-owner fatal, and healthy
P1/P2-led refight pad2 regressions. All three boss cases share native foot
pointY400 (26214400 fixed), retain passenger HP17/ammo73/retries/boss completion
flags, and pass native movement. Handoffs are0 for the healthy control and1
per fatal case. The P1-fatal composed image visibly contains X/P2 in both panes;
restoration checks reach3532 by the opposite-owner boss case with zero faults.
This does not qualify the bypassed preceding door, boss defeat/reward, combat
damage, the red background bands or other stages. Opposite roster/default
loading and same-build loopback qualification are next.


The same E97 candidate passes all ten private-effect visual cases on that
owned Zero/P1 Split session (`personal-effects-survivor-regression`): native
dash trails and charge glow before menus, after each owned menu and after
charged withdrawal/rejoin, plus X/P2 survivor charge after P1 fatal entry.
Native asset identity, effect cleanup/expiry and once-per-world timing checks
pass. The composed charge capture was inspected: it catches P1 at early native
fatal entry, while its old body is still visible. It does not establish charge
presentation after P1 becomes hidden. That stronger boundary is being added.
The withdrawal/rejoin cases exercise the adjusted foot target.
The original DAT hash and explicit protection/position bypasses are retained.


The opposite E97 roster also passes the complete default-loading Split sequence
(`qa-20261020-scene-foot-x-default-split`): X/P1 + Zero/P2 stand-lock control,
held owner fatal, actual native Grizzly P1 control and each owner fatal, all
eight P2-led original refight pads2-9 and a P1 pad2 control. Actual boss cases
retain HP17/ammo73/retries/boss flags, identical native foot points and native
movement. Both fatal composed images were inspected: the living hero appears
in both panes after the dead owner falls. The same red backdrop bands remain
unqualified. `run/resident-observation.json` records a verified49,319,936-byte
pack and133 resident-served requests of147 total events; this is positive
default-loading evidence, not an explanation for the earlier watchdog exits.


The strengthened hidden-primary glow check passes on a fresh E97 Zero/P1
Split session with default loading (`qa-20261020-hidden-survivor-zero-default`).
The survivor-only harness waits for life[5,0], P1 draw0 and native immunity
expiry before sampling twelve more world updates. It records44 additional
P2 glow draws after P1 is hidden, verifies original DAT assembly identity,
charge/follow/release and world clocks, and retains the harness source. The
composed capture was inspected: X/P2 and its small charged glow are visible in
both panes; the outward native death rings are still present. This closes the
specific visibility boundary left open by the earlier early-DYING capture;
it does not qualify complete death-effect cleanup or other effect classes.


The tightened original-door driver still fails setup on E97 Zero/P1 default
loading (`qa-20261020-door-foot-zero-default/p1-control`). After the completed
P1-fallen glow case, checkpoint3 reaches mode10/minor0/stage1 but no published
co-op body or enrollment appears before the90-second arrival deadline. The
final coherent diagnostic body/lifecycle/world observations, capture and exact
driver source are retained. These are diagnostic snapshots, not a direct
observation of the live original P1 body, so no original spawn failure is
established. No approach position was submitted and no door was activated.
The original initializer sets the gate, and this driver refused Cross in
mode10 before enrollment; the revised setup permits that native confirmation
only before a new enrollment. A fresh-session control remains necessary.


The first E97 same-build Split loopback scene-fatal attempt
(`qa-20261020-scene-netplay-zero-p1`) passes intro, both movement/attack inputs,
active charge/release and both menus with matching core/mod CRCs, then fails
its Grizzly arrival setup deadline. Replicated HP-refill and mission-entry
commands are acknowledged identically. The original mission intro remains
gated in mode10 because the driver only confirms titles/briefing modes. The
retained harness identifies that limit; no native boss scene/fatal was reached
and no scene recovery or network disagreement is inferred. The next driver
permits native confirmation for fresh mission introduction while continuing
to avoid confirmation during the controlled checkpoint4 boss approach.


The corrected E97 Zero/P1 + X/P2 Split loopback case passes
(`qa-20261020-scene-netplay-zero-p1-dialogue`):295 matching core/mod CRC samples
through native intro, both movement/attack inputs, active charge/release,
both menus, replicated mission/checkpoint entry, actual P1-owned Grizzly boss
lock, P1 fatal, survivor movement and original controller release. Replicated
commands4/12/2/5 have identical successful receipts, and each peer records
exactly one dead-owner handoff. Retries and boss completion flags remain
unchanged; the scene has a genuinely hidden living passenger before fatal
entry. Each peer completes1042 restoration checks with no mismatch, watchdog,
device reads, VRAM leak or host-state leak. The final composed image shows
X/P2. Native boss damage, individual resource preservation and preceding
door behavior are not qualified online by this HP-refill setup. Exact source
and invocation are retained. This is a same-build local pair; Internet,
cross-build/platform identity and mismatch rejection remain unverified.


The opposite E97 Split loopback roster also passes with30ms simulated latency
and10ms jitter (`qa-20261020-scene-netplay-x-p2-jitter`): X/P1 + Zero/P2, actual
P2-owned Grizzly boss lock, native P2 fatal, living X/P1 movement and original
controller release. Its275 matching core/mod samples span intro, each input,
charge/release, both menus, mission/checkpoint entry, held lock and recovery.
Commands4/12/2/6 receive identical successful receipts; each peer records
exactly one handoff. Both complete871 restoration checks with zero mismatch,
watchdog, device reads, VRAM leak or host-state leak. Retries/boss flags remain
unchanged, and the final image visibly shows X/P1. The same HP-refill, bypassed
door and same-build/local-only limits apply. The death rings and red backdrop
still require their separate lifecycle/reference checks.


## Actual Grizzly door qualification after scene recovery

On a fresh E97 Zero/P1 default-loading Split session, pre-enrollment native
confirmation completes the checkpoint3 setup. The retained live-primary and
coherent diagnostic arrival observations distinguish that introductory gate
from the earlier zero diagnostic snapshots. The first actual door attempt
(`qa-20261020-door-fresh-zero-default/p1-control`) reaches native collision
and D4=1, but its assertion combines a newer scene observation with an older
body sample and fails. The original controller then naturally completes the
door, checkpoint4 and both returns after that harness exits; this observation
does not pass the failed report.

The coherent driver passes P1-led control (`p1-coherent`), and the promoted
`tools/test_mmx5_coop_door_runtime.py` passes P2-led control (`p2-native`).
The latter asserts the original checkpoint3 initializer receipt, native
placement activation and collision, actual P2 scene ownership/D4, checkpoint
advancement3->4, both ordinary returns, identical original foot points,
passenger HP17/ammo73/inventory, unchanged retries/boss flags, native Right
movement from each hero and zero private-render restoration faults. Position
and protection fixtures bypass the preceding route; only the leader's native
approach activates the actual door. The waiting hero starts on the distant
checkpoint platform. Other doors, wrong-side/nested/nearby cases, Unified,
opposite roster and owner fatal during this door remain unverified.


The stronger promoted P1 normal door control also passes (`p1-native`), but
`p1-fatal-acquire` fails on the same E97 session. Fatal HP is submitted during
scene1/phase2, the passenger reaches hidden phase3 with HP17, native P1 fatal
entry occurs, and exactly one recovery selects P2. The living X/P2 then enters
native death too before checkpoint4 or return/control validation. Its body
retains D4=1, immunityA4=1, and crush contacts89=0D. Both actors are dying in the
retained coherent samples/final capture. This is an actual door failure, not
one of the earlier setup/assertion failures. The remaining sequence stops.

Original399D4 only initializes door action13/D4=FF when A4 is zero. General
recovery gives arrival immunity, then D4=1, so that native initialization is
blocked. Original54D24 already cleared the old owner's collision box when it
committed to the carry;54D9C moves the actor collisionlessly and restores its
box at completion. Restoring a normal standing box under the replacement
owner while leaving D4=1 exposes it to terrain/crush checks. This source
evidence explains the observed state; a bounded native carry-state recovery
fix and runtime recheck are next. Earlier script/boss cases remain qualified
only within their reported limits.


CandidateD5126187 (production42E604D9) builds and passes17 CTests in each
configuration. Its door-owner handoff invokes original399D4 before applying
arrival grace, with A4 clear for that native initialization, and preserves
the collisionless box when the old D4 is already negative. The original door
controller still owns movement, destination/checkpoint advancement and box
restoration; no success flags are supplied. Source and executable hashes are
retained in `qa-20261020-door-carry-zero-default/build-identity.json`. Runtime
rechecking starts with the retained P1-owner fatal failure, followed by healthy
controls and both owner-fatal timing points. Those checks are pending.


D5126187 still fails that owner-fatal case (`p1-fatal-acquire` and the focused
`p1-fatal-trace-nativecarry` replay). Native carry initialization now reaches
D4=FF and a zero box, but the survivor still dies. The existing targeted write
trace (no new runtime instrumentation) records the decisive ordering: at
frame9213, original2FE84 sets contacts70 to0D while recovery still has X's
standing box; only afterward is that box removed. At frame9214, native38A50
changes the survivor's HP17->80, then38C00 sets it to0. The full HP, mode, box,
contact/cache and door traces and source are retained. The unwrapped ring
reports2002 total targeted writes; each of those six field dumps contains all
its matching entries. Position/freeze ranges were recorded but not dumped.  The next revision removes the committed carry's box before
calling original2FB58, allowing that native collision pass to produce its
collisionless contacts; no fabricated terrain/contact bits are supplied.
The first candidate is retained as failed and does not qualify the door fix.


The contact-order revision is debug532A9498 and productionCEC9E5D9. Both build
and pass17 CTests (`build-tools/door-contacts-*-tests.log`). The owned default-
loading Zero/P1 Split sequence is running in
`qa-20261020-door-contacts-zero-default`, starting with the previously failing
P1-owner fatal and the same targeted trace. Executable/source identities are
retained separately. Runtime checks remain pending; the failed D512 carry-
initialization attempt is not replaced by a build-only pass.


The first532A9498 runtime regression passes (`p1-fatal-acquire`): native P1
fatal during the actual door, exactly one handoff, P1 fully fallen/X-P2 alive,
original door checkpoint3->4, HP17/ammo73/inventory/retries/boss flags retained,
native foot pointY96 (6291456 fixed) for both characters and survivor Right
movement/settle. The composed capture visibly shows X/P2 in both panes. The
targeted HP trace contains no native38A50 crush write, unlike the retained
D512 failure. This qualifies the specific collisionless native carry recovery;
healthy controls, opposite owner and the later hidden-passenger fatal timing
are still running before broader qualification. The earlier failures remain
retained. No bypassed door success, checkpoint or contact flags are supplied.


The complete532A9498 Zero/P1 + X/P2 default-loading Split door sequence passes
(`qa-20261020-door-contacts-zero-default/sequence.json`): healthy P1/P2 leaders,
either owner fatal after acquisition, and either owner fatal after the
passenger is natively hidden. All six cases assert native checkpoint3->4,
foot pointY96, retained passenger HP17/ammo73/inventory/retries/boss flags,
original controller completion and movement/settle from each living driver.
Fatal cases finish with the owner fallen and exactly one handoff; healthy
cases have zero. Restoration checks reach5546 with zero mismatch, watchdog,
device reads or VRAM leak. Both fatal-seat composed images were inspected: the
survivor is visible in both panes. The original camera-region route and HP/
contact protection remain explicit setup bypasses. Opposite roster, Unified,
nearby/wrong-side/nested cases, other doors and the both-player option remain
unverified. Coupled script/boss/glow regressions are running next.


On that same532A9498 default-loading Zero/P1 session, the coupled controlled
stand-lock control/held-owner-fatal and actual native Grizzly boss P1 control/
either-owner-fatal regressions also pass (`post-door-regressions.json` records
the sequence). Native resources/retries/boss flags, original foot alignment,
movement and exact0/1 handoffs remain correct. The hidden-primary charge check
is the last pending case before the opposite roster.

The QA boot driver now releases Cross once the original intro gate clears,
allowing an existing native jump to land before the idle observation. The
first532 boot completed with the older driver after repeated jump/air states;
its exact source/identity is retained. The new driver retains its own source
and hash per run. This is a QA input correction; X/P1 boot validation is next.


The final532 coupled glow case fails its120-update/15-second deadline before
P1 fatal is requested (`hidden-primary-glow`). It advances41 actual world and
native updates, from14463/7047 to14504/7088, with both heroes alive, scene0/0
and plugin failure0. The partial-charge capture shows both heroes and an
active X/P2 effect; the fully hidden survivor boundary is not reached. This
failed timer does not establish a glow-state or owner-fatal regression.

The same runtime logs repeated latest/additive capture-write failures for
`overlay_captures.json` and contributionA7554BFC6C5BBD7F. The observation after
the failure (`capture-write-failure-observation.json`) retains elapsed836.8s,
RSS1,092,935,680 bytes,422 handles,15 threads,10058 restoration checks with zero
mismatch/watchdog and average private pass87.744ms (guest3.398ms). Both capture
files exist as regular files when inspected later; that does not establish
their availability at the failed writes. The private bin started with fresh
capture history, default resident loading was enabled, and this was the only
owned game runtime. Its verified resident pack served102 of113 recorded
requests. Framework/UI heads remain6f0ccbc6/b964eabe.

P37 remains unresolved: this reproduces capture-write errors plus slow world
progress without a watchdog exit, and does not prove that either causes the
other. The runtime stopped normally after evidence collection. The original
15-second failure and exact harness are retained; a separate fresh-session
control is needed for the current candidate's hidden-primary glow case. The
opposite X/P1 default-loading door sequence is now running separately.


The opposite532A9498 X/P1 + Zero/P2 default-loading Split door sequence also
passes all six cases (`qa-20261020-door-contacts-x-default/sequence.json`),
including either owner fatal at either requested timing point. Resources,
retries/boss flags, checkpoint3->4, foot alignment and native controls retain
the same assertions; handoffs are exactly0/1. Restoration checks reach4418
with zero faults. The P2-fatal composed capture visibly contains X/P1 in both
panes. Resident observations retain a verified pack and59 served requests of65
events. The revised gate-only Cross boot driver passes with the native X
roster. This completes both rosters for the bounded local-Split Grizzly door
case; Unified/nearby/wrong-side/nested/other-door coverage remains open.

A separate fresh532 Zero/P1 session now checks native X/P2 charge and shot
release directly after the passed actual P1-fatal door, with no intervening
location or position reset. The visual driver links and hashes the prior
passed scene report and requires the same owned process/binary plus already
fallen P1/living X-P2. It verifies native glow identity/follow/drawing after P1
is hidden, native active-shot observation on release and retained inventory.
The existing private replay child-allocation counter omits the survivor-world
path, so the new case observes the coherent native98 active-shot field rather
than treating that counter as universal. The original checkpoint1 survivor
glow case then repeats as a fresh-session control for the retained extended-
session timer failure. These effect checks are pending.


The fresh532 post-door effect case passes (`qa-20261020-survivor-effects-zero/
post-door-survivor-charge`): without a location/position/fatal reset after the
linked passed P1-fatal door, X/P2 produces native full charge and118 further
glow draws after P1 is already fallen/hidden, preserves its original DAT
assembly/follow identity, releases the glow and exposes one active native
buster shot. HP17/ammo73/inventory remain unchanged and native/world clocks
agree. The composed image was inspected: only charged X/P2 appears, with the
small native glow visible in both panes after the death rings have expired.
This observes one active shot; it is not a universal allocation-count claim.
All twelve retained door-approach observations also have checkpoint3 before
collision; the helper now explicitly asserts and records that existing
precondition in addition to the native checkpoint4 outcome.

The subsequent checkpoint1 control on that fresh session reaches full charge,
P1 fallen/draw0 and46 further hidden-primary glow draws with correct original
asset/follow state, then times out obtaining `present_shot_seq`. No endpoint
capture, release or complete pass is claimed. The failure/cleanup debug
requests also time out. Latest-capture write failure, a full background
diagnostic queue and a20,701,256us starvation-watchdog abort are logged. The
process later exits. `postmortem` retains the114467-byte run report (atexit,
frame9470),22129-byte heartbeat and5985853-byte ring dump. The131509745-byte
freeze dump remains in the private bin with a separate SHA/path identity.
The watchdog check location is not by itself the stall cause. P37/capture I/O
remains open and is the next investigation; neither enlarging a timer nor
disabling capture is being treated as its fix.

## Default-on Alia stage-hint suppression (owner request2026-10-20)

Local issue `beads-eio.10.7`. New package `mmx5.enhancement.skip-alia`
exposes **Skip Alia Hints**, on by default for solo and local co-op. The
trusted online profile fixes it on for both peers and ignores offline
preferences; source and manifest join its build compatibility identity.
The setting is applied on a fresh boot.

Original EXE inspection identifies category3/type14's common80056E88 handler
in every stage BIN member0-29. Its initial80056BB4 tests a global hint
rectangle, records a per-character hint bit and acquires3D29C's stand-lock.
The mod only consumes unstarted type14 actors in the native96-slot kind3
pool (original allocator2D144,8009E7A0..8009F9A0), through original2D720 (the same finalizer its state2 uses). It does
not start a textbox, set the shown bit, inject confirmation input or touch
story/checkpoint state. Already-started messages retain native cleanup.
Bosses, capsules and rewards use other paths; individual preservation checks
still need behavioral evidence. No additional host simulation state exists.

`qa-20261020-alia-native-inventory.json` lists49 authored hint placements
across12 inventoried sections, with original EXE identity and rectangles.
This is content coverage, not49 mechanic passes. CandidateA3D46E0C and
production2D50A958 each pass17/17 CTests; regenerated native code contains
the configured56E88 entry hook. The initial original-message local-Split
control (`qa-20261020-alia-original`) fails its counterpart-arrival wall
deadline before the focused hint4 case. It also logs repeated capture-write
failures while hint suppression is disabled; no completed A/B pass or
causal link between capture failures and arrival timing is claimed.

The initial A3D46E0C default-on runtime case reaches hint4's road area but
fails the explicit retirement-counter assertion. The candidate incorrectly
guarded the separate stage-controller pool. Native2B4E4 kind3 dispatch and
2D144 allocator instructions identify the correct96-slot pool. That guard
is corrected before further behavioral qualification; the failed case and
its executable/source remain retained.

Corrected candidate44289580 (productionFCD10A2D) passes17/17 CTests in
each build. `qa-20261020-alia-default-nativepool/hint4` passes with default
resident loading and Zero/P1 local Split: a position-only approach crosses
the native hint4 rectangle after the original camera catches up; retired
hints increase2->3, the shown bit stays clear, no shared lock persists, both
heroes move natively, and private inventory/retries/boss flags are preserved.
The composed two-pane image was inspected.

`qa-20261020-alia-original-nativepool/hint4` passes the disabled-mod control
in local Unified on the same binary: the actual type14/id4 actor is observed
before crossing; its shown bit and original stand-lock occur, with zero
retirements. The retained screenshot catches the box opening rather than
readable text. This control uses Unified without expensive render verification;
it does not qualify the original-message path in Split. The first Split
control's arrival deadline failure remains retained.

The default-on coupled native Grizzly scene reaches a hidden passenger and
returns both living heroes with matching feet/resources and native script
release. Its final Right assertion fails after measuring from a snapshot
taken131 world updates earlier; the target was12 updates. The helper now
starts that wait after input submission. Complete boss/control qualification
remains pending a rerun. Additional capture-write failures are retained in
that run; P37 remains unresolved. Same-build online and solo checks follow.

The first44289580 online case (`qa-20261020-alia-netplay-zero`) fails
before gameplay/hint execution: both remain in mode0 when P2's debug status
responds emu-busy. Its link simulator overflows the256-entry queue19 times
on P1, explicitly invalidating the requested30ms/10ms timing conditions.
No online hint/scene pass is claimed from this attempt. A no-delay,
headless-GL loopback control follows with the same feature disagreement.

The no-delay headless online control also ends on a debug timeout before
recorded enrollment (`qa-20261020-alia-netplay-zero-control`). Its last
coherent observations have mode10, gate1, inactive P1 and world0 on both
peers; logged core/mod samples agree through tick4032. Those boot samples
do not qualify hint suppression. Both initial attempts explicitly override
the title's normal `bios_hle=true` with the helper's defaultfalse, whereas
the prior passing E97 scene pairs use true. A title-default control will
match that prior setup; no BIOS cause is established. The helper now
records BIOS/frontend/latency conditions directly in its report as well as
the retained peer configs and source.

Solo default-on fresh boot passes on44289580 with native X selection,
movement/attack input, no co-op activation and one retired native hint
(`qa-20261020-alia-solo/run`). Diagnostics remain default on. The owned
process exits through the normal helper cleanup. The next online control
uses title-default BIOS HLE, matching the prior qualified E97 pairs.

The title-default online case passes on44289580
(`qa-20261020-alia-netplay-zero-title-default`): Zero/P1 Split, BIOS HLE
true, no simulated latency/jitter, headless GL and render verification.
Each peer retires the original intro hint while its shown bit stays clear
and no lock remains, despite offline feature preferencesfalse/true.
All286 checkpointed core/mod samples agree through both heroes' movement
and attacks, both personal menus, native Grizzly P1 scene lock, P1 fatal,
one ownership handoff per peer, controller release and survivor movement.
Private view restoration checks1355/1357 have zero mismatch, watchdog,
device-read or VRAM-leak faults. This validates the new online hint policy
and the named scene sequence on this same build; it does not establish
Internet, cross-build, simulated-delay or actual-door qualification.
The two earlier forced-`bios_hle=false` failures remain retained; the passing
control does not establish their root cause.

Final local replay passes (`qa-20261020-alia-default-final/sequence.json`):
fresh Zero/P1 default-loading local Split boot, the native hint4 skip/control
case, then the actual Grizzly dialogue/passenger/controller return and
P1 movement check with the refreshed post-input baseline. Hint and boss
endpoints record940 and1672 private-view restoration checks respectively,
with zero mismatch/watchdog/device-read/VRAM-leak faults. Both heroes retain
their resources and original foot alignment in the named boss case.
The earlier stale-baseline failure remains in its original directory.
Repeated latest/additive capture-write errors still appear even in this
passing sequence (including contribution27CDFEB364750A5C); P37 remains open.
No watchdog or capture behavior was disabled.

This completes the requested default-on Alia hint mod's scoped solo/local/
same-build online qualification. It does not close the wider P22 dialogue,
capsule, reward or persistence audit, nor the overall X4 parity goal.

## Capture I/O investigation after the Alia checkpoint

After local a7c176c, the entire older C3 diagnostic log was reviewed:
38202 bytes, SHA7f21c097b1a501d81426676f736eac43e7b579c460d4cb2dec9a4e139d65222b,
182 probe rows, no abnormal operation failures or ERROR/watchdog lines,
including scene activity after its initial126-row summary.

An expanded copied-source capture/loader probe1FE53434 passes17/17 CTests
and the fresh-private default-loading Zero/P1 local Split boot, native hint4
and actual Grizzly dialogue/controller/control sequence
(`qa-20261020-overlay-stream-probe-zero`). Its104 timing records have no
abnormal operation failures or capture ERROR lines. Slow successful
replacement835ms, commit829ms and mutex773ms are retained; thread roles
were not established, so these are not a main-thread stall diagnosis.
This diagnostic control does not reproduce the normal build's write errors.
Exact copied sources, source identity, symbols, driver and postmortems are
retained. No watchdog/capture behavior was disabled and shared sources
remain unchanged.

The next probe logs failures only, suppressing successful timing output and
expected missing contribution/cache reads while retaining failed temp/write
paths. This reduces observer timing effects. A nonsuspending/noninvasive
CDB snapshot, with only local symbol lookup, is prepared to capture all
threads after an actual capture-write error; none triggered in the first
control. The normal44289580 executable is retained in build-tools and the
prior private QA bins; productionFCD10A2D is untouched. Root debug used the temporary copied-source CMake override during these
diagnostics; its normal configuration/executable is restored below. Dependency downloads are disabled for these configurations.

The quieter build's linker was observed live with measured I/O progress before completing. Its
owned-process CDB snapshot shows CRT fwrite -> WriteFile/ZwWriteFile;
`quiet-linker-threads.txt` and identity are retained. Observations increased
from48MB read/0 written to92MB read/23MB written while CPU time remained
below one second. This establishes an I/O wait in that build snapshot; it
does not establish a relationship to the game's capture failures/watchdog.
Framework AGENTS.md requires subsequent title builds at two compile jobs
and one game instance; diagnostics use one owned game at a time.

The quieter copied-source probe0A5C98B9 completes the same17/17 CTests.
Its first private replay (`qa-20261020-overlay-stream-quiet-zero`) fails startup as recorded below;
source/symbol/driver identities are retained separately from the timed probe.

A further read-only P16 reference review confirms X4's Both setting is more
than proximity gating (`mmx4_coop_lifecycle.c:663,1981,2001,2110`). Its
manifest applies to both boss doors and fortress refight teleporters, is
host-selected online, and excludes dead/departed partners. Admission delays
only the read-only contact query while the native door animates/renders.
After opening, both living heroes retain free movement and independent
views; their full collision boxes must physically clear the aperture before
committing the authored camera interval and allowing boss/story admission.
Only the open aperture's sealed wall samples become passable; native floor
and lethal classes remain. Abort/death/nested-script handling is tied to
the door actor, scene serial, stage and section. X5 currently has only
either-player projection/scene ownership; a nearby-only check would not
satisfy this X4 feature contract. No X5 Both implementation/pass is claimed.

The first quieter probe0A5C98B9 ends at the helper's90-second debug-startup
deadline before any gameplay/hint/capture-error evidence. The owned runtime
was still in early host initialization, with419MB read, about147 seconds
of age and0.20 seconds CPU in the retained live observation; its startup
log had not reached the debug listener. The helper then stopped its owned
process89108. This is a failed startup observation, not an I/O error
reproduction or a gameplay pass.

For the next diagnostic, the boot helper accepts an explicitly recorded
startup wait (default90 unchanged, diagnostic600). Native watchdog and
gameplay deadlines remain unchanged. Executable/source hashes are captured
before process launch and the same source bytes are retained, avoiding
extra executable reads while the game initializes and a changing source
hash after launch. The new private replay is
`qa-20261020-overlay-stream-quiet-slow-boot-zero`. A longer observer wait
is not being treated as a performance or capture/watchdog fix.

The longer-wait quiet replay reaches the debug endpoint after84.281s, then
fails during pre-selector boot: native starvation watchdog7,035,607us at
frame345, before any failed probe operation or capture-write ERROR is logged.
This does not rule out a pending I/O wait. The native report's stack words
include a resident adapter return after original152B8, but `crash_trace.c`
uses a bounded raw stack scan, not native unwinding; stale PGXP return85ACB
also appears. These candidate addresses do not establish a call chain or
root cause. Exact symbols, run report and heartbeat remain in
`qa-20261020-overlay-stream-quiet-slow-boot-zero/postmortem`.

An early native-thread diagnostic is prepared to trigger after two failed
readonly `frame` requests following the first successful response. Its small
minidump includes native thread contexts/stacks and region/thread metadata;
it may briefly suspend threads, so elapsed dump time and observer effects
are recorded. Watchdog deadlines and runtime behavior remain unchanged.
The copied-source probe stays immutable and separate from normal builds.

A further actual-placement inventory identifies35 category4/type8 candidate
records in16sections, including16 records across the eight Maverick stages
(`qa-content/door-candidates.json`, with source/BIN/inventory identities).
Only Grizzly1/0/subtype14 is presently confirmed and exercised as a door;
category/type matching alone does not qualify the other34 mechanics. P29
now describes all authored Maverick doors, without assuming one door per boss.

The temporary project include has now been removed by a fully disconnected
configure, without rebuilding or touching shared dependencies. With no root
runtime using it, normal debug4428958095ee0c6e98292d102742211a30c8f9f33961dbb3502af02a96457453
was restored from the verified backup. Shared `release_stage.py stage-execution`
then verified its embedded ENHANCED identity007e3b485c8e1b2f220caed7afbd468f61797d5503404d70bc867051e67aca03
and bound the exact executable SHA in its sidecar. ProductionFCD10A2D is
untouched. The early thread trace runs only the immutable quiet0A5 copy in
`qa-20261020-overlay-stall-threads-zero`, with its own staged execution sidecar.

## Revised comparative campaign, owner-approved October20

Coverage follows individual X4 failures and meaningful X5 analogues, with
broad original-content sweeps and focused regressions. No blanket product of
characters, seats, cameras, loaders and network conditions. Add a case only
for a distinct native mechanic/code path, ownership rule or observed failure.
The frozen Playtest42 comparison has234 independent report rows. Fifty-six
have explicit individual failure-class/trigger plans;178 still need that
planning review. Existing bounded passes remain bounded, and X4 checkmarks
never qualify X5. All30 later reports179-208 remain separately traceable.

The retained early thread dump is analyzed once, offline, using local symbols
only. It is a real native unwind (CDB), unlike the earlier raw stack scan.
Main thread is in CreateFileW through libstdc++ filebuf open -> diagnostics
begin -> inputs -> original8001255C; the startup-log reader is separately in
ReadFile. The two readonly frame probes timed out after frame334; dump time
was0.516s. This instrumented0A5 run subsequently completes native boot, so
this sample establishes a transient main-thread diagnostic-session file open,
not the cause of every watchdog or capture-write error. No new runtime variant
is justified by this evidence alone. Session setup still performs synchronous
filesystem operations despite background steady-state writing; that concrete
logger risk remains open. P37 stays unresolved.

The normal44289580/default-loading local Split X/P1 run is the next broad
baseline, without private render verification overhead. Dedicated incoming
contact checks must disable setup HP/contact protection before asserting
native damage. The record remains hidden/muted and all artifacts are local.

The normal44289580 broad baseline completes native X/P1 + Zero/P2 Split boot
(default loading/diagnostics/Alia, render verifier off), startup166.328s on
this host. P2's controller-only approach from168 to491.25 leaves P1 at144
and reaches active original intro enemies. This establishes encounter access
and interaction, not causality of every spawn trigger.

The incoming-contact driver passes three bounded windows with P2 HP/contact
protection disabled and P1 HP protection only. Original317B8 is traced and
accepted P2-contact counters advance once per named window:32->27 before
menus,23->18 after P2's menu and18->13 after P1's. Additional native damage
between windows is retained; these are not isolated identical saved fights.
P1 stays32HP. Each owner rejects other-seat Start, freezes the world counter,
and resumes natively. Composed contact images were inspected. No HP/damage,
enemy-state or success flag was injected for these contacts. Evidence is
`qa-20261020-broad-baseline-x/incoming`, with the exact driver source/hash.
Legacy X4 regression2 now has this bounded X5 pass; other damage classes,
projectile consumption, voice fidelity and broad terrain remain unverified.

The same-session Quit/re-entry follow-up reaches and visually inspects both
original P1 prompts (Continue game? then Quit and go back to the title screen?).
The initial driver then times out awaiting gameplay mode exit after Up/Cross.
Its input/confirmation readiness has not been qualified, so this is retained
as a failed setup, not a proven X5 menu defect or a re-entry pass. Evidence is
`qa-20261020-broad-baseline-x/menu-reentry` plus `quit-observation`.
X4 feedback208 remains unverified; no speculative menu patch is added.
The bounded investigation ends here pending a clearer original-menu trigger.

The individual planning pass now covers all234 frozen X4 rows: each has its
own failure-class/risk and bounded X5 trigger/check. This supersedes the
intermediate56-planned/178-pending planning count above. Cases that need a
native X5 counterpart located say so explicitly; no inapplicability or runtime
pass is manufactured. Historical evidence/build limits remain unchanged.

X4 feedback205's logger parity is now implemented pending native qualification:
`session.json` records the runtime game version, effective initial local/netplay
and camera mode, partner initialization/campaign, and committed co-op options.
`mode-changes.csv` uses the existing bounded background queue for runtime
transitions, with no new per-input filesystem calls or larger event records.
The strict N/C/D input-v2 stream stays separate. Focused recorder/writer checks
pass, including all five effective mode labels, stale saved camera preferences,
projected character identity, interleaved byte-identical input rows and an
unwritable mode-history destination. Debug full build/tests are next.
Synchronous session setup remains a distinct open I/O risk; it is not fixed by
these additive metadata fields.

The next broad sweep selects one original checkpoint entry from each of the
26 authored stage/section tables (`qa-content/section-entry-sweep.json`).
Its optional grounded-jump probe records accepted native input, actual rise,
gravity/landing and the other body's motion at suitable unlocked foot starts.
Unsupported/unsafe launch states remain unverified. Each section's six broader
mechanic categories remain independently unverified until explicitly exercised;
a successful load or short jump does not clear the route, enemies or exit.
No new native candidate/configuration is created for each location.

A bounded original-code review for the pending Both feature confirms X5's
common54BB8 substate table8007C34C:54C00 contact admission,54D24 waits for
native negative D4/action13,54758 door animation,54D9C automatic carry, and
54E84 closing/checkpoint cleanup. Native54D9C compares cameraX+0A with the
authored target+26 before restoring the foot box. Both will need physical
full-box clearance and an authored camera interval rather than that singleton
auto-walk/equality, with passenger suppression limited to the paired crossing.
This is implementation planning, not a Both behavior pass or proof that all35
category4/type8 placement candidates select this handler.

The normal debug executable builds successfully with the metadata change.
The initial full-suite run times out its writer test (23.42s reported against
the existing15s test limit) despite the earlier focused2/2 pass. The failure
is retained in CTest logs; no host-I/O or logic cause is assumed and no timeout
or watchdog is changed. Only that failed check will be rerun after the suite.

The writer's unchanged focused retry passes in6.55s. The first full-suite
16-pass/1-timeout report and the retry report are both retained under
`qa-20261020-mode-build-checks`; all17 checks now have passing results, with
the intermittent timeout explicitly unresolved. Normal candidate SHA is
952b028f80451d527d5b7b098a6072bb606ab3a62e63da833cce5a62d2560254.
Shared framework6f0ccbc6/UIb964eabe remain unchanged. The next single owned
Zero/P1/default-loading local Split session first verifies actual logger files,
then sweeps the representative original section entries on that same candidate.

The first normal952B028F native run fails boot before counterpart enrollment:
watchdog8,046,942us at frame2444, dispatch depth1, last store224D0 in the
fixed96-slot kind3 dispatcher. The last57E8C function is a finite initializer,
not evidence of a loop. This differs from the earlier resident callback exit;
raw stack-scan addresses remain candidates rather than a causal unwind.
`session.json` nevertheless verifies the actual version/local Split/options,
and mode history records X->Zero before enrollment. Full partner/mode and
section checks did not run; the failure is retained at
`qa-20261020-mode-and-sections-zero`.

One same-binary local storage control is staged on C for the executable and
generated diagnostics/QA/capture output, keeping the same settings, original
F disc/config/BIOS and existing F compiled cache via junctions. Historical
capture contributions are only partially copied, an explicit confounder;
this cannot prove a disk fault or count as a runtime fix. The optional bulk
history copy was stopped rather than extending that archival task. The exact
952B028F hash and paths are in `qa-20261020-storage-control/stage.json`.
Production compilation runs first, serialized with game launches.

Reconciliation of existing individual evidence now records17 bounded passes
and217 unverified rows. Legacy22 uses the actual532A9498 Grizzly passenger
return/foot/movement report; feedback122 uses all eight C66C2B20 P2-led native
refight activations. Legacy21 remains unverified: those pad samples clear the
scene/control locks but do not prove both grounded feet and later movement.
These are traceability corrections, not additional runtime passes. The saved
Quit timeout image was also inspected: it shows the first Continue prompt
again, supporting the retained failed setup rather than a speculative menu fix.

Production candidate7668995D builds with debug tools and qualification cheats
off. Its initial suite is16/17, with the writer timing out against the existing
15s limit; the one unchanged focused retry also times out (21.77s reported).
Both logs and build identity are retained in `qa-20261020-mode-build-checks`.
The newest initial-test folder contains the complete ordered input/mode rows
and recovery/invalid-destination files; the retry creates no newer test folder.
These observations do not identify whether process startup, path I/O or later
shutdown caused the deadlines. Production writer validation remains failed;
no limit is changed and no further blind retry is scheduled. The exact952B
C-output native check is now running separately, after tests have exited.

The C-output native952B session boots in108.765s and passes the effective
local Split logger receipt, including version/options and X->Zero/partner-ready
mode history. Its first-entry sweep loads14 tables, with24 native jump/rise/
landing passes across12 tables. Stage3 loses P2 before input, while the Chaser
entry is outside the on-foot probe. Stage11 aborts at frame20544 with watchdog
4,001,931us, last function2970/store67264. Original671B8 has a CD-DMA busy poll
after that store; raw symbol candidates are not an unwound causal stack. This
does not qualify a file-open cause or close the watchdog issue. Small reports,
the6MB ring and both existing freeze captures (one110MB) are preserved under
`qa-20261020-storage-control`; the original stage/root junctions are not copied.
The derived coverage disposition explicitly retains the observed stage3 death
and unattempted tables without rewriting the original frozen harness/results.

The unchanged production writer executable2D5E32D3 passes the single C-path
functional control in0.984s under the same15s bound, including complete mode
rows and destination recovery. Both F-path CTest timeouts remain failed; this
environment comparison proves neither a disk fault nor a game-watchdog fix.

One same-candidate stage2/section1 -> stage3 chain reproduces the water-entry
loss with existing debug write tracing, not a native instrumentation variant.
At frame8530, original32898 sets P2 contact history0F; frame8531's38B2C writes
HP20->A1, then38B90/38C00 commits fatal HP. The solid caller/shape is the native
water-stage actor atC6A10. The accepted-input timeline shows P1's scripted
walk from86 to721 while P2's incoming pose stays behind; its later position
569,411 is inside the hull. This identifies native crush and a stale-arrival
risk, not yet the complete root cause. No HP/death/contact-success flag was
injected. Initial failure, exact driver, trace and timeline remain at
`qa-entry-followup/water-chain`. The driver now records pre-jump fatal entries
as failed observations, and a living survivor can load the next table without
a fabricated wipe/frontend recovery. The bounded investigation ends pending
the next gameplay fix; the11 untouched tables continue on this same session.

That follow-up adds stage12 and two native jump/landing passes, bringing
unique entry coverage to15 loaded tables and26 passes across13 tables.
Stage13/section0 remains in native mode9 for the90s observation, with neither
hero initialized/enrolled, then misses the bounded native-menu recovery.
This is a retained failed setup, not proof of broken gameplay or an applicable
story branch. Its selector is original BIN member20 and it has no static
placement table; its native entry prerequisites still need qualification.
Nine table entries remain untouched. `combined-entry-disposition.json` retains
both failures and the exact outstanding IDs. Owned55488 was stopped before
the next build; its reports/trace/timeline are preserved locally.

The incoming-arrival correction is drafted, not yet validated: keep the same
native beam/resolving animation, refresh its target to the current grounded
foot leader, wait for a safe foot target before collision returns, and run the
native terrain query at that commit. Mounted/Chaser behavior retains its
existing separate path. No scene completion, HP or terrain-success flag is
set. Debug build is running before the original water chain and a mounted
entry regression. The boot driver also now rejects nonexistent input files;
earlier commands used a retired build-coop BIOS pathname and actually ran the
bundled OpenBIOS fallback as logged. Subsequent runs use the existing staged
`build-parity/bios/openbios.bin` explicitly.

The water-entry finding is also traced back individually to X4 legacy25's
unsafe incoming landing, now failed with the bounded evidence. Feedback6's
moving-owner arrival report retains that related finding without inheriting
a controller-only movement pass/failure. Current comparative counts are17
bounded passes,1 failed and216 unverified. The X5-specific row does not add
another frozen X4 report or inflate the234-row inventory.


On current debug0D15A16C, the identical stage2/section1 -> stage3 water chain
passes after the incoming-target correction. Both heroes enter at the leader's
current native feet, x721/y430 and431, ground8, HP32/32, then execute native
jumps/rises/landings. The retained2048-row filtered HP trace has no native
fatal write; it is not coverage of every damage event. P2's probe also moves
the other body, so it does not qualify globally stationary independent control.
Evidence: `qa-20261020-storage-control/qa-incoming-fixed-zero/water-chain`.

The same candidate passes native intro/ownership/Select and ordinary Ride
Armor regressions, then a distinct controller-only moving-owner return:
P1 walks219.5px while P2's native Select arrival goes JOINING -> ALIVE;
original3B33C trace contains exactly one incoming start. P2 retains HP17 and
all32 ammo bytes, with no checkpoint/enrollment/retry/campaign change.
Mounted-during-incoming, other geometry and network arrival remain unverified.
Debug build and all17 CTests pass in45.96s. Production validation is next.
The explicit BIOS path exists and matches the linked OpenBIOS hash; automated
output stays muted while SPU emulation runs.


The same0D15A16C session also passes a P2 X native Grizzly defeat: ordinary
buster shots reduce native HP54 ->0, the original death controller retires
its actor, and native mode12 reaches the mission report showing one Maverick
defeated. Only player positioning/contact protection were used; no enemy HP,
victory, reward or campaign bit was written. Future combat checks may use
one-hit enemy HP setup, explicitly authorized by the owner, while retaining
the actual native hit/death/reward/exit behavior.

P1 alone advances that mission report into the original Save browser and
completes a real save to the session's fresh private card. Saved slot0 has
Grizzly defeated mask1 and checksum1483 matching the native checksum formula;
card2 stays byte-identical. Native Continue then enters mode20's Dynamo story.
The45s read-only selector wait and60s bounded story-input check do not reach
the selector. The checkpoint fixture bypasses the initial chapter prerequisite
and leaves native campaign phase0, so this is not a failed co-op selector or
story progression claim. Actual selector portraits, subsequent roster, native
reload, both reward abilities and continuous paired victory animation remain
unverified. The sparse defeat report's original minimum-positive-HP field is1;
its samples explicitly contain HP0 and `defeat-qualification.json` derives the
correct minimum0. Reports, UI input record, before/after cards and images:
`qa-20261020-storage-control/qa-incoming-fixed-zero/grizzly-p2-defeat`.


A frozen-source review of legacy9 maps its actual deferred-emitter/counter,
charge glow and dash-trail failure to the already retained native P07 checks;
that individual row is now passed bounded. This is an evidence reconciliation,
not another runtime test. Legacy8 stays unverified because its broader menu
attack recovery still needs native ordinary projectile/saber damage checks.
The234-row comparative inventory now contains20 bounded passes and214
unverified rows. P37 remains failed at the workstream level.

The0D15 session exits normally through TCP quit at frame93694. Existing
freeze recording nevertheless captured spin_freeze at frame2949 and hard_freeze
at82801, both at BIOS1FC06924/last store62FF4, before later gameplay/frontend
progress. Scalar summaries and original captures are retained; their labels
are observations, not a causal stack or a watchdog fix. The172-file evidence
archive is verified byte-for-byte (394,044,953 bytes), including QA, current
diagnostics, both captures and the final run report. No stage-root junctions
were traversed. Production configuration completes offline with debug tools
and test cheats off; the build runs with two compile jobs and no game.


### Native slot roles before the remaining entry sweep

The original136C8 loader indexes archive members by `stage*2+section` for
frontends as well as gameplay. The resident20920 mode dispatcher indexes
8007108C by PLAY mode. Comparing those original constructors to the selector
proves the following five raw tables belong to frontend/interstitial modes;
on-foot stage checks are inapplicable there. Their menu, story and audio
behavior still needs its applicable workstream checks.

| Raw slot | Member | Original constructor / native mode | Disposition |
| --- | --- | --- | --- |
|13/0 |20 |20CB4 sets mode4 and slot13/0 | Mission selector; the retained mode9 teleport failure is invalid frontend setup. |
|13/1 |21 |20B80 sets mode6 and slot13/1 | Selection/interstitial mode6; exact frontend behavior remains unverified. |
|14/1 |23 |20AB0 initializes mode1 and slot14/1 | Initial frontend; retain boot/menu obligations. |
|15/0 |24 |20BF4 sets mode12 and slot15/0;210B8 calls its save entry in mode18 | Mission report/native save, separately exercised above. |
|15/1 |25 |2107C sets mode17 and slot15/1 | Frontend mode17; its exact story/ending role remains unverified. |

Source EXE/archive hashes match the original inventory. Constructor byte
ranges/hashes, selector values, raw IDs and reasons are recorded in
`qa-content/native-slot-roles.json`, with original disassembly/source retained
under `build-tools/native-stage-roles`. The raw113-record/26-table inventory
and earlier failed setup reports stay unchanged. Removing these five frontend
tables leaves108 records in21 candidate gameplay tables, an upper bound:
slot14/0 is still unqualified. Original1F108..1F15C selects14/0 and initializes
checkpoint/world data; its empty position record alone cannot establish its
purpose or inapplicability. The five untouched candidate entries are14/0,
16/0,17/0,18/0 and22/0, plus the retained stage11 watchdog failure. No new
runtime or mechanic pass follows from this read-only role audit.


ProductionA1F0FFC7 completes the incoming-arrival milestone with debug tools
and qualification cheats off; all17 CTests pass in60.42s, including the writer.
Debug0D15A16C passed17/17 in45.96s. Exact executables/execution manifests,
source hashes, configure/build/test logs and native evidence are retained.
Framework6f0ccbc6/UIb964eabe remain unchanged. The moving-return trace has all
five entries from sequence0, so the exactly-one-start result is not truncated;
the reusable driver now rejects an incomplete trace. No watchdog, timeout,
loading default or user setting is weakened. The next combined same-build
loopback check targets this candidate's return/charge/menu/Alia ownership and
actual network-mode diagnostic metadata.


The checkpoint/placement union also exposes one section the checkpoint-only
sweep cannot cover: stage11/section1 has46 authored placements at80073B5C
(member17) and no checkpoint table. Its original branch/continuation entry
must be found before loading it; a data-only overlay can still use resident
handlers and does not establish inapplicability. The27-table raw union minus
five qualified frontend tables leaves22 potential gameplay sections, including
the still-unqualified14/0 role. The108 records/21-table figure above describes
checkpoint tables only. Slot11/1 is explicitly queued alongside the five
untouched checkpoint entries and stage11/0's watchdog failure.

### Current combined loopback limit

The first same-build0D15A16C hidden/muted local Split pair times out its180s
bootstrap before enrollment (native mode10, gate1, both body updates inactive,
co-op ticks/world/enrolled0). These are the last published diagnostics; they
do not prove which initialization/arrival calls had already run. Both peers
record the correct forced netplay/separate mode despite offline co-op disabled
and local-camera Unified, the exact local version/execution identity, and the
X->Zero campaign transition. Online partner-ready and gameplay remain unverified.

A single bounded control gives selection, enrollment and arrival separate
allowances without changing the executable or game watchdog. It selects Zero
in14.657s, then fails a debug read before enrollment at the same native gate.
The control does not resolve the failure; its frozen harness/reports/captures
are preserved, and the temporary driver change is reverted. No third variant
or new native instrumentation is scheduled. The initial shared deadline is
therefore not claimed as the runtime cause. Matching prefix core/mod samples
do not qualify attacks, menus, Alia triggers, withdrawal/return or cross-build
compatibility. Both owned pairs are stopped. Exact evidence and comparison:
`qa-20261020-storage-control/incoming-netplay-qualification.json`.


The existing second-control hard-freeze capture at native frame3807 has an
actual24-frame main-thread unwind. Its top is NtReadFile -> ReadFile -> CRT
read; exact0D15A16C COFF/RVA symbolization continues through ISOReader::ReadRawSector,
read_effective_user_sector, psx_mod_read_disc_file, attacks_tick (inlined
second_tick/load_assets), mod dispatch and original22084. This is an observed
blocking asset-file read, not a raw stack scan. Original3C4BC, the last guest
store, merely returns an animation pointer; it does not establish a guest loop.
The capture's live PLAYER peek is initialized while the last published co-op
body snapshots are zero, so their stale state cannot locate the exact boundary.
`incoming-netplay-unwind.json` retains system symbols, exact binary identity,
COFF names/offsets, the initial incomplete addr2line results and source limits.

Source review identifies load_assets' full ROCK_X5.DAT read:47,935,488 bytes,
23,406 sectors, despite the active resident loader already retaining that
verified effective archive. A narrow reuse correction is now built for qualification:
borrow DAT only while its package/pack are active, copy every retained resource
as before, and free only the separately owned fallback buffer. Disabled and
fixed REFERENCE loaders retain the original effective-disc read; the reference
implementation supplies the same accessor returningNULL. No shared framework
source, loader policy, cache verification, option, game watchdog or timeout is
changed. Existing source-path counters will distinguish reuse from fallback.
Native/default and disabled/reference qualification are pending; this is not
a general watchdog fix or proof that earlier stalls have the same cause.


### Resident reuse candidate BC1F66D0 (2026-10-20)

Debug executable SHA256 `bc1f66d0bdc43635c6928dc1d12496fbdbc541ab84ffffeff6657170e63d908e`
builds and passes17/17 CTests in218.67s. The writer check retains its15s
limit and passes in14.03s. Execution identity is
`5ca1047fc612e59d20a4cd6407579ed100ddbd16226c604ce4df91c256f69aa0`;
shared framework/UI inputs remain unchanged. The fixed REFERENCE accessor
compiles as C99 with warnings-as-errors and exports unmangled
`mmx5_resident_dat`; this is interface coverage, not a full REFERENCE runtime pass.
Native default-loading/disabled-loading checks and production qualification
remain pending. Build/test logs, XML and source hashes are under
`build-tools/asset-resident-reuse-*`.


### Recovery checkpoint after system restart (2026-10-20)

The branch remains `feature/mmx5-x4-parity-local`, HEAD `ddf7284`. All four
resident-reuse runtime sources still match the saved BC1F66D0 build identity;
the executable SHA256 also matches. Its17/17 debug results remain valid.
The new runtime source changes and playtest staging helper are uncommitted.
The helper passes a syntax check; an actual new owner payload is not staged.
The last production binary remains A1F0FFC7 and does not contain resident reuse.

The first BC1F66D0 native run fails before enrollment: exit2,
`starvation_watchdog`, frame0, only original BIOS entry1FC00000 observed.
The existing freeze capture's actual main-thread unwind is in nvoglv64 DLL
initialization through ChoosePixelFormat, SDL_GL_LoadLibrary and
SDL_CreateWindow. This does not exercise load_assets or qualify the reuse fix.
Original watchdog behavior is unchanged. Evidence was copied and verified in
`qa-20261020-resident-reuse/boot-before-f-policy`, including the report,
heartbeat, freeze capture, copy receipt and symbolized unwind. Earlier C:
artifacts remain preserved, without cleanup or relocation.

After the owner's updated storage requirement, all new scratch/build/QA
outputs must remain on F:. Process-local TEMP/TMP, application data, XDG and
shader-cache overrides now point beneath
`F:/Temp/Codex/psxrecomp/x5-parity`. Python's resolved temp directory is verified
on F:. Windows' legacy known-folder API still returns the established C:
LocalAppData path; graphics-driver cache routing has not yet been qualified.
No further graphical test has been launched since that requirement.

Recovery checks find no surviving game, CTest, CMake, Ninja or linker process.
Windows records an unclean shutdown; its cause is not established by the game
capture. The next step is an F:-only runtime profile with verified output paths,
then default/fallback native qualification, combined local smoke, production
checks and the owner payload. The comparative ledger remains20 bounded passes
and214 unverified entries. Full parity remains open.


### Post-restart candidate qualification (2026-10-20)

Production15423EB5 builds with debug tools/test cheatsOFF and fully disconnected
configurationON. All17 production CTests pass in3.32s; its execution identity
matches BC1F66D0. `build-tools/asset-resident-reuse-production-identity.json`
records the executable/source hashes and flags. The shared crash_trace.c is
verified byte-for-byte (normalized line endings) against committed6f0ccbc6;
no watchdog change is introduced by this campaign.

The one F:-only software-headless control reaches native gameplay and records
one resident reuse, then fails co-op initialization on the unsupported software
backend's retained-texture path. It does not qualify enrollment or graphics.
Its frozen harness/counters/report remain under
`qa-20261020-resident-reuse/default-headless`; the temporary frontend option was
removed rather than creating more variants. The owned process is stopped.

After the owner permits the graphics driver's ordinary cache temporarily,
actual OpenGL/ZeroP1/local Split succeeds on BC1F66D0 with an F:-only profile.
Native enrollment/arrival finish, `assets-resident-reuse=1` and disc fallback=0.
The owned process has a2.156s debug-endpoint startup (not a full game-load
measurement). Native pickup/shared tank/menu ownership and script passenger
checks pass, as does moving-owner return with exactly one beam and preserved
HP/ammo. Actual Grizzly P2-led admission/return passes. P1 fatal after the
passenger becomes hidden also passes native owner handoff, checkpoint advance,
survivor control/resources and unchanged retry budget. Composed gameplay and
P2-owned menu captures are inspected. Evidence is under
`qa-20261020-resident-reuse/default-gl-zero`; setup cheats and bypassed routes
are named in each original helper report. These are bounded checks on one
current candidate, not a roster/loader/camera matrix or full campaign pass.

The driver's existing GLCache/DXCache still resolve to C:. The owner's temporary
exception covers short graphical checks; application builds, profiles, saves,
recordings, caches and logs remain on F:. No existing AppData data is removed.
Water-entry, disabled-resident fallback, same-candidate loopback and the actual
owner payload are still pending at this checkpoint.


Water-entry recheck and fallback source qualification also pass on BC1F66D0.
The identical s02-a1-c00 -> s03-a0-c00 loader chain has four passed native
jump/rise/landing probes (two seats at each point), with both actors grounded
and HP32 before each jump. Water's companion-body-stationary observations are
false, so input isolation is not inferred from those water probes. Broad
terrain, currents, routes and actual exits remain unverified. The helper's
stdout still labels broader mechanics unverified; the specific pass is in
`ground_jump_probes`, not the loading receipt. Default local session finishes
with enrolled1/failed0 and one asset reuse across the stage changes, then is
stopped normally.

A fresh Zero/local Split run with resident loading disabled also completes
native enrollment/arrival, independent movement and attack-input probes. It
records exactly one disc fallback and zero resident reuse, then quits with
exit0. This qualifies the separately owned buffer path without a new runtime
binary, extra loader option or timeout change. Evidence:
`qa-20261020-resident-reuse/fallback-gl-zero/report.json`.

After the owner adds F:/G: load balancing, both drives are verified ready:
F: has487GB free with current queue44/average42 and1.81MB/s observed;
G: has9.19TB free, queue0 and0B/s observed. New heavy netplay evidence is placed
under `G:/Temp/Codex/psxrecomp/x5-parity/netplay-assets-reuse-zero`.
The completed F: outputs stay in place. The same-candidate loopback is running;
its gameplay/CRC result is not yet claimed.


Local milestone `9781b59` commits the resident reuse/fallback correction and
bounded qualification above. Runtime sources still match the tested candidate;
there is no further game-code rebuild for staging-helper changes.

The first G: same-candidate loopback attempt is a failed setup, not a gameplay
regression: both private peer folders lack bios/openbios.bin, so the bundled
BIOS verifier exits before emulation (frame0, dispatch0, cpuNULL). The explicit
--bios input does not replace the build's adjacent bundled-image contract.
Both runtime.log files say Bundled BIOS Missing. No gameplay/CRC observation
is produced. Preserve `netplay-assets-reuse-zero` unchanged. The peer-staging
helper now copies the built bundled image/notice and rejects missing inputs;
the exact same candidate is rerun at
`G:/Temp/Codex/psxrecomp/x5-parity/netplay-assets-reuse-bundled-zero`.
No game code, native watchdog or timeout is changed for that rerun.


The bundled-BIOS correction allows the same BC1F66D0 candidate to complete the
combined Zero/Split loopback:159 unique matching core/mod samples through5088,
zero desync, both players' movement/attacks, Alia suppression despite conflicting
offline preferences, active charge glow/release, withdrawal/spectator view,
return/private resources and both owned menus. Both owned peers stop normally.
Both diagnostics sessions record version local-x5-parity, execution5ca1047f,
forced co-op/netplay_separate despite offline co-op disabled/local Unified, and
native X -> Zero -> partner-initialized1 history. Each has three mode rows and
accepted-input CSV data. `logger-and-crc-qualification.json` retains the checks.
This closes feedback205 as bounded passed:21 passed/213 unverified comparative
rows remain. No cross-build, broad network, actual-door-online, long campaign
stability, or full replay pass is inferred. The prior failed attempts remain.

The last retained matching CRC sample is5088; the final session check reports
progress through5114. The online asset-source counter was not retained, so
this successful loopback does not establish which archive-read path it used
or why every earlier online startup failed. The explicit resident/fallback
source qualification is the separate local evidence above.


### Interim local owner payload (2026-10-20)

`G:/Temp/Codex/psxrecomp/x5-parity/local-playtest-20261020`
contains production15423EB5 with debug tools/test cheatsOFF, native disc/exe
paths in the local F: checkout, private settings/cards and local Split enabled.
Run `Play X5 Co-op.ps1`; it uses a normal window (fullscreen0) and clears the
automated dummy audio driver. Both controller seats are declared; select a
distinct physical pad per seat in the launcher. Diagnostics and Alia hint
suppression remain defaultON. No synthetic progress card is included.

The common SDK selects287 native overlay DLLs for the exact game/build tag,
excluding foreign namespaces, generated C, producer locks and ABI sweep memos.
Executable/execution-sidecar/BIOS hashes, OpenBIOS notice, frontend fonts,
co-op settings and launcher PowerShell syntax are checked. The actual staged
catalog passes the SDK verifier with exactly9 packages, no mods/packages tree
and no CD Speed/Fast Loading packages. The stricter source plugin audit reports
only the two intentionally excluded dormant native registrations (psx.cd-speed
and psx.fast-loading); that raw exit1 is preserved rather than presented as a
green source audit. It does not expose either feature in the staged catalog.
`QUALIFICATION.json` and the two catalog logs retain the evidence.

Native gameplay qualification used debugBC1F66D0 on the same game sources;
production15423EB5 passes all17 tests and the actual package checks. The two
builds intentionally have different co-op compatibility identities; cross-build
netplay compatibility is not claimed. The owner has not launched this payload.
Both-player doors, snapshot/rewind parity, broad stages/bosses/rewards/endings,
remaining comparative entries and longer/network-specific regression remain
required full-goal work. No publication, push, PR, release or tracker sync occurs.


### Both-player implementation contract audit (2026-10-20)

The delivered38d2f29 payload remains frozen. No new Both behavior is claimed.
A separate G: baseline BC1F66D0/OpenGL/Zero/local Split boot passes audit setup
only and is stopped; it is not an additional door/content pass. New evidence
and original-code disassemblies are beneath
`G:/Temp/Codex/psxrecomp/x5-parity/both-door-source`.

The original SLUS executable is hashed in source-identity.json. Native549E8 is
a read-only eligibility query: it rejects native immunity/dash/attack locks,
requires the player approach from the left, and compares the actual full body
and door boxes. Native399D4 consumes positive D4 only, changes it toFF and puts
the opener in action0x13. Native54D24 waits for that negative handshake and
clears the opener's terrain box before opening. Native54D9C later auto-carries
and waits on exact camera-min equality before running its original closure.
Native3A670 restores ordinary action2 (3A2C8 clears attack state; it does not
by itself restore ordinary controls). These addresses have distinct jobs;
porting X4's address-level calls directly would be incorrect.

Reconciliation with retained actual Grizzly P2-door evidence shows actor
800C6B58 at9360,64, box8007C334, script pointer0. Opening changes authored
camera bounds from8704..9024 to9360..9664 and sets PLAY+1C. The original owner
has action0x13/D4FF; the existing Either path hides/returns the passenger.
`original-door-phases.json` derives these facts from the retained native report,
without another door run or any success-flag write. Both must suppress only
admission while waiting, preserve the real opening animation, restore ordinary
controls/boxes, defer the new minimum, require both full boxes to clear, and
use the authored camera interval before native closure. Pause, independent
camera ownership and no second passenger beam remain explicit obligations.
Aperture collision class/geometry still needs native qualification; X4's38
exception must not be blindly copied. Death/abort and mounted eligibility must
retain individual checks, rather than inheriting the Either result.

Framework6f0ccbc6 prepare_profile_settings already imports every host option
and fingerprints the selected option values before the session. Thus the new
Both option can use that existing contract; no speculative shared-SDK protocol
change is needed. New mutable door state must be included in the game digest
and respect private-render restoration. Comparative reports61,79,91,97,101,
114,116 and137 remain unverified until their bounded native analogues pass.
This audit resolves implementation choices; it does not close those reports.

### Both-player native implementation draft (2026-10-20, in progress)

The owner payload remains frozen. A separate G: candidate adds the experimental
`door_players=both` choice (default `either`), admission-only suppression, scoped
door ownership/camera bounds, physical-clearance gating and the original native
closing routine. New state is reset, hashed field by field and exposed to the
existing diagnostics. The selected option appears in session metadata. No shared
SDK changes or remote operations occur.

First draft849EAA02 builds with SDL3/ENHANCED/debug tools and passes17/17 CTests
in2.49s. Native QA uses OpenGL/Zero/local Split, resident loadingON, overlay
interpreter and private-render verification. The added hook list changes the
overlay cache tag to`cg18_0bea7895_gcae7f4844_f0`; old compiled overlays are not
reused as qualification. Evidence and exact source/binary hashes are beneath
`G:/Temp/Codex/psxrecomp/x5-parity/both-door-candidate-identity.json` and
`build-both-doors/qa-both` in that same parent directory.

| Bounded observation | Result / implication |
| --- | --- |
| P2 reaches the actual Grizzly door before P1 | Native admission waits; no premature checkpoint advancement. |
| Both reach the trigger | The native opening runs, both retain ordinary action2/foot boxes, no passenger transport starts. This pose alone does not prove input recovery. |
| P1 menu short pulse | First harness misses Start. A held-input control opens immediately; retained as a harness failure, then replaced with native acknowledgement. |
| Both owned menus during open wait | Correct menu owners and rejection of the other seat's held Start are observed. The subsequent P2 return exposes the projection defect below. |
| Reverse movement after both menus | Failed: actors stay still. Original352A8 folds D4 into D3, so negative D4 still locks ordinary controls. Scope that gate out of body updates while preserving it for world scripts. |
| Actor identity after P2 menu return | Failed: P1 Zero becomes X. Re-entering an already projected P2 context overwrites P1. Switch by actual projection and restore the incoming context. |
| Physical aperture, deep leader/late partner, native closure/checkpoint | Unverified; the preceding failures stopped the case. |

The two concrete fixes are rebuilding; they are not qualified yet. Original
failures remain in`grizzly-first` and`grizzly-poll-gated`. Private redraw checks
show no mismatch/leak on this draft; this does not excuse the canonical-body
defect. The initial setup helper's12s arrival deadline expires under render
verification; the same owned process subsequently reaches both ordinary live
actors. Preserve that failed setup report. Its verification-only arrival bound
is now60s; native watchdogs/timeouts are unchanged. Startup stack/symbol evidence
also records an initial file-hash read delay that clears before gameplay.

The comparative ledger remains21 bounded passes/213 unverified. Both-player
admission, movement, menus and camera reports will receive individual evidence
only after the corrected native case establishes the relevant behavior.
Mounted admission, death/abort, distinct doors, adjacent intros and netplay
remain explicit follow-up checks.

The corrected2C9BB90D candidate passes17/17 CTests in7.19s, with a binary/source
snapshot at`both-door-candidates/2c9bb90d2047` under the G: evidence root. Its
first boot watchdog-exits before frame0. The existing automatic dump places main
inside NVIDIA's ChoosePixelFormat initialization; the later postmortem records
first OpenBIOS dispatch/cycle4. The unchanged repeat passes audit setup. See
`build-both-doors/qa-both-control/boot/analysis.json`; this does not identify the
older stage11 DMA-poll cause or establish a general stability fix.

On that unchanged repeat, the Grizzly case preserves both character identities
through both owned menus. Reverse movement still fails for P2. The decisive
held-control probe shows P1 moving15.5px left while P2's update counter remains
216 across78 world ticks. The newly added Both Select guard incorrectly returns
from the entire second-player update. Remove that early-return condition; the
existing native D4 eligibility gate already prevents voluntary departure.

P1's forward probe then stalls at9333.5 against the fully opened doorway.
Read-only native map reconstruction from actual RAM (original28FF0/30BB4) shows
class38 at x9344/9360, y32/48/64/80; adjacent x9328/9376/9392 are clear. The whole
y96 row is class38 floor. Evidence is`qa-both-control/aperture-map.json`, with
layout/tile/property pointers, raw layout and individual tile/class observations;
`held-control-probe.json` retains body/counter observations. The next candidate
removes the P2 early return and applies only the observed38 class exception
inside the fully opened door rectangle, excluding its bottom edge. The native
query still runs to preserve its sample-coordinate side effects. Other classes,
nonplayer actors and closed/unrelated doors retain their original query.
These fixes are rebuilding together; physical crossing/closure remains unverified.

#### Bounded Grizzly foot crossing passed

CandidateD5088CD1329B14A03E3E816000D5431899A9F3985AEFBEB6C6A11FBC65DE8330
passes17/17 debug CTests in2.69s and the corrected native case at
`G:/Temp/Codex/psxrecomp/x5-parity/build-both-doors/qa-both-map2/grizzly-native-close`.
Its exact binary/source snapshot is`both-door-candidates/d5088cd1329b` under the
same G: parent evidence root. OpenGL/SDL3/Zero/local Split, resident loadingON,
overlay interpreter and private-render verification are explicit test conditions.
The owned hidden/muted session is stopped after evidence capture.

The P2 update and character-identity fixes hold through both menus. P2 reverses
from9333.5 to9308 while P1 stays still; P1 then walks to9633 with P2 outside.
Native door state remains1/3 and both saved minima stay8704. The independent
camera origins are9473/9148. P2 physically walks through to9389.5; only then does
the native door enter1/4 and complete its closing animation at2/0. Both camera
minima commit9360. Real foot anchors remain96 throughout, with HP17/23 and
ammo73/91 unchanged. The composed capture corroborates the independent views.
The existing native animation ring records13 ordinary changes (5/7/8/9), with
zero incoming1/outgoing3 transport calls. Private redraw evidence totals7216
verified passes, zero mismatch/device-read/leak/watchdog counters and no co-op
state leak. This is bounded gameplay evidence, not general stability acceptance.

The first aperture hook8802CC29 saw only floor queries. Existing function-entry
arguments show the pre-movement side/ceiling checks use30AE4, including
x9344/y58/77/95; full collision30BB4 samples floor96. The same scoped class38
guard now covers both original queries. The existing call ring identifies these
two paths. Retain the prior failed reports.

Two harness assumptions are corrected separately from game behavior: release
Right after admission before requiring native idle, and qualify actual closing
from the native actor state/animation. Checkpoint4 advances when the leader is
inside even while the physical door remains open and both camera minima are
held. Its precise ownership/timing remains unverified; closure does not prove
that checkpoint timing is correct. The recent2048-record checkpoint watch window
does not contain a changed checkpoint value and cannot identify its writer.
Do not erase that gap or change checkpoint semantics on this observation alone.
Same-area native continuation is used for the final setup, with no success-flag
write; the original checkpoint/position/HP/ammo prerequisites remain bypassed.

Comparative reports79,116 and137 now pass this bounded analogue. Reports61
(dead/absent fallback),114 (exact P1-left trigger on the current candidate),91,
97 and101 remain unverified with individual reasons. The comparative ledger is
24 passed/210 unverified; no full Both, progression or X4 parity claim is made.
The frozen content inventory also yields35 placements in constructor family8 /
kind4, including the observed Grizzly subtype14. Raw subtype values are an
inventory, not35 door passes or a demand for a character/seat/camera matrix.
See`both-door-source/content-door-inventory.json` for their original records.

## Door eligibility and ownership (2026-10-20)

Candidate81262804a4eaa51e5dde33d01135368c5c055a26b24420296b02f13d2546ac80
passes17/17 debug CTests in5.05s. Its binary/source snapshot is
`G:/Temp/Codex/psxrecomp/x5-parity/both-door-candidates/81262804a4ea`.
The following evidence lives under
`G:/Temp/Codex/psxrecomp/x5-parity/build-both-doors/qa-door-ownership`.
Runtime conditions remain SDL3/OpenGL, Zero/P1, local Split, resident loadingON,
overlay interpreter and private-render verification. Automation is hidden/muted;
the owned processes are stopped. The owner payload is unchanged.

| Bounded check | Result / evidence |
| --- | --- |
| Default Either, healthy P2 leader | Passed on D5088CD1; `either-p2/report.json`. Native opening/closing, passenger return, feet, private resources and control. |
| Exact P1-left reversal and Both owner death | Passed on D5088CD1; `both-owner-fatal/report.json`. P1 reverses from9408+ to9383.5, regains Right to9633, then dies natively. P2 stays9308 during ownership transfer, crosses and recovers its own menu/control without a retry or passenger beam. |
| Voluntarily withdrawn P2 before admission | Passed on81262804; `fixed-unavailable-admission-resources/report.json`. Native beam reachesY-69 and ABSENT; original single-living door completes with no paired traversal. P2's native return retains HP23/ammo91, original location/enrollment and grounded feet after four ordinary updates. |
| P1 already fallen before admission | Passed in the same report/candidate. Native DYING->FALLEN precedes admission; P2 completes the original door and recovers movement. P1 stays fallen, private inventory and the shared retry remain unchanged. |
| Both owner regression on81262804 | Opening, both menus, reversals, native owner death/handoff and closing pass in `fixed-owner-fatal/report.json`. That report retains a failed checkpoint4 assertion. The remaining floor/resource and survivor-menu/control checks pass from the same closed state in `fixed-owner-fatal/closed-state-continuation.json`; no reload or crossing restart. Native incoming/outgoing transport calls remain zero. |
| General stability | Failed/unresolved: D5088CD1 later idle-aborts after6.56s without heartbeat. `idle-watchdog` retains the existing dumps, heartbeat, postmortem and analysis. The hard-freeze dump truncates before main_stack; the postmortem collision path alone does not identify a cause. |

The new X5-specific withdrawal failure is separate from door admission. Existing
write history identifies native2927C clamping the outgoing body toY0 during the
extra split-camera pass (2A440->A6C0->29184). That prevents35EDC from leaving its
visibility region. Preserve only the LEAVING body's vertical position across
that native camera update; ordinary live-body bounds and camera updates remain.
The fix is always active for the affected path. No watchdog or instrumentation
changes are made. The original failing evidence is`both-unavailable`.

Retained failed harness reports`fixed-unavailable`,
`fixed-unavailable-native-location` and`fixed-unavailable-settled` expose
an assumed checkpoint4, an immediate ground-flag assertion, and resource changes
that already occurred before admission. Their observations/analysis are retained.
The final check records checkpoint timing separately, observes native ALIVE then
four ordinary physics updates, and compares the living driver's actual admitted
inventory through closing. The unavailable partner still compares against its
original nonfull fixture. Door checks do not qualify unrelated damage/pickups.
The Both harness now also records checkpoint state without treating closing as
proof of checkpoint advancement; the original failed report stays unchanged.

The comparative ledger is25 bounded passes/209 unverified. Report114 now has
its original P1-left analogue. Report61 remains unverified overall: eligibility
and the foot-door fallbacks pass, but actual boss admission must also defer
until physical clearance. Mounted/abort behavior, distinct doors/second beams,
precise checkpoint timing, broad stage/progression/save coverage and current
production/netplay qualification remain open. No full Both or X4 parity claim.

## Everyday interactions and new content entries (2026-10-20)

CandidateEBA84D1CAB8205A128506B54D26617646F867AC933DA3002542D06295EDC2808
passes17/17 debug CTests in8.61s. Snapshot, original native routines, source patch,
failed reports and qualified cases live under
`G:/Temp/Codex/psxrecomp/x5-parity/qa-everyday-20261020`.
Conditions remain Zero/P1, local Split/default Either, resident loadingON,
overlay interpreter, SDL3/OpenGL and private-render verification. Automated
runs are hidden/muted. Owned games are stopped; the prior owner payload is
unchanged. SDK6f0ccbc6 remains a read-only input with no tracked diff.

| Check | Result / evidence |
| --- | --- |
| Original X4#110 failure analogue | Reproduced81262804: P2 large-health collection enters native mode2 and copies P1 camera2432/96 over P2's4400/208 before HP increases. `pickups-native-separated/report.json` and the retained post-failure trace/capture. |
| Corrected large heal, opposite-seat small heal, instant ammo | All three pass onEBA84D1C in `fixed-native-resource-contract/report.json`. HP/auxiliary energy and native selected-weapon/parts refill rules apply only to the collector. No actor relocation, partner refill or camera collapse; each collector recovers native Right. |
| Displayed views during large heal | Before/healing composed images and `composed-review.json` confirm the separated corridor/sky-and-hull views and both stationary heroes. The case retains native freeze flags and real delayed healing; screenshot existence alone is not the pass. |
| Native entry16/0 | Both jump/rise/landing checks pass; rise31.01/35.27px. |
| Native entry17/0 | Both pass; rise35.27/35.27px. |
| Native entry18/0 | Both pass; rise21.70/31.01px. |
| Native entry22/0 | Both pass; rise31.01/31.01px. |

All four entries use checkpoint0 from the original content inventory; their
receipts prove the native initializer consumed that record. Each other hero
stays stationary during the tested rise, and actual grounded landing is
observed. Per-location RAM, input, captures and dispositions are in
`remaining-entry-probes`. HP protection bypasses damage; no victory, exit,
boss or progression success flag is set. These eight probes qualify their
launch points, not complete terrain/routes, campaign reachability, alternate
branches, enemies, audio, retry, bosses or actual exits. The original108-record /
21-candidate-table inventory remains;19 candidate entry tables now have
historical load evidence across named builds. The placement union still adds
11/1, whose native entry path is unqualified.14/0's role and11/0's watchdog
remain explicit entry gaps, alongside the wider113-record/route obligations.

The fix distinguishes original545DC's item freeze (PLAY10..17 and19) from a
shared camera scene. Existing delayed-pickup ownership records identify the
heal. Preserve the other saved camera and pause its extra update while that
native heal is active; retain the genuine PLAY1C/scene-owner/vehicle gates.
No new persistent state, option, callback, instrumentation or watchdog change.
The owned session totals8238 verified private redraw passes at qualification
end, with zero mismatch, device-read, VRAM-leak or local co-op state-leak
counters. General watchdog stability remains separate.

Native53E3C also replenishes the collector's auxiliary energy atA8 by six times
the health amount, capped by148. Weapon bars begin atAA. The first corrected
camera run therefore retains its failed broad `health changed ammo` assertion
in`fixed-pickups`; its observations show correct cameras/HP and the native
energy increment. The final harness checks that original contract rather than
changing game behavior. The earlier pause-fade and distant-camera setup
bounds are retained in`pickups-g-disc` / `pickups-g-disc-live`; the already
prepared scene was reused after22 steady observed updates. No new game-setting
or build matrix was introduced. Actual unlocked weapons/menu icons are still
unverified; seeded ammo slots do not qualify acquisition.

A fresh default-Either81262804 intro also watchdog-aborted before any pickup
case could attach (7.156s without heartbeat). Its complete automatic hard-freeze
capture at frame3583 has main in WindowsReadFile through ISOReader raw-sector
read/CDROM advancement. The final postmortem is later at3701; do not attribute
every older watchdog to this one captured I/O block. Existing dumps, symbols
and analysis are in`idle-watchdog`. A582,954,960-byte original disc copy onG:
is byte-verified againstF: (SHAbe731bc4b9d3211b9267a34b8a68c769199a15479b14004ff25b67cdfebe8af4).
The unchanged81262804 G-disc control reaches the pickup reproduction; the
correctedEBA84D1C also finishes the three cases and four entries. No general
stability fix is claimed; no native watchdog timeout, loading, rendering or driver setting is
weakened. Further bulk data stays offC:.

Report110 now has a bounded passed comparison. Report12 retains individual
partial dash-ownership/expiry evidence; exact native palette/CLUT comparison
is still missing. The ledger is26 passed/208 unverified. Broader interactions,
all content routes/exits, actual boss/reward/story/save checks, snapshot parity
and combined current-candidate production/netplay remain required.

## Native personal menus and Quit/re-entry (2026-10-20)

Candidate82084041ADD7D84D6E76AFFC5B6B685ACB934E6DA330418678450F51B5CC90D3
passes17 debug CTests and the same bounded native input sequence that exposes
two distinct ownership failures. Evidence root:
`G:/Temp/Codex/psxrecomp/x5-parity/qa-menus-20261020`.
The game code adds only guest-menu Select suppression and a stage-init reset
of the retired stage callback guard. No option, persistent state, diagnostic
variant, watchdog change or loading/rendering profile change was introduced.

| Native check | Evidence / result |
| --- | --- |
| P2 campaign Quit reproduction | EBA84D1C `guest-quit-repro/report.json`: P2 alone opens original Continue and Quit prompts, then actually resets to title. Both composed prompts inspected. |
| Guest authority and ordinary personal menus |82084041 `reentry-fixed-native-menus/report.json`: P1 Zero and P2 X receive their own art; only the owner resumes. P2 Select stays in its menu. Initial HP13/11 and energy73/91 stay private through either menu, and each seat recovers native Right. |
| Actual P1 Quit | The corrected build still accepts P1 Select, Continue No, then Quit Yes after the original fade gate. Both native confirmation pages suppress gameplay bodies/life HUD in composed output, and gameplay really exits. |
| Retired callback reproduction |75FF5C29 `fixed-native-menus/report.json`: after original Quit/new-game, native world/timer advance864 times but P2 held Start does not open its menu. Stage-dispatch counter stops before Quit. `reentry-retired-guard.json` reads the owned process without writes: stage_call1, busy0, projected0, pause_owner-1. Exact original binary/symbols are retained. This matches frozen X4 feedback208. |
| Corrected native re-entry |82084041 repeats the same flow: X/P1 and Zero/P2 initialize, P2 opens Zero's menu, P1 cannot resume it, guest Quit remains blocked, and P2 resumes/moves+5.5px while P1 stays X144. Native stage hooks continue after re-entry. |
| Private redraw |958 verified passes at qualification end; zero mismatch, watchdog, device-read, VRAM-leak or co-op host-state-leak counters. This is bounded session evidence, not general stability acceptance. |

`tools/test_mmx5_coop_menus_runtime.py` retains its exact source/hash with each
report. Setup uses the native Grizzly checkpoint1 loader, initial HP protection
and contact protection; HP refill is disabled before seeding/qualifying the
nonfull initial inventory. Native Quit, selector, new-game intro and menu/control
checks write no menu or campaign-success flags. The new intro is protected;
its damage is unqualified. All owned sessions stop normally.

The first guest-only fix is retained as a failed re-entry candidate. A read-only
host sample identifies one concrete stale guard; the second candidate clears
that guard at original3BAC8 initialization after the existing busy/projected
rejection, just as the frozen X4 lifecycle fix clears its retired menu guard.
The native confirmation driver now waits on actual fade/cursor/page state;
the older ambiguous Continue-prompt timeout remains historical evidence.

Legacy7/15 and feedback208 now have individual bounded passed comparisons.
Feedback179 retains partial own-menu evidence; actual acquired special selection
and attack remain unverified, as do feedback31/150's weapon/icon interactions.
The ledger is29 passed/205 unverified. Production/netplay, current owner-payload
refresh, full content/progression, Continue/save/restore and general watchdog
stability remain open. Work stays solo/local, with no remote tracker sync.

## Native boss rewards, save and return (2026-10-20)

Same game binary82084041 (SHA256
`82084041add7d84d6e76affc5b6b685acb934e6da330418678450f51b5cc90d3`),
source-equivalent to local game commit `e4c4f4b`. No game C change, new build,
watchdog variant or CTest rerun was needed for this evidence milestone.
All automated sessions are owned, offline, hidden and muted; audio emulation
continues. Bulk evidence root:
`G:/Temp/Codex/psxrecomp/x5-parity/qa-boss-weapons-20261020`.

| Bounded check | Result and retained evidence |
| --- | --- |
| Intro setup | Unverified. Fresh native checkpoint3 and a checkpoint2/top approach did not establish the boss scene; direct entry produced pit/crush death or an uninitialized camera/admission. `intro-native-entry`, `intro-checkpoint2`, `intro-top-approach`, `intro-cp3-native-right`. Timeboxed without a code patch; no intro pass or proven boss defect inferred from these setup failures. |
| Explicit prerequisites | `post-intro-prerequisite/report.json`: post-intro PLAY79=1 and16 old story queue bytes at194..1A3=FF bypass the intro/preexisting story prerequisite. Grizzly defeated mask remains0. Native checkpoint4/position and contact/positive1-HP setup bypass route, door, damage balance and full fight. No tested defeat/reward/save-success flag is seeded. |
| Real P2 final hit | `grizzly-native-scene/report.json` enters original Grizzly control release; `grizzly-p2-native-clear/report.json` records native P2 normal buster1->0, original death, boss retirement and native cleanup11->mission report12. The first driver stopped incorrectly at11; its failed receipt is retained. `mechanics-qualification.json` and `post-boundary-observation.json` prove the same encounter's natural completion. The tracked harness now waits through11; that correction has not been rerun on a second fight. |
| Paired victory | FAILED: surviving P1 becomes hidden draw0/state3/action0 passenger at world10824.35 original samples show P2 action25 victory alone at worlds11367-11727. Individual feedback90 and128 fail; no patch or animation shortcut applied. |
| Native P1 reward/save/Continue | `native-reward-navigation/report.json` and `native-ui` capture controller-only original reward/save pages and actual Save completed. `native-save-receipt.json`: card1 occupied block1, phase1, defeated mask1, stored/calculated checksum5111; card2 stays byte-identical blank. Before/after real card copies retained. Actual native reload is unverified. |
| Native story and next selection | Native ready-page P1 Cross completes Dynamo/Signas dialogue. `native-ui/dynamo-native-page-ack.json` and `selector-briefing-continuation.json` succeed; earlier too-early/count-limited input receipts remain failed QA attempts. `native-revisit-receipt.json` proves real selector -> Zero confirmation -> native stage1 with Zero/P1-X/P2 and clear mask1. No story phase/selector/success writes occur in that continuation. Portrait-original comparison remains open. |
| Owned earned weapon menus | `native-owned-weapon-menu/report.json` inspects Zero C-SWORD and X/P2 C-SHOT in Zero campaign. Original DAT record88 native X menu assets match72704 VRAM words exactly (`original-menu-vram-comparison.json`); actual cyan icon inspected. Original disc unchanged. |
| Cycle/body/HUD recovery | `native-earned-weapon-confirmed/report.json`: native R1 C-Shot1->buster0, both bodies/meters retained, X returns blue; subsequent Square admits native shot counter0->1. Feedback31 bounded pass. Selected special Triangle spends only P2 energy282->276; P1 unchanged. A prior Square-special driver assertion is retained as an incorrect-binding QA failure. |
| Special projectile fidelity | Unverified. Native Triangle spends C-Shot ammo, but sampled composed images do not decisively establish its projectile appearance/travel/expiry/contact. `native-cshot-present-sequence/report.json` is an admission/capture receipt only. Feedback150/179 and delayed-special analogues remain open; do not infer an attack pass from resource spend or menu VRAM. |

`Runtime.confirm_native_text` now waits for the original ready-page marker at
80091C38+C/D=FF/03 and a fresh P1 Cross edge before releasing after native header
advance. Live original story/selector progression validates the helper; it
writes controller input only. The failed caller that counted32 polls rather
than pages remains preserved. Original PAD is800C931C, not text bytes atD931C.

The ledger is **31 passed /2 failed /201 unverified**,203 open. Feedback31/69
close bounded checks; feedback90/128 expose one concrete scene-ownership
failure with two original symptoms. This does not qualify native reload,
snapshots, every special, other bosses/routes/endings, netplay on the current
combined candidate or general watchdog stability. The owner payload is unchanged.
Main owns runtime/fixes and ledger. One explicitly owner-approved subagent
completed the [bounded remaining-QoL source audit](COOP_X5_NEXT_CHECKS.md): seven
reusable checks drawn from existing rows, no additional ledger tasks or passes.
It identifies absent elective private-tank, character-enhancement and scaling
options, plus concrete native Chaser, exhaust, death/audio and capsule checks.
Source risks remain unverified until native checks establish behavior.
No remote operations or tracker sync.

## Paired native victories and bounded Chaser check (2026-10-20)

Candidate4D0376A8 (SHA256
`4d0376a81a96edae5deb59ed466e7b78023f580c737e810becd696975c1c8d4e`)
keeps both living heroes through an original Maverick victory. Native stand14
is classified only for the active dying zero-HP boss matching SCRATCH4C and
PLAY20 on Maverick stages1-8. The companion receives that native command once
and advances its own original35040 body/animation; shared PLAY/camera state is
restored. Native3B1A8 cleanup/action25 is scoped to that companion, leaving the
world owner's original shared music call. No victory/clear/reward flag is set
by the mod or the test. State resets and deterministic hashing include the new
paired-scene state; this does not qualify current netplay from a build hash.

Both companion projection paths are checked once because they execute different
code. This adds no Cartesian configuration matrix. Root evidence:
`G:/Temp/Codex/psxrecomp/x5-parity/qa-victory-20261020`.

| Check | Result / evidence |
| --- | --- |
| Build and native hook coverage | New3B1A8 boundary declared in game.toml and original generated code regenerated, not hand-patched. Initial16/17 failure caught the omitted declaration; original failure retained. Final debug17/17 pass in2.35s (`ctest-debug-draw-fixed.log`). |
| P2 X owns the victory | `draw-fixed-native-introduction` passes original encounter/passenger return/control. `draw-fixed-native-clear` actual normal buster1->0, scene2/phase0, both native start/held poses visible with correct Zero/P1-X/P2 art, both original departures, native mode12 initialization and clear mask1. One paired scene/one companion pose; no hidden hero while other celebrates.1032 verified redraws, zero mismatch/watchdog/device-read/VRAM-leak counts. |
| P1 X owns the victory | Same candidate `x-owner-native-introduction` and `x-owner-native-clear`: X/P1-Zero/P2, native final hit, scene1/phase0, both correct composed start/held poses, individual departures and real clear mask1. One paired scene/one companion pose;858 verified redraws and zero faults. |
| First candidate retained |1299CB0F ran both native action25 states but omitted P1's retained sprite emission during a P2-owned world. Its composed image caught the missing body;4D0376A8 includes paired victory in that existing renderer path. The first driver also stopped at mode12/phase0 before native reward initialization; same-fight settled observation proves mask1, and the corrected driver now waits for actual initialization. Neither failed receipt is overwritten. |
| Chaser entrance | `chaser-native-entry-dash/report.json`: actual stage5/checkpoint0, both native mode0 -> grounded mode1 and mounted/facing identities. Both cars share one observed original type1B control actor8009EB90. READY appears in composed output, but its complete once-only lifetime/retirement remains unverified. |
| Chaser acceleration | P1 neutral/no boost/vx0; native P2 Circle+Right enters boost1/mode5 with vx0->3.875px/update. P1 worlddx132 equals camera carry132; P2dx191.5 includes59.5 extra own motion.218 verified redraws, zero faults. READY obscures the rider/exhaust capture, so animation/layering and related full comparative rows remain open. |

`tools/test_mmx5_coop_grizzly_clear_runtime.py` now supports the native X finisher
in either seat, actual paired-pose captures/assertions and native report-init
waiting. The subagent-owned `test_mmx5_coop_chaser_entry_dash_runtime.py` retains
exact harness/helper identities and bounded input/phase/car/camera observations;
main alone ran it. Original Chaser fatal/retry driver is unchanged. All owned
sessions stop normally; automated output stays muted with emulation preserved.

Setup bypasses remain explicit: intro phase/old empty story queue, original
checkpoint selection, approach positions, positive1-HP boss and contact
protection. Actual shots, death, victory and clear/reward transition are native.
The existing real-card save/Continue evidence is retained; native reload remains
open. No current production/netplay qualification or owner-payload refresh is
inferred from this debug milestone.

A separate unverified visual anomaly is retained: red/noisy horizontal backdrop
bands in the Grizzly room are also present in the prior82084041 native encounter
and death captures (`qa-boss-weapons-20261020/grizzly-native-scene/after.png`,
`grizzly-p2-native-clear/native-death.png`). Their relation to direct-checkpoint
setup, native palette or authored-map rendering has not been established.
The correct victory bodies do not qualify this backdrop; compare an appropriate
original/native baseline before claiming a cause or a renderer pass.

Individual feedback90/128 are now bounded passed with original failed evidence
preserved. Ledger: **33 passed /0 failed /201 unverified**,201 open. Chaser
mechanical observations supply partial evidence to existing rows without
promoting obscured presentation or network checks. Work and tracker updates
remain local; no publishing, pushes, PRs, releases or remote sync.


## Native card Load and ordinary dash exhaust (2026-10-20)

Candidate856484F5 SHA256
`856484f58d0858b1979c90f4fc9516d473fda46d7a093f2168de8e9fec8581a5`
extends the existing allocating-seat projection only to native type1 ordinary
dash exhaust at8004F0B4. Original3D200 allocates it through2D0E4 without an
origin; initialization and later4F654 updates read PLAYER, original character
assembly, facing and dash-animation eligibility. Existing retained trails do
not cover this path. The existing owner array is cleared by every allocation,
including P1 reuse. Type2 buster behavior stays in the same scoped wrapper;
unrelated shared effects retain their original paths.

Root evidence:
`G:/Temp/Codex/psxrecomp/x5-parity/qa-save-effects-20261020`.

| Check | Result and limit |
| --- | --- |
| Original failure | `dash-native-floor-before`: both heroes native grounded/alive at X/P1(160,379), Zero/P2(80,378); native P2 Circle+Right creates type1 slot1 at(128,391), using idle X/P1 art/offset. It then follows P1's animation lifetime. No effect flags injected. |
| Fix qualification | `dash-fixed-native-exhaust`: actual native dash in each seat, original assembly identity, native CLUT7802/facing/character offsets, natural expiry and same slot1 reused from P2 to P1. Sequential pool reads are bracketed by coherent body/world observations, with one native step of publication tolerance. Composed captures show correct ordinary exhaust and both bodies. World/native clocks agree. |
| Setup limit | Native GrizzlyCP0 and HP/contact protection plus position/history only. The first CP1 teleport to2640 killed P2 on authored terrain; `dash-before` is invalid setup, not an exhaust test or a diagnosed collision bug. No post-door/vehicle/wall-jump dust inference. |
| Buster regression | `dash-fixed-buster-smoke`: one original full-charge type2 release allocates exactly one original4F768 child, leaves the other seat's shot counter unchanged and does not fail co-op. No menu/repetition matrix. |
| Build | New4F0B4 declared in game.toml; original recompiler regeneration changes two C shards, never hand-patched. Debug17/17 CTests in3.53s (`ctest-dash.log`). Production/netplay unqualified. |
| Actual native card Load | `dash-fixed-native-reload` uses only P1 input and the real earlier native save SHA977FA138 (full hash in receipt). Original card recognition/decoding changes mask0->1, native Dynamo/Signas story finishes, original selector and character confirmation enter Grizzly with X/P1-Zero/P2. Native X R1 cycles0->1 CShot and P2 owns its real menu. Private card hash remains identical. |
| Native hero contract | Original disk-buffer1C904 restores slot5F to PLAY1BA;1CBDC..1CBF4 maps this saved nonzero value to X0. Separate RAM/current-data1CC34 uses slot58 availability. Native loaded-game confirmation is04/04/05;04/04/02 is a fade. Roster5 selects Zero; other original roster entries select X. Do not transplant X4's campaign identity rule. |
| Caller errors retained | Earlier reload receipts expected Zero at the loaded selector, confused disk/current-data rules or waited for new-game01/03. Their unmodified reports retain those QA failures. Same-session `default-menu-qualified` proves old4D0376A8 native default-X load, earned weapon, P2 menu/resume/jump/landing. Final fresh856484F5 driver completes the corrected native default path. Explicit Zero choice in the new disk-load driver remains unexercised. |

The copied card was earned by the earlier actual P2 Grizzly final hit, P1
reward/save UI and inspected native checksum5111. That original fight explicitly
bypassed post-intro phase/empty old-story queue prerequisites; tested clear,
reward and save flags were never seeded. Cold disk Load adds real persistence
evidence to feedback69. It does not implement host-state serialization,
mapped save/load or rewind (P35/feedback33 remain open).

Original source receipts from the approved subagent remain in
`G:/Temp/Codex/psxrecomp/x5-parity/subagent-x4-qol`.
The authored regular Grizzly capsule is placement35, checkpoint3, subtype3.
Original55798 permits either regular hero; the native grant sets shared
PLAY+A1 bit08, saved in slot59. This is an X5 armor-program part rather than
an immediate X4-style equipped ability. Runtime dialogue/grant/control/art/
persistence remain unverified; do not add a blanket X-only capsule filter.
The original cleared-portrait rule selects alternate authored tiles (BIN20
EF440..EF480/F0B78..F0BFC), not a direct portrait CLUT switch. Feedback17 stays
open pending actual rendered-tile/art comparison.

Feedback19 closes with this bounded native evidence. Ledger34 passed/0failed/
200unverified remains an inventory of the234 frozen reports, with no new
variant rows. All changes and issue updates remain local.


The same856484F5 runner adds a bounded native hurt/death observation
(`native-hurt-death-approach/report.json`, `qualification.json`).
One actual P2 enemy contact reduces Zero HP32->27; P1 remains at32 under its
explicit HP-only protection. Native private hurt record0 matches original
group3/bank15 pitch0400, address0 and ADSR80FF/1FC0. Its voice44 begins at
frame28859, native X/P1 jump voice22 starts28868, and P2 ends naturally28879:
11 observed overlapping frames, with no private voice steal. P1 also receives
a native hurt reaction; that extra sound is retained in the event interpretation,
not described as a pure no-contact jump control. P1 jump slot/pitch/ADSR match
its original record, but its resident hardware sample-base address was not
separately qualified. Muted emulated PCM is69090 stereo44.1kHz frames/nonzero;
there is no ear-review claim.

After recovering from that real hit, a fatal-HP setup enters P2's original
death path. Private stereo death record0C matches the original Zero tones on
voices44/45, pitches0390/03C7, address07640, ADSR80FF/1FC0. Death particles use
the original Zero member art and common native CLUT7802.324 sampled opposite
radial pairs retain the dying actor's exact center(400,1130), while P1 moves
elsewhere. Native emitter attempts are44 random+32 radial. The152 raw trace
entries are identical adjacent wrapper/guarded-original pairs: existing
`effect_allocate` calls its guarded guest once, so they represent76 native
attempts, not a second burst. Maximum sampled active54; randoms retire,32
radials remain active with native visibility culling at the final sample.
The original reviewed radial path does not promise fixed-age active retirement.
P2 reaches lifecycle5 once and living P1 moves natively.

This adds partial evidence to existing death rows and hurt evidence to already
passed audio rows, without closing composed death-color/pose, boss-approach
fatal, all-character/vehicle deaths or general slowdown/stability. The first
`native-hurt-death` approach observed no contact before its12s bound and is
retained; position/history-only approach setup in the same intro then reaches
one actual native contact. No contact result, sound event, particle, emitter
state, retirement flag or survivor movement was injected. The fatal setup is
not evidence of a fatal enemy hit. Ledger stays34/0/200.


## Coherent content setup and native ladder evidence (2026-10-20)

Local debug candidate32C2C795
(`32c2c795975170691c0e06d237e06d7463235ae9776e53e3dfbb7e9533f46175`)
adds one reusable offline setup primitive: both hero positions/history and main
camera position/history are applied at the same native world boundary.
It preserves gameplay/control/collision/progression flags and camera bounds.
Both current and target bounds in every active native view must permit the
requested camera; scenes, vehicles, pending exits and netplay are rejected.
The Split camera origins are kept coherent with the existing cache restore;
no parallax ratios are invented. This is a setup bypass, not a camera, route,
entry or transition pass. Camera Y payload uses unused diagnostic+B4 because
existing both-door diagnostics own+A4.

`qa-capsule-content-20261020/paired-setup-qualified` passes actual locked-room
camera rejection without relocation, safe native GrizzlyCP0 paired setup,
unchanged campaign/clear/program bits, both living HP32 actors and subsequent
independent P2 Right. Debug17/17 CTests pass in2.91s. No current production/
netplay qualification or owner-payload refresh is inferred.

The same real native card cold-load driver passes explicit Zero choice on
both856484F5 and32C2C795: original04/04/05 roster selection, Zero/P1-X/P2,
earned X/P2 CShot and its owned menu. The earlier unexercised explicit choice
gap is now covered; the card remains byte-identical. This adds evidence to
existing passed rows, not another comparative item.

Actual stage6/0/checkpoint1 gives a bounded native ladder check:
`native-stage6-ladder` records simultaneous native Right+Up ascent from
Zero/P1 Y410 and X/P2 Y411 to266/267, through native ladder actions14/15.
That driver incorrectly measured ascent after both were already at the top,
so its failed caller assertion is retained. Same-session
`native-p2-ladder-ownership` then passes P2-only alignment, Down to344.5,
Up to267 and return animation, while P1 stays exactly(2229.5,266).
Native P2 actions15/16 are observed with both heroes alive; composed captures
are retained. Original checkpoint and HP/contact protection are setup
bypasses. No animation/contact/ladder flags or positions were injected into
this input check. This does not establish a closing-hatch, route or exit pass.

Capsule access is still unverified. CP3 fixes cameraY240 below authored capsule
Y145; its absent actor is not an interaction failure. Earlier-checkpoint
large jumps did not establish native route-camera prerequisites and produced
setup deaths or script changes. The paired primitive avoids incoherent relocation but does
not resolve those native prerequisites. An explicit camera-minY access
bypass was also rejected by the setup guard; its precise rejection predicate
was not isolated. No grant was reached or seeded. All receipts remain under
`G:/Temp/Codex/psxrecomp/x5-parity/qa-capsule-content-20261020`.
Further capsule setup variations are stopped. The native upper-route pose
observation likewise is not a traversal pass.

The source-only next-stage audit is
`G:/Temp/Codex/psxrecomp/x5-parity/subagent-x4-qol/next-stage-transition-batch.json`:

- Stage6/0 exit is an actual HP32 solid class11/subtype0 at3744,280. Native
  attacks must destroy it, followed by31-update cleanup and a living
  participant crossing x3761. Then original progression supplies6/1 CP0.
- Stage4/0 exit is solid0C/subtype0 at4418,2760. Its distinct61-update
  cleanup and living-HP/x4404 predicate precede native4/1. Neither exit is
  source-proven a miniboss and neither is runtime-qualified.
- Raw14/0 is original STARTUP dispatcher content
  (1E5A4->80070EFC[1]->1F314->1F108), with hidden on-foot body and
  screen-oriented type13 actors. It is inapplicable to gameplay jumping,
  while its native startup presentation/Start exit still needs qualification.
  The otherwise-needed boot observation did not reach the expected native
  interactive phase within35s; it is retained without another boot.
- Raw11/1 has a null checkpoint pointer80072D6C. Do not force
  `load_location(11,1,0)`; a genuine native entry contract remains missing.
  Native mode20 has a concrete11/0 story constructor and actual cold-card
  story evidence, but that alone does not rule out other11/0 gameplay paths.
  Preserve the old forced-mode watchdog evidence without treating it as
  proof of a naturally reached gameplay crash.

Source qualification adds14/0 to the prior five frontend tables:
20 candidate gameplay tables/107 raw checkpoint records remain from the
original26/113. This changes applicability, not completion; story/frontend
obligations remain. The234-report comparative ledger stays **34 passed,
0 failed,200 unverified**. This batch adds reusable access infrastructure,
source contracts and native ladder/selection evidence; it closes no new row.
Work and issue updates remain local.

## Both-door clearance before native boss admission (2026-10-20)

Debug candidateCACCC785
(`caccc785445333e13db854ed1b3d56cc24539e2d11dcb833b9200d2c1d50b55a`)
fixes a real progression stall found during the stage8 boss sweep. On32C2C795,
the native boss initializer issues stand14 while the Both door is still open.
The opener stops atx4249 before the camera/partner clearance can complete;
the native boss waits for the still-pending door. Both heroes remain alive,
but the sequence never reaches the warning or releases controls.

The existing enemy-dispatch wrapper now holds only an uninitialized native
Maverick actor while a Both door is active. It resumes the original initializer
after the actual crossing/closing branch; no warning, HP-fill, control, death,
clear or reward flag is manufactured. The guard also applies without Split;
that additional runtime path and current netplay remain unverified.
There are no new hook sites or persistent simulation fields.

Evidence root:
`G:/Temp/Codex/psxrecomp/x5-parity/qa-boss-sweep-20261020`.

| Check | Result and limits |
| --- | --- |
| Original stall | `stage8-native-door-admission/stalled-state.json` retains the active phase2 door, original class37 boss, both living bodies and premature stand. Earlier contact-protected waiting is separately a setup failure: native549E8 rejects A4 immunity. |
| Stage8 native admission | `stage8-deferred-entry/report.json` passes:63 native initializer deferrals; boss remains uninitialized throughout sampled open traversal. At first real initialization, body left edges4362/4360 both exceed door-right4224. Original warning, HP56, native damage-enable8B=1 and both controls follow. No stage8 defeat is credited. |
| Existing flat-door regression | `grizzly-both-regression/report.json` passes single arrival waiting, native opening, both owned menus, both reversals, deep opener waiting, real closing and preserved private HP/ammo. No extra roster/camera/loading matrix. |
| Build checks |17/17 debug CTests pass in2.40s. Source C changes are limited to the existing dispatch wrapper. Production/owner-payload/netplay qualification is deferred. |

`qualification.json` independently checks the native actor fields and derives
physical left edges from the original EXE foot boxes. Feedback61 is now a
bounded pass, using this new admission evidence alongside the named earlier
unavailable-player/owner-death checks. The ledger is **35 passed,0 failed,
199 unverified**. Other geometry, exact checkpoint timing and mounted/abort
coverage remain open.

The actual boss defeat was attempted but is **unverified**. One delayed handoff
found the fight had already undergone a native team retry. Immediate final-hit
attempts then exposed setup interference: contact fixture12 continually resets
native timer61, so an already-hurt X can remain in action17 instead of firing.
The reusable final-hit driver now uses HP-only refill, preserves native hurt
recovery, selects the actual health-bank actor and title-specific class, and
does not rewrite an existing positive1HP setup. Its subsequent stage8 attempt
lost a hero during native knockback. Neither failure proves a buster, boss
lifetime or arena-height bug; all reports and images remain retained.
No stage8 defeat, paired victory, reward or return is passed. Further combat
setup variants are stopped for this milestone.

The intro0/0/CP3 load likewise loses both heroes before boss admission; it does
not reproduce feedback41 or54. Their original-code recipes are retained in
`subagent-x4-qol/intro-boss-final-hit-recipe.json` and both rows remain open.
Stage8's source recipe correctly distinguishes its native class37/HP56 from
Grizzly class36. The source-only ordinary Maverick map is inventory, not
runtime coverage; stage7's highest checkpoint7 is an alternate earlier route,
not the nearest boss approach. Actual routes/bosses, snapshots/rewind, current
production/netplay and watchdog stability remain campaign gaps. Watchdog and
default diagnostics/hint suppression are unchanged. All work remains local.

## Two native Maverick clears and cumulative polish (2026-10-21)

Debug CACCC785 is unchanged from the Both-door C milestone. This batch adds
three reusable runtime helpers and evidence; it needs no diagnostic hooks or
new build variants. Evidence root:
`G:/Temp/Codex/psxrecomp/x5-parity/qa-maverick-batch-20261020`.
`qualification.json` checks the named receipts; `composed-review.json` records
agent visual review of the actual composed captures, not owner validation.

| Check | Result and bounds |
| --- | --- |
| Raw2/1/CP2, class39 | Actual Both-door admission, original HP46 fill/control release, HP-only protection and native X/P2 final hit after positive1HP setup pass. Both original victory poses and outgoing beams finish before the initialized report earns clear bit2. `stage2-native/report.json` and its `clear/report.json`. |
| Raw3/0/CP6, class38 | Same reusable checks pass with the distinct original HP63 and actual native hit/death. Original report changes earned mask2->6, preserving Zero campaign1. `stage3-native/report.json` and its `clear/report.json`. |
| Rewards / real Saves / returns | Native P1 inputs navigate rewards, Save and story. Actual card1 slot checksums5035/mask2 then5552/mask6 pass; unselected card2 stays unchanged. Second Save includes native occupied-slot overwrite confirmation. Both actual selector returns pass in `stage2-native-return/report.json` and `stage3-return-qualified/report.json`. No card serializer, reward or success field is injected. Current mask6 cold Load remains open. |
| Earned portrait | `stage2-earned-portrait/report.json`: original stage table maps raw2 to portrait6. All16 destination words equal authored cleared art and differ from normal art, before/after actual cursor away/back. Both composed panes show cleared art. It also persists after raw3 at the mandatory Dynamo selector; that supplemental read-only check leaves cursor refresh unverified because native encounter ownership blocks directions. Closes legacy20 and feedback17. |
| Victory cleanup | `victory-no-repeat-qualified.json`: each hero enters native victory25 once, then outgoing1 and inactive state3 before report initialization; neither returns or repeats the pose. Closes legacy19. |
| Post-victory ordinary effects | `post-victory-native-stage3-dash/report.json`: actual native next-mission entry, then X/P2 dash creates213 private trail draw observations with its own assembly and natural expiry while P1 is neutral. World/native/body ticks advance38/38/38. Closes feedback13 for ordinary Maverick cleanup; vehicle-specific effects remain open. |
| Mandatory encounter | After the two real clears/saves/returns, native selector owns center cursor8 with PLAY10F2 and section0. Original Cross selects first Dynamo9/0/CP0, class3B/sub0. `subagent-x4-qol/dynamo-native-encounter-recipe.json` is source only; no fight/defeat/return pass yet. Launch changes PLAY10F2->3 before combat, so it is not defeat evidence. |

Checkpoint loads bypass routes. Only the first raw2 setup bypasses the
post-intro chapter and old-story queue, explicitly recorded; intro completion
is not credited. HP-only refill preserves native contact/hurt recovery. Boss
HP is lowered once to positive1; native attacks must cause death. Companion
position/history uses the finisher's observed grounded floor. No tested death,
clear, reward, Save, selector or story-success flag is seeded.

Earlier observer failures remain retained. Actual native continuation corrected
invalid assumptions about the mode3 constructor, PLAY27 as fade, the fourth
selector header byte, and the separate occupied-slot overwrite prompt. The
actual fade byte is8009A424 and grid dispatch prefix040400; no frontend flags
were rewritten to satisfy those checks. Mandatory Dynamo art persistence is a
partial supplemental check, not another cursor pass. Earned weapon2 selection
and mirrored masks do not prove its special control-actor attack or ammo debit.

The frozen234-report ledger is now **39 passed,0 failed,195 unverified**.
There are three qualified Maverick defeats across named candidates (prior
Grizzly plus raw2/raw3 on CACCC785), not three on this binary or full routes.
Five ordinary Mavericks, actual Dynamo, refights, fortress/endings, broad
section/alternate/exit traversal, snapshots/rewind, current production/netplay,
and watchdog stability remain open. Stage8 combat and capsule/intro access
variants stay deferred. Default diagnostics, hint suppression and watchdog
behavior are unchanged. Everything remains local.

## First mandatory Dynamo and current cold native Load (2026-10-21)

The same CACCC785 candidate now qualifies the distinct first Dynamo path from
actual earned mask6. `qa-maverick-batch-20261020/dynamo-qualification.json`
links the same-session native entry, actual final hit, owned departure, real
Save/return and subsequent cold Load. No runtime C change or build variant.

| Check | Decisive evidence and limits |
| --- | --- |
| Actual native mandatory entry | P1 Cross on earned center cursor8, PLAY10F2/section0, then original Zero confirmation/title/READY enters9/0/CP0 and class3B/sub0. Fourteen ready original intro pages are confirmed with native P1 input. Both living bodies, original HP fill and control release are retained in `dynamo-native/report.json`, continued in `dynamo-final-hit/report.json`. |
| Dynamic native HP | Original24548 scales initial58 using earned progress, time and rank. Actual fullHP64 equals current PLAYA2, with released body D0 and original live boss state. The earlier hardcoded58 observer assumption is corrected; no admission/HP flag is manufactured. Source receipt: `subagent-x4-qol/dynamo-native-encounter-recipe.json`. |
| Actual defeat | Positive bossHP is reduced once64->1. Actual X/P2 normal buster input causes original zeroHP/action2 departure. `dynamo-final-hit/report.json` passes these subchecks; its overall retained failure is the P1-only text helper, not a native defeat failure. |
| Owned departure / native report | Scene owner is P2 after its final hit. P1 Cross does not advance the ready page; original P2 Cross advances six actual pages. Native animation/departure retires class3B, sets PLAY0F10, then reaches initialized mode12/raw15/0 with oldstage9. `dynamo-departure/report.json` passes. No ordinary Maverick victory/clear bit is imposed on this distinct path. |
| Real Save / return | `dynamo-save-return/report.json`: actual native occupied-slot Save completes, card1 checksum5275/mask6; card2 unchanged. Actual ordinary selector04040000/raw13/0/cursor0 retains Zero campaign1, earned mask6 and Dynamo progress3. Progress2->3 occurred at launch, so only the actual hit/departure/report proves defeat. |
| Current cold native Load | Old owned session62648 stops normally after the real card copy is retained. A fresh CACCC785 process loads that unchanged private card through title/Continue/native Load, with observed mask0->6 decode and real selector. Native saved default X0 is observed; explicit original Zero selection enters earned raw2/0 with rosterZero1/X0, both BODY+C9 masks6 and Dynamo progress3. `cold-native-load/report.json` retains the first-cycle observer mismatch; `cold-load-qualified/report.json` completes the same actual flow. |
| Earned selections / recovery | Original R1 order is0,1,5,6,3,8,7,2,4, with separate authored masks. Earned mask6 correctly cycles0->3->2->0, observed with native P2 inputs, not ascending0->2. Both heroes independently move160->165.5 on native Right, then P2 opens its owned original menu. Actual loaded card remains byte-for-byte unchanged. No weapon attack/ammo spend, full route or snapshot claim. |

The three retained observer mismatches have concrete explanations: native HP
scaling, a transient projected PLAY44 character during gameplay, and the
original nonascending weapon order. Campaign identity is checked in native
pause/stable frontend and actual saved data; an intermediate P2 projection is
not treated as a progression mutation. P1-only story helpers remain appropriate
for shared frontend pages, while an owned in-game scene uses its controller.
No tested transition, text completion, clear, reward or save flag is seeded.

Actual composed captures show readable original white dialogue on its blue/
purple panel in both panes and the ordinary selector with both earned cleared
portraits. The early initialized-report image is still fading and is not a
report-readability pass. The paused `fight-ready.png` is the original inventory,
not an arena rendering pass. The hero hidden during P2-owned departure follows
the existing scene transport policy; broader boss-dialogue partner recovery
and exact original palette comparison remain open comparative checks.

`test_mmx5_coop_native_load_runtime.py` now accepts an earned stage to revisit
and checks the exact original weapon-order/mask tables through a full acquired
cycle and wrap. Its default remains the earlier Grizzly card. This batch's
actual chained native receipts exercise the generalized stage2/mask6 path;
Python syntax and diff checks pass. The runtime C candidate is unchanged, so
its prior17/17 debug tests remain the applicable build evidence.

The234 frozen X4 rows stay **39 passed,0 failed,195 unverified**. This batch adds
one distinctive boss/progression path and current native save-Load qualification,
not new variants or duplicate comparative closures. Five ordinary Mavericks,
second Dynamo, refights/fortress/endings, broad section/route/exit traversal,
snapshots/rewind, current production/netplay and watchdog stability remain open.
No owner-facing payload is replaced or launched. All work remains local.

## Private E contents and verified feature gaps (2026-10-21)

CandidateBEA439FA
(`bea439fa7db57f9695c9c1164049de929cf80f2ba74618526c1ebeda02de1be3`)
passes18/18 debug checks in2.37s. Evidence root:
`G:/Temp/Codex/psxrecomp/x5-parity/qa-x4-interactions-20261021`.
`qualification.json` validates the named native receipts and exact HP changes;
`composed-review.json` records actual composed menu inspection.

`separate_etanks` is default-off, as in frozen X4. It projects both E bytes
across native P2 menus/body passes and the manual/deferred item collector;
acquisition and W remain native/shared. Contents survive section/checkpoint
initialization and rejoin; native new-game/card-Load frontend clears the cache.
The existing native card format is preserved: original1CAA4..1CAAC empties E/W
contents on actual card Load, so native card energy persistence is not promised.
Private bytes/initialization/option enter deterministic state; private drawing
backs up/restores them. No snapshot support is implied.

| Same candidate, reusable session | Result and individual evidence |
| --- | --- |
| Private healing / menu authority | `final-private-etanks/report.json`: native P2 E8 heals11->19, preserving P1HP13 and its E16; P1 then heals13->29. Other-seat Start cannot resume either owned menu or advance the world. |
| Native pickup / shared capacity | Original fullHP pickup21 gives4 E units with FC40off, independently of its16HP hurt heal. P1's empty E cannot spend P2's refill; P2 then heals11->15. Original P2 pickup28 actually adds second-E acquisition, shared by both. Allocated item setup is disclosed; authored placement admission is not passed. |
| Two-slot compaction / reload | Seeded P2 firstE32 plus native secondE refill4; original firstE heal1->32 compacts second4 over first. Actual original checkpoint continuation then retains4; P2's real E use heals11->15, while P1 remains29 with empty E. The native fullHP completion rule is preserved, not replaced with guessed leftover energy. |
| Default shared control | `final-shared-etanks/report.json`: default-off private policy consumes shared16 for P2's11->27; P1's later empty-E use cannot heal. P2's fullHP pickup refills common4, then P1 heals13->17 using it. Same BEA439FA binary; no roster/camera/network matrix. |
| Shared W / diagnostics | Native W5 use consumes the common W contents in both policies without changing either HP. `privateE-logger-receipt.json` and `sharedE-logger-receipt.json` prove actual default-on logs record optiontrue/false on G:. |
| Existing progression control | `final-shared-cold-load/report.json` passes actual cold native Load of the prior earned mask6/Dynamo card, native Zero choice/entry, both acquired masks, original X cycle0->3->2->0 and owned guest menu. Earlier4FC76E8D also passes the combined default pickup/tank/withdraw-return/script-passenger regression. |

Early failures remain retained and have source explanations: Up alone selects
E1cursor0A; extra Right can select W. Native pickup21 supplies different HP/E
arguments. Native E completion can compact slot2 even at fullHP. PLAY44 may
transiently name projected P2 during gameplay, so the native-Load helper uses
original frontend choice plus coherent body roster. The completed E check
waits original menu state800D4F45==1 after healing/compaction; it does not pass
an intermediate HP sample. These corrections required no simulation variants.

Feedback180 is now bounded passed. Two applicability/implementation audits
also resolve unknowns into **failed** contracts, not gameplay passes:

- Feedback181: X4's optional boss scaling is X50%/Zero40% plus its recovery
  policy, not a separate Zero20% toggle. X5 still has no option/hook/cooldown
  integration. The portable `mmx5_coop_damage` helper and focused tests cover
  shared fractional carry, accepted-hit commit, rejection/discard, undo,
  independent health banks, actor reuse and lethal cleanup. It changes no
  gameplay by itself; integration/native acceptance remains required.
- Feedback33: X5 explicitly blocks co-op state save/Load and rewind and has no
  co-op snapshot provider. Native memory-card qualification cannot pass this.
  The bounded source blueprint is
  `subagent-x4-qol/x5-snapshot-gap-audit.md`: engine provider, prevalidation,
  retained art/DMA IDs, menu VRAM, audio, private E and one combined restore/
  rewind session. Raw guest RAM or host-struct copies are insufficient.

Native section-exit checks remain unverified. One stage6 CP1 upper-route
position setup crushed P1 before the exit. One stage4 CP2 approach never
admitted the original subtype0 exit under its native camera prerequisites and
later lost P1. Neither establishes a controller/exit bug. Access variants are
stopped; no barrier death or section-success flag was seeded.

The frozen234 rows are **40 passed,2 failed,192 unverified (194 open)**.
Current production/netplay, snapshots, balance integration, broad content and
watchdog stability remain required. No publication or tracker synchronization.


## 2026-10-21 ? offline state and rewind milestone

Candidate **ADBB69655A62FF7483F25D7DD1CB6E48D536EE6128D4F26AA5A5150C0B855580**
uses the existing debug-tools/test-cheats build, original disc and hidden,
muted OpenGL runner. No shared SDK/UI edits, new native hook sites, remote
operations or owner payload replacements. Evidence root:
`G:/Temp/Codex/psxrecomp/x5-parity/qa-snapshots-20261021`.

The title now registers explicit LE schema1 under `mmx5.coop`. It preserves
both private actor/shot/glow/trail/car contexts, lifecycle and transition
ownership, contact/solid/pickup histories, private E bytes, cameras/placement
visits, audio initialization and exact retained graphics/menu buffers.
Validation reserves offside buffers only; read transfers them without
allocation. Immutable art/UI/audio prepare at activation without native
initialization, so cold Load can validate and reconstruct saved texture IDs.
The four texture ranges are forgotten/rebound and SPU sample residency
rebinds before the saved PC resumes. Offline state/rewind are enabled;
netplay user jumps remain blocked. Stable native menus and script holds
remain eligible rather than evading serialization through a blanket block.

| Bounded check | Evidence / result |
| --- | --- |
| Combined actor/resources/menu/world/scene mechanics | `final-combined/report.json` passes. P2 private E8 heals11->19; loading its earlier menu restores both positions/HP/ammo, world clock, owner and the spendable8. A live-world save survives original checkpoint reload, including visible native buster/slash art. Controlled original stand/release calls establish owner1/hidden living P2; save/release/load/release restores that ownership and returns native control. Setup bypasses route/acquisition and the authored script trigger, never campaign success flags. |
| Cold Load and mapped controls | `cold-final/report.json` passes. Before Load: no co-op enrollment or warm retained banks. Saved P2 menu restores HP13/11 and original Zero menu art. Normal host Select+R1 slot panel and Square perform Save; `mapped-cross-load/report.json` then proves Cross actually loads slot0 and returns that P2 menu/resources. These use the normal mapped host controls; no physical F7 keyboard event was injected. |
| Rewind / attack and control recovery | Same `cold-final` run uses normal Select+R3, filmstrip Left and Cross to restore the earlier owned P2 menu after world movement. It resumes once; the restored Zero slash naturally finishes, then native movement resumes. Early failed input timings remain in `cold`/`cold-mapped`; a held filmstrip direction and waiting for the restored native attack/load-input guard fix the harness, with no game-code variant. |
| Atomic incompatibility rejection | `schema-rejection/report.json`: only the co-op host-provider schema is changed1->2 in a copied private state. Actual Load refuses it; PLAY, both coherent bodies/life and world time remain identical. The actual-walker unit test separately covers truncated/extra bytes, fixed-option mismatch, different warm-cache sizes, exact retained sprite/menu pixels and allocation-free read. |
| Art / logging / audio limits | Composed `B-restored`, `A-restored`, cold menu, rewind menu/world and controlled hold captures retain both native art and HUD ownership; no absent/stale bank observed. Diagnostics start new G: baseline segments on successful restore. Audio unit coverage verifies initialized flags and SPU epoch rebinding without guest writes/reopening VABs; audio listening/complete overlapping playback continuity remain unverified. |
| Capacity | `snapshot-sizes.json` records actual host wire sizes below the existing16MiB aggregate limit. Exact pixel payloads avoid volatile-art reconstruction assumptions; caches can theoretically exceed that limit, in which case capture refuses. No unlimited-cache/history claim. |
| Authored scene access | `combined`, `native-scene` and `native-pair-scene` retain timed-out Grizzly approach attempts. Stationary-passenger approaches hit Unified separation at9717.5; moving the pair still did not activate the encounter. Investigation stopped; no admission/door/actual boss-scene pass is inferred. The controlled stand check qualifies restore mechanics only. |

The approved worker completed `f181-native-accepted-hit-recovery-audit.md`
and `x5-snapshot-provider-source-review.md` on G:, without touching runtime or
tracked files. Its combat audit identifies accepted zero-loss contacts,
different native recovery fields, class34/Dynamo phase protection and two
HP-derived reactions that must retain the unscaled hit. There is no broad
cooldown patch or new combat matrix. F181 stays failed until integration and
native gameplay evidence exist.

F33 is now **passed bounded** for offline snapshot/mapped-control mechanics;
the frozen234-row ledger is **41 passed,1 failed,192 unverified (193 open)**.
Other authored scenes, content/progression, boss balance, current production/
netplay and the existing watchdog issue remain required work. This is a local
milestone, not full X4 parity.


## 2026-10-21 â€” overlapping falling-object comparisons

Candidate **D7BAE9109E6934F46EB6194956561B1EA7B1BBB9D12D725A008AEA6536C589FB**.
Evidence: `G:/Temp/Codex/psxrecomp/x5-parity/qa-environment-20261021`.
Same local branch/debug-tools candidate; no remote operations, shared SDK/UI
edits or owner-payload replacement. F:/G: readiness/I/O checked; G: holds new
bulk outputs while F: is busy. Games stay hidden/muted. Exactly one approved
source worker; main owns the game/build.

| Comparative mechanism | Bounded native evidence |
| --- | --- |
| F168 / P2 falling-object admission | Content tables give raw0/0/CP1, placement42 class1/sub1 at2992,960 and original F588C signed PLAYER.x>=2880. `slab-trigger` ADBB6965 proves P2at2896 with P1at2688 does not arm; P1samepoint arms native6=1/acceleration1000. Position bypasses the preceding block/route, leaving the object/trigger untouched. |
| Correction / active hook | Only original F588C runs once under P2 when P1 has not met the threshold. Parent F5C30 stays canonical for direct shot queries, its immunity gate and both solid-contact passes. Native code owns phase/acceleration/shake/fall/collision/retirement. Configured overlay entry participates in cache identity; actual counters0->1 and1->2 prove execution. First60469544 wrongly required header0; runtime observed4. D7BAE910 uses verified stage/function/table checks instead. Failed receipts retained. |
| F112 / lifetime after P1 death | `final-slab/report.json` passes: native arm, P1 death, then falling and actual retirement over74 world ticks with P1DYING/P2ALIVE. P1 reachesFALLEN and P2 jumps; retries stay2 and completion flags unchanged. `fallen-lifetime-completed/report.json` additionally starts with P1 fully FALLEN: original subtype2 arms/falls/retires, retries stay1 and P2 jumps. Existing survivor stage projection admits it directly, so the narrow override counter correctly stays2. The initial harness incorrectly expected another override; its failed receipt is preserved with the corrected check. Temporary-world-object lifetime analogue only; no timer-based support-platform/ice geometry pass. |
| Real team wipe/retry | P2 then dies through native fatal handling. Original retry restores checkpoint1 and grounded pair, spends exactly one retry2->1. No loader/continuation fixture substitutes for that retry. Fresh placement42 arms/falls/retires for P2 again, counter1->2, no healthy-seat death. F45 usable support/contact remains unverified. |
| Native primary control | `canonical-P1-control/report.json` on same candidate: P1crosses threshold while P2stays below; native phase/acceleration change while P2override counter stays2. |
| Combined snapshot regression | `split-snapshot-menu/report.json`: same-candidate localSplit/defaultsharedE saves P2-owned menu, resumes/moves/changes owner, then loads both positions/HP/owner/world clock and recovers native P2 movement. Actual composed art/HUD retained. Extends F33 without claiming cold Split or netplay. |
| Water / other platforms | Raw3/0/CP1 entry/ground-jump prep retained; no original waterline/splash/ripple producer identified within source timebox. Water stays unverified. Falling object is not a lift/support; F29/45/46/49/51/105/111/201 retain their own checks. X5 signed carry-read alone does not prove a negative-release producer, so no speculative X4 carry patch copied. |

All19 CTests pass in2.83s. Existing immutable snapshot state/schema is
unchanged: the trigger reuses the guarded area-call projection and adds no
persistent host field. Native source regeneration used only the recompiler.
Source worker receipts: `environment-water-falling-native-recipe.md`,
`intro-slab-dispatch-ownership-review.md`,
`intro-slab-trigger-helper-config-review.md` on G:. The latter has a parent
runtime addendum correcting its unproved header0 assumption; source review
did not replace runtime qualification. No roster/camera/backend matrix.

Frozen234-row scope remains **43 passed,1 failed,190 unverified (191 open)**.
Boss balance, water/lifts, content/progression, current production/netplay and
the existing watchdog issue remain required work.


## 2026-10-21 ? earned weapon and personal-menu overlap

Candidate **893D5C8A023523A9BB501875D8F90B5B3F7912DD49EEE7274D43F65954FE6977**.
Evidence: `G:/Temp/Codex/psxrecomp/x5-parity/qa-menu-attacks-20261021`.
Same local branch, one build after a concrete native failure. G: holds all new
bulk files; F:/G: healthy/free-space/I/O checked. Owned games stay hidden/muted.
The sole approved worker supplied source/review and stopped; main owns runtime.

| Comparative interaction | Decisive evidence and limit |
| --- | --- |
| Earned acquisition prerequisite | `fixed-native-earned-load` cold-loads unchanged real card977FA138, earned from the earlier actual P2 Grizzly final hit/native reward/save. Original mask0->1 decode, story/reward/selector, explicit Zero choice, pair entry and acquired X cycle0->1->0 pass. No reward/weapon-availability flag seeded. |
| Confirmed attached-effect bug / X5-03 | BaselineD7BAE910 original CShot init uses private X art, but deferred46900 reads restored P1 SHOTS: actual native FX XY writes0,0. Native type5 stores its private shot at+50. Extend the existing per-allocation-owner effect wrapper only for type5/address46900 and aligned SHOTS parent; original46900 still owns following/cancellation/retirement. Configured entry regenerated by recompiler. |
| Primary physics preserved | Earlier trace46A40/RA46A10 was original terrain contact70=6, PC2FB34. Native muzzle379,370 and negative-X motion establish left-facing setup beside terrain, not a demonstrated physics bug. Original Right establishes positive facing for the accepted-hit check. No motion/contact/expiry override. A later airborne shot reflected from the nearby enemy (RA469C0); its failed short FX sample remains a setup/observation limit. |
| Legacy8 / pause and attack recovery | Both owned menus preserve roster/HP/ammo and reject other-seat resume while world ticks remain frozen. Each resumes independent Right. Actual X/P2 selected CShot hits the original class14 enemy16->13 and spends only its ammo288->282, then original cleanup completes. Native attacks remain usable. |
| F10 / ordinary post-menu graphics | `final-menu-combat` and `native-primary-attacks` retain both native bodies/HUD/terrain after menus. Actual P1 ZBuster class3A and green/white saber class18 draw and finish. No solid-purple/menu-return corruption seen in this ordinary encounter. Heavy-scene graphics and full CShot pixels remain separate. |
| F179 / personal menus and selected weapon | Native red Zero/ZBUSTER/earned C-SWORD and blue X/C-SHOT menus inspected; guest Select cannot reach campaign Quit. Both heroes recover native controls and use their original weapons. This closes personal-menu authority and selected-weapon use in this roster; earlier opposite-campaign Quit/re-entry evidence stays linked. |
| Original effect lifetime after the fix | Same-session clear-space native CShot276->270 spends only P2 ammo; attached FX retains private X assembly9F0506A4 and follows(280,354)->(317,339). Owner callback5->20 over this original shot; native projectile count/effect clear afterward, idle partner resources unchanged. Setup bypasses route/places P2, never native activation/contact/retirement. |
| F150 / projectile appearance still open | Clear-flight frame2 contains small cyan/magenta fragments. Native motion/damage/assembly and a correct menu icon do not prove original full projectile geometry/palette. Keep unverified; compare original assembly/texture UV/CLUT before another build. No more timed-capture or loading variants in this pass. F18 fades and F144 heavy scenes also remain unverified. |

All19 CTests pass in2.55s on this candidate. The same-session
`ordinary-effect-reuse` regression passes existing effect1 dash exhaust
(delta3) and effect2 full-charge buster (exactly one owned native child,
original cleanup and untouched P1 shot counter) after effect5 use.
No persistent field or snapshot
schema added: the existing personal-effect owner array, digest, snapshot wire
state and projection readiness already cover type5. Source reviews:
`subagent-x4-qol/cshot1-native-projectile-contact-recipe.md` and
`cshot-attached-effect5-implementation-review.md`; runtime evidence is separate.
Initial mistaken CP1 setup and dead-seat continuation rejection are retained;
actual content inventory identifies the starting location as CP0. They do not
qualify traversal, retries or a game failure.

Frozen234-row scope remains **46 passed,1 failed,187 unverified (188 open)**.
F181 boss balance, full CShot presentation, broad content/progression,
water/lifts, current production/netplay and watchdog stability remain required.

## Retained CShot appearance and grouped Chaser entrance (2026-10-21)

Candidate **9796C215A59F810D36A99B8D1D630D9B5432AD38150D4AC46424B48583894B90**.
Evidence: `G:/Temp/Codex/psxrecomp/x5-parity/qa-chaser-20261021`.
Frozen234-row ledger: **48 passed,1 failed,185 unverified;186 open**.

| Check | Decisive result and limit |
| --- | --- |
| F150 / original selected-weapon appearance | Native earned Grizzly card Load; both owned character menus, correct cyan X/P2 CShot icon, rejected wrong-seat resume/guest Quit, retained resources and resumed controls. Original DAT frame2 decoded pixels plus current selected CLUT match every300 opaque samples in each2x composed pane with zero channel error. Live32-byte palette equals original DAT88/type9 offsets60/E0. `cshot-frame-proof/pixel-geometry-review.json` and subagent `cshot-live-palette-original-match.json`. Passed bounded; other weapons/armor/network are separate. |
| Original projectile behavior retained | Actual Triangle spends only X/P2 ammo288->282 then282->276; shared FX5 receives retained draws (counter42->64 on the second shot), original owned muzzle and native retirement. `cshot-frame-proof/report.json`. No motion/hitbox/expiry patch. Original accepted-hit evidence stays on893D5C8A; do not relabel it current-candidate damage evidence. |
| F60 / enrollment during native Chaser entrance | Same process crosses from ordinary Grizzly gameplay to native stage5/0/CP0. Both original car mode0 arrivals then grounded mode1, mounted/facing identity and shared CAR+A0 opening controller8009EB90 observed. One opening READY appears per composed pane. `fixed-entry-ready-dash` and `native-controller-entry-dash` retain samples/captures. Passed bounded entrance enrollment; full road/exit is unverified. |
| Legacy16 / full visible READY retirement | Both harnesses remain unsuccessful overall. The first incorrectly waited for type1B object retirement; source proves it persists as a completion controller. The second reaches sub5=3/count14=0/completion16=1, but the composed capture still displays READY. Controller state alone is not a disappearance pass. Both cars later fall before a clear grounded dash window. Keep full retirement/dash/layering unverified; no more variants/builds in this timebox. |
| QA mission teleport | Original fixture9 changed rawstage while old world actors still ran and hung at sprite lookupPC8002B614, before Chaser load. Defer only destination fields to native20B80 mission entry, preserving existing pending checkpoint fields/schema. Current actual Grizzly->Chaser loading/native entrance succeeds. `loading-context.json`, `loading-disposition.json`, both current entrance receipts. This fixes audit setup; teleporting is not a route/transition pass. |
| Source/content audit | All Chaser road/factory/boss checkpoints belong to raw5/0; there is no5/1 table. CP1 is not automatically an on-foot section pass. Native road departure needs original camera completion and real car departure, not seeded flags. See subagent `chaser-road-ready-runtime-recipes.md` and `chaser-authored-road-content.json`. |
| Candidate checks | Release-config development/cheat candidate builds;19/19 CTests1.31s, including assets, snapshot and hook checks. Recompiler regenerates only the affected shard. Focused source review checks native painter resources, owner palette selection and existing local-view/snapshot boundaries. No new persistent fields/schema or shared SDK edits. Production/current netplay remain unverified. |

The graphics correction temporarily normalizes only unremapped native header
origins during the original painter and restores the header immediately.
Original geometry/flips and remapped layouts retain their native paths.
Exact live shared FX5 with an aligned SHOTS parent uses its existing allocator
owner for retained pixels/palette, including canonical palette restoration
when P2 owns the world pass. It replaces the original draw once; no player
projection or effect update is added during drawing.

The first CShot driver failed an overly strict two-position sampling assertion
after its three captures caught one live FX position; both menus/control and
native cost already passed. Its failure is retained. A single subsequent shot
uses existing frame history for appearance proof and original cleanup. Its
initial palette read used body graphics+50000, overlooking the body table's3C
offset; the analysis receipt explicitly replaces that slice with the resource
base derived from original CShot table28998. No passed claim uses the wrong
slice. These are harness corrections, not hidden game failures.

No owner handoff or publication is made. PID62732 stopped normally after final
observations; the previous hung PID53700 was stopped before this candidate.
The sole approved worker supplied bounded source reviews and stopped. Full
goal scope remains234 individual comparisons; boss scaling, broad routes,
progression, water/lifts, current production/netplay and watchdog remain open.

## Factory content, actual class32 clear and first Dynamo (2026-10-21)

Same **9796C215A59F810D36A99B8D1D630D9B5432AD38150D4AC46424B48583894B90**
as prior milestone; no rebuilt game or loading variant. Evidence:
`G:/Temp/Codex/psxrecomp/x5-parity/qa-factory-batch-20261021/qualification.json`.
Ledger **49 passed,1 failed,184 unverified;185 open of234**. Legacy17 closes
only the native vehicle-stage boss-door/arena analogue, not its earlier road.

| Shared session check | Actual evidence / limit |
| --- | --- |
| Native earned card and content setup | Actual Grizzly card Load, Zero/P1-X/P2, default loading/hints/diagnostics, localSplit and Both doors. CP2/3 use original5/0 records, not invented sections. No tested clear/reward bits are seeded. |
| Factory collision and independent controls | CP2 both grounded at6464,762/763, each native jump rises25.484375px and lands; CP3 both at7440,1274/1275, rises35.2734375/42px and lands. Other hero stationary during each rise. `factory-checkpoints`. These launch-point checks do not prove factory route, enemies/audio or alternate exits. |
| Actual boss door / legacy17 | Settled original CP4 retry at8496,1274/1275. Native P2 Right reaches the second authored door while P1 remains8496; Both waits. Native P1 approach acquires it; both walk through at8632/8631 then8807/8804.5. `native-door-retry` retains the actual crossing despite its later scene assertion. Actual boss initializes at authored8848,1264, descends to arena, fills53HP, and releases both grounded heroes. `boss-admission`. First door and road handoff stay bypassed. |
| Real class32 defeat | One positive53->1 boss-HP setup, native X/P2 buster supplies zeroHP/death. Native effect retirement, both action25 poses/held poses and real report mask01->11 observed. `native-clear`. No defeat, victory, reward or exit flag write. |
| Rewards / real Save / progression | `native-reward-save-return`: actual earned weapon/report, original overwrite/Save completion, slot checksum5641/mask11, untouched card2 and actual selector. Selector correctly forces first Dynamo(cursor8); failed free-Maverick selection is retained as native prerequisite evidence, not a cursor bug. |
| First mandatory Dynamo | Native entry/dialogue/HP62. Original helper admitted fight before P2 JOINING finished and rejected its ALIVE precondition before any shot. Same-process resume completes P2 arrival/grounding in `dynamo-arrival.json`; original positive1HP retained. Actual normal buster causes zeroHP and action2 in `dynamo-final-hit`; no second HP write. |
| Owned dialogue and actual return | P1-only text helper correctly fails to advance P2-owned retreat dialogue. Reusable `confirm_native_text(seat=...)` now releases/polls the chosen pad and observes a real page change; six original P2 Cross pages advance. `dynamo-departure` sees original retreat/actor retirement/exit/mode12. `dynamo-save-return` performs actual Save and free selector return with mask11 unchanged. Earlier failed helpers are preserved, not relabeled successful. |
| Next environment analogue | Sole approved source worker maps raw8/0/CP1 to authored class12/sub0 descending support2128,928. Native singleton triggerX1944 can overlap lift activation, carry/camera, charge/travel, release, survivor and retry checks in one session. `subagent-x4-qol/environment-next-support-runtime-recipe.md`. Source-only; none of these lift rows close yet. Water producer, signed-negative carry release and distinct timed platforms remain unverified. |

Two unsafe position setups remain failed: the CP3 jump to the boss approach
exceeds its fixed camera interval and clamps the heroes to7672; the subsequent
CP4 raw-Y position attempt also loses both heroes, without a proven fatal
cause. An actual native retry settles the real floor at1274/1275, after which
native Right alone completes the second door. Do not call either failed setup
a game crash/door failure or a passage through the first factory door.

The post-close harness also wrongly required scene0 immediately. Original
class32 initialization legitimately begins its intro there. Source proves
phase2/sub4 waits native text byte80091C44==0; actual dialogue confirmation
advances4->5->6->8->9->10 and restores controls. No game patch was needed.
The separate Dynamo helper reused old arrival/owner assumptions before their
recorded corrections; the retained same-encounter follow-ups resolve them
without reload or forced readiness. These corrections add no test variants
to the234-row ledger.

The shared QA text helper keeps default P1 frontend input and adds explicit
seat1 for owned in-world scenes; it waits two real frames for P2 release
polling rather than trusting a transient canonical P1 PAD read. The existing
final-hit helper now includes the source-qualified on-foot stage5/class32,
rejecting mounted Chaser bodies. Python syntax checks and actual native
execution qualify these helpers. Game source, compatibility identity and
candidate binary are unchanged; existing19/19 CTests remain the named prior
candidate evidence, not a new production/netplay run.

Frozen X4 source/reference scope stays234. F181 scaling/recovery remains
failed, and broad routes/branches/endings, production/current combined
netplay, watchdog stability, water/lifts and owner acceptance remain open.

## Overlapping first-lift ownership, contacts and native retry (2026-10-21)

Candidate **078540141835563203508C2F8FEB12B955968FEE670825E198314FC671DBCA52**.
Evidence root `G:/Temp/Codex/psxrecomp/x5-parity/qa-lift-20261021`, with
`qualification.json`, original failed receipts, named runtime reports and
`overlap-analysis.json`. Ledger **57 passed,1 failed,176 unverified;177 open**
from the same234 frozen reports. No success/phase/contact/control flags seeded.

| Shared encounter / distinct ownership phase | Evidence and limits |
| --- | --- |
| Original failure and single canonical control | On9796C215, actual8/0/CP1 class12/sub0 parent staysphase2 for24updates with livingP1x1712 and nativeP2 beyond1944. Same-parent P1 native Right immediately startsphase5. `corrected-trigger`. Original scanner0C mistake is retained as unverified harness setup; corrected12 scanner uses the same encounter. |
| Canonical support-return failure | Native P1 descent reaches actual2128/phase8, but scene1/3 leaves P2 hidden. P1 terrain70=0, current solid71=8, actor72=8/76=1; native integer foot2112 equals original top2112. `canonical-destination` and `live-before-fix`. |
| Two bounded correctness fixes | Project only exact nativeF48B4 admission for P2, with overlay/opcode/table/type/phase guards and canonical P1 preference. Preserve one original F4808 world/combat/movement/contact pass. Scene-phase3 retains lock/action/67 guards; its fallback requires current body71 top contact, live action1 solid, seat-specific sides8/signedpositive carry, valid boxes and original integer top equality/strict horizontal overlap including reflected signedX offsets. No new saved state or carry command patch. Sole approved worker reviewed original2ECB0/3240C/source contracts. |
| Live pair and actual moving support | `grouped-corrected-07854014`: P2x1945 admits native sceneowner2 while P1 stays1712. Native shake/release, passenger return during descent, both current top contacts, native Zero jump/reland relative rise58.734375px while X rides, actual full X charge/release, original parent2128 stop and Zero left control pass. First harness stopped on a newly allocated action0 object before its init; corrected same-approach resume requires active action1/phase5, with no reload. Charged-shot owned travel/contact/expiry remains unverified. |
| Both cameras and world cadence | Both retained Split cameras receive original targetsX1920/Y763..1920, ease during descent/jump and reach currentY1920. Authored vertical bounds stay763..1920. Actual moving stretch advances world2261, native timer2261 and VBlank2261 (10268->12529;12170->14431). Ordinary OpenGL/dynamic rendering/settings; not a solo speed comparison or a long-session watchdog pass. |
| P1 fallen before activation | `survivor-retry-07854014`: original fatal finishes before threshold. Living P2 walks to native admission, releases script and rides actual24px movement; no team retry/outcome change. This segment intentionally ends in the actual wipe rather than duplicating a full living-pair ride. |
| Real wipe/retry and fresh contacts | P2 fatal causes original team wipe/retry2->1, sameCP1 and both settled arrivals. No continuation loader replaces the retry. Native P2 re-admits support, passenger returns, both have fresh current top contacts and carry. |
| P1 death during movement / survivor finish | After that fresh-pair carry, P1 dies during phase5/sub3. P2 continues native movement/tiles/camera to2128/phase8 with top contact, currentY1920/targets763..1920, unchanged remaining retry/outcomes, then native destination jump rises35.2734375px. No script/motion/camera stop forced. |
| Separate unresolved observations | Actual native card entry-only passes from real savedmask11. The earlier full Load/cycle helper unexpectedly returns to title/story; source cause unproved. P2 Start after P1 death does not open a menu despite recovered movement/jump and fade0. Both retained under X5-06/07; do not merge them into unrelated lift passes. |

The reusable lift helper checks explicit original action/phase prerequisites,
coherent bodies, cameras and clocks. It adds only the distinct pre-trigger
survivor, actual retry and during-motion fatal phases needed by original X4
reports. Native Load gains an explicit entry-only mode for setup; such a receipt
never qualifies weapon cycling or guest menus. The failed full check remains.
Recompiler regeneration, max2-job build,19 CTests2.70s, helper syntax and
actual native behavior pass. No production build, current netplay, owner
handoff, remote sync or publication occurred. Watchdog remains unchanged.

Next content is source-derived stage8/0 CP2 diagonal support/contact trigger,
CP3 distinct vertical support and CP4 actual Both door/class37 boss/reward
return. Reuse the first-lift jump/camera checks; do not repeat them for every
support unless a different code path warrants it. Worker recipe:
`subagent-x4-qol/stage8-downstream-content-boss-recipe.md`. Broad routes,
water, temporary/timed supports, four remaining Mavericks/refights/final story,
missing optional boss scaling, current production/netplay and stability remain
open. No full-parity claim.

## Later controllers, survivor pause and actual stage8 boss clear (2026-10-21)

Candidate **C0DF29A5605BCA89D1292BDD97EDD6B756D2BAAE91F6A06B7F5EC2EA60E8BA78**.
Evidence root `G:/Temp/Codex/psxrecomp/x5-parity/qa-stage8-downstream-20261021`.
Ledger **58 passed,1 failed,175 unverified;176 open** of the same234 frozen
reports. F159 is one comparative closure; the other work adds real content
coverage and fixes X5-specific findings without inflating comparison counts.

| Bounded check / actual outcome | Evidence and scope |
| --- | --- |
| First lift to real next checkpoint | Prior07854014 actual support destination, P1fallen/P2alive: native Right reaches authored2320/2080 door, actualCP1->CP2, original carry/close releases body and shared lock. `actual-first-exit`. This is an actual transition, not a CP2-load claim. |
| CP2 native failure and canonical control | On07854014, original class12/sub1 sees P2actualside1 at2597.5 while P1stays2480, but canonical72 stays0/parentphase3; P2 later falls. Same original parent native P1 contact startsphase6. `cp2-baseline`, including corrected pool scan; a wrong first-slot assumption is retained as pre-input harness error. |
| CP3 native failure and canonical control | Same07854014: P2 walks to3637.5/current top contact while P1stays3488; native parent remainsphase4 across24updates. Same P1cross startsphase7. `cp3-baseline`. No trigger/contact/phase write. |
| Correctness boundary | C0DF29A5 substitutes only P2's real previous-contact72 for exactF48F8 and projects only exact admitted motion callbacksF3930/F4A5C plus F4968 admission. Preserve canonical P1 admission preference and one F4808 root/combat/contact update. Per-solid controller override is cleared on native reset/retirement/reuse, hashed and serialized as a bounded Boolean; snapshot provider/schema becomes2. Split camera writes stay with the admitted seat; original world palette/campaign restored. No full-parent duplicate or forced native command/stop. |
| Actual later support motion | `cp3-C0DF29A5`: both actual CP3 arrivals settle; P1stays3488/1370 while native P2walk/top contact admitsphase7 and carries parent1407->1342.5. P2cached current/targetY1200->1135; P1Ylimits/current1200 unchanged. Later cycle/exit unverified. This plus real CP2 admission/usable carry closes F159 bounded; timed expiry/negative carry remain separate. |
| CP2 save/restore and unsuccessful full ride | Actual native gap jump/top contact startsphase6 and carries P2 with P1x2480. Native slot6 save/load both succeed; restoredphase6 and P2-owned movement continue before P1dies. `cp2-native-input-window-C0DF29A5` and same saved-motion replay retain the complete observations, rather than relabeling the failed helper successful. Original28B18 fromF3978 records(2336,2112,F9BE8) twice before P1 loses floor. P2 then continues as survivor until native fatal near3046.5/1649.7 and a low ceiling. No source proof of a co-op crush defect or correct exit maneuver; complete route/ceiling handling remains failed/unverified. |
| Harness limits corrected | First CP2 run rejected one sampled zero71 despite surrounding real carry. Next jump was shortened by measuring from a pre-submission tick; current code measures after input and waits a real current contact window. The subsequent exact-P1-XY assumption fails when P1 loses floor; current check distinguishes originalX from native vertical motion, while hero-loss/full-destination assertions remain strict. All failed receipts remain. One same-state replay investigates the actual failure; no extra game build or loader/character/network matrix. |
| Survivor pause fixed and held locks retained | Native fatal38C08 introduces shared1C=1; existing primary fatal freeze restore omitted it. Preserve old1C only in the existing living-partner branch. `survivor-pause-fixed`: actual ordinary P1death finishes,1C stays0, P2 opens/owns native inventory, resumes and movesRight. `held-native-lock-regression`: original standlock1 survives P1death/fatal completion, then original3D2C4 releases it and P2recovers. No direct shared-lock/control writes. |
| Final Both door / actual boss admission | Original CP4 loader bypasses earlier route; both settle at4000,170/171. Native leading P2 approaches forwarddoor4208/160, P1follows, actual Both acquisition/clearance/close and native dropping arena/intro/warning/HPfill/control return pass. First counter check was the rear door; corrected same approach uses forward position. The first post-close scene0 assertion was too early for nativeintro; same encounter follow-up qualifies readiness. `native-boss-door-corrected`, `native-boss-admission`, then complete chained `native-boss-door-final`. |
| Real final hit, victory and actual Save/return | After the first qualified fight was left running and reached native GameOver, its evidence is retained and owned53356 stopped normally. Same C0DF29A5 boots real savedmask11 card into owned39752; chained entry/door/ready/final-hit/Save avoids another unattended fight. Native boss fills64; one disclosed positive64->1 setup, real X/P2 normal buster gives zeroHP/original death. Both native action25/held poses/departures, real report11->91, earned rewards, overwrite/Save checksum6046 and native selector0404/00/mask91 pass. Card2 unchanged. `native-stage8-clear` and `native-stage8-reward-save-return`. No defeat/reward/exit/save-completion flag write. |

Recompiler regeneration, one max2-job build and19/19 CTests2.53s pass,
including actual snapshot walker mutation/restore of controller ownership.
Python helpers execute natively; source syntax/diff checks apply. No production
build, current netplay, owner handoff or publication is claimed. Default-on
logger/hints and watchdog behavior are retained. The one approved worker gave
concrete source findings for the pause/support fixes, then completed a separate
bounded review of the loaded-weapon failure. That review identifies a missing
deferred weapon5 graphics-owner projection; its link to the observed title
reset still needs a runtime receipt. Main completed the support runtime work.

The newest actual native card is
`native-stage8-reward-save-return/after-native-save-card1.mcd`, SHA256
`348b9e39ca0db00478468836dc30e1c19947addaf9278cc86a222478be552880`.
This is earnedmask91/stages1+5+8 and firstDynamo, not a synthesized progression
fixture. Cold reload of this newest card remains unverified.

Next: preserve the current native selector/card and use one candidate for
stage4/6/7 authored door/boss coverage; keep route/terrain checks distinct.
The original CP2 full route needs a source-informed exit/ceiling maneuver,
not another controller/camera variant. Intro warning/dragon interactions,
water/timed/temporary support mechanics, negative carry, refight defeats,
fortress/story/save branches/endings, missing optional scaling, current
production/netplay and watchdog stability remain open. Full parity is not
claimed.


## Earned weapon upload and native Load recovery (2026-10-21)

Local candidate94AA9338 SHA256
`94aa9338ec037c1fe8de9cf9dab6a19dba7be8ab46197ba2613121e09b5bea56`.
This fixes X5-06 without closing an unrelated frozen X4 comparison.
The ledger remains58 passed,1 failed,175 unverified (176 open).

One approved worker traced the exact original refresh/graphics loader and
immutable X/Zero asset contracts. Main reused existing function/write rings
on C0DF29A5 after a real cold Load of earnedmask91, ordinary released controls
and120 stable native frames. R1 selects1 normally, then selecting5 reproduces
the title return. At native50EE8, actor80094398+38 receives80B05FD8 from Zero's
packed frame word00A003F8. Native17348 calls182D8 with that invalid pointer;
18304 writes zeros over PLAY at800D1C00 in the same frame7228. This is direct
causal evidence, not a speculative timing/reset-chord diagnosis. The complete
failed receipt and original source review are retained.

Exact native class1B/handler50D04 now uses the existing allocation-owner
projection for its entire original update, like the already-owned dash,
buster and CShot deferred effects. All class1B subtypes share this same graphics
consumer; canonical/P1 allocations retain their original resources. No second
native update, synthesized weapon data, reset bypass, extra ownership array or
snapshot schema change is introduced. Registered/configured50D04 is regenerated
through the recompiler. One max2-job build and19/19 CTests2.86s pass.

On the same integrated94AA9338, another real card91 cold Load preserves saved
progress, selects Zero/P1-X/P2 through native menus and enters stage1. The full
earned R1 cycle is0->1->5->8->0. Weapon5's original pointer-write offset is29FC0;
its actual base9F008180 has the immutable original X graphics directory header.
The original upload and effect retirement complete, no18304 write targets PLAY,
and canonical Zero source80105BE0 is restored. Native P2 menu ownership,
screenshot and actual resume pass. Both pads finish neutral. No card, RAM,
weapons, position or progression writes are used by this check.

Evidence under `G:/Temp/Codex/psxrecomp/x5-parity/qa-weapon5-owner-20261021`:
`native-load-baseline`, `cycle-baseline`, `causal-proof.json`,
`native-load-fixed`, `cycle-fixed`, `qualified-cycle.json`, `candidate.json`,
and the regen/build/CTest logs. The reusable
`tools/test_mmx5_coop_earned_cycle_runtime.py` checks native entry identity,
actual earned order, original X weapon5 directory, retirement and owned menu.
The exact archived successful harness and the independently checked resource
qualification remain linked; later stricter source assertions are not reported
as a separate runtime run. Special projectile art/contact/weakness families,
new snapshot-schema cold restore, production/current netplay and broader
campaign/ending qualification remain open.


## Remaining ordinary Maverick flows and named-row batching (2026-10-21)

On the same94AA9338 candidate, one compact checkpoint/door/boss/reward batch
adds actual stages4/6/7. Original content tables establish4/1/CP1,6/1/CP4
(CP3 is byte-identical), and7/0/CP6 (CP7 is an earlier alternate route).
The sole source worker prepared these records/readiness/distinct mechanics;
main used the existing owned native-card process. No extra build, roster,
camera, loader or network variant was introduced.

The frozen comparison count remains58/1/175. These flows broaden actual content
coverage; exact intro-death, distant-view, weakness, secondary-target and other
open comparisons are not closed by an ordinary Maverick final hit. Named open-row
bundles near the top of this document now guide the next sessions directly.
F63 and F118 retain individual partial evidence while staying unverified.

| Current actual encounter | Qualified together | Remaining limits |
| --- | --- | --- |
|4/1 class34, final door4528/80 | P2-alone Both wait with P1behind; native acquisition/open/physical passage/close; original text/warning/filled65/control release; real X/P2 final hit after one positive1HP setup; paired starting/held poses and real departures; report91->99, earned rewards, overwrite/Save checksum6741, selector return | Earlier route, first admission-family door, weakness/attack patterns and native recovery bank are not covered. Composed ready/held-victory captures inspected. |
|6/1 class35, final door3824/144 | Native selector entry then authored CP4 setup; actual P2-led Both wait/passage/close; original warning/text/filled56/released controls/live target; real X/P2 nonlethal56->55, native actor94 recovery30->0/damage-table restoration, fresh follow-up55->54; one positive54->1 setup, actual buster zeroHP/original death/initialized report99->B9; real rewards/Save checksum6007/story/selector | Exact composed paired victory/held poses were missed during the sample/setup race and remain unverified. Native paired scene/companion counters each advance once, but are not a visual substitute. Actual subtype1/8HP boss-emitted target exists; P2 damage to that separate body remains F118 unverified. |
|7/0 class33, final door8016/1024 | Native selector then actual CP6 setup; P2-alone Both wait/passage/close; ordinary long native intro/text/fill59/8C1/control release; original phase9 class23/sub1 attack child allocates with parent resources, advances and appears in both composed views; actual X/P2 buster finish after positive1HP setup; paired starting/held poses/departures; reportB9->F9, real rewards/Save checksum6744, selector return | Attack-child damage interaction, lower-HP aggression history and broad attack patterns are unverified. Native initialization/child emission is not a secondary-target or distant intro-rendering pass. Composed child/held-victory captures inspected. |

All eight ordinary Maverick defeat/report flows now have bounded evidence on
named candidates: previous1/2/3/5/8 plus current4/6/7. This is not all eight
on one current campaign/candidate, every stage route, refight or ending parity.
The current actual card has F9 (earned1/4/5/6/7/8), SHA256
`d2c35553866e2609bb756eedabc4e1a6f85f1fbf1b568beee29d02a7fc74ccbf`.
Its newest cold reload remains unverified. The native selector is04040000,
maskF9; both pads finish explicitly neutral and there is no active helper.

Retained unsuccessful harness observations remain under
`qa-remaining-mavericks-20261021`: frontend fixture11 cannot acknowledge until
native gameplay (corrected by original selector entry); requiring transient
phase4 missed the completed stage6 intro (same encounter independently qualified
from maxHP/live target/release); floor-level shots miss the flying boss and its
periodically disabled target box (position-only native muzzle alignment fixes
setup); an already-fired shot kills it between sample and teleport, so victory
correctly refuses the fixture. The last actual zeroHP/death/report proof is
continued on that same encounter, not relabeled as a successful full helper or
replayed. The reusable final-hit helper now recognizes that observed race.
No game lock, phase, vulnerability, defeat, reward or save-completion flag was set.

Receipts: `qa-remaining-mavericks-20261021/qualification.json`, each stage's
entry/clear/reward reports, `stage6-native-admission-qualified`,
`stage6-recovery-aligned`, `stage6-clear-qualified`, and `stage7-native-attack`.
Helpers: `test_mmx5_coop_maverick_entry_runtime.py`,
`test_mmx5_coop_boss_interaction_runtime.py`, existing final-hit/reward helpers.
Python syntax and Git diff checks pass; no compiled source changed after the
19/19 CTests2.86s candidate build. Default-on diagnostics was read directly:
83219 native rows,107669 accepted co-op controller rows and64 explicit developer
fixture records at the checkpoint. The native QA stream is marked as containing
fixtures and is not claimed input-only replayable. Logger/hint defaults and
watchdog behavior are retained; current production/netplay/owner acceptance
remain unverified. Everything remains local.


## Actual second mandatory Dynamo gate (2026-10-21)

Same94AA9338 candidate, no build/config/roster matrix. Current earned F9
selector cursor8 and native PLAY10F4 choose actual raw10/0 class3B/sub1,
with the original character confirmation/title/READY/intro. Both heroes finish
arrival and native control release; actual dynamic HP fills68. One disclosed
positive68->1 setup is followed by a real X/P2 normal-buster hit to native HP0
and original retreat action2. Ready text uses the actual scene owner. Original
retreat/child retirement reaches initialized report12/raw15/0/oldstage10,
then native P1 rewards/occupied-slot Save checksum6289 and actual free selector
cursor0, unchanged MaverickmaskF9 and Dynamo progress5. Progress4->5 advances
at launch and is not used as defeat proof. No success or scene flag was seeded.

`qa-native-dynamo2-20261021/{encounter,reward-save-return}/report.json` and
`qualification.json` retain the exact input, actor, text, native report and
card evidence. The actual saved card SHA256 is
`1e1a1b68dcf287d70b52a051163cacfb40e33d24496935325bebf4e850be28d1`.
The same94AA9338 candidate cold-loads an unchanged private copy through the
actual native Load, original Zero selection and stage1 entry. Both acquired
masks remainF9 and native Dynamo progress remains5; see
`qa-special-bundle-20261021/native-load/report.json` and
`native-dynamo2-cold-load.json`. Native
first/second Dynamo branches now have named-candidate evidence; broad attack
patterns remain separate. The reusable
`tools/test_mmx5_coop_dynamo_runtime.py` follows the actual native mandatory
selector branch and never substitutes launch-progress for defeat evidence.
The comparison ledger remains58/1/175; this was a real pending prerequisite,
not a new comparative closure.

## Named-row intro bundle: setup limit (2026-10-21)

The planned F41/F54/F66 and death-presentation bundle remains unverified.
Fresh native New Game/Zero reaches the intro, but the CP2 lower-camera plus
paired rooftop position setup does not admit the upper controller and later
drifts off the intended floor. Full native CP3 loading and an actual Game Over
Continue each lose the primary during incoming arrival before free control.
None reaches native WARNING/class31; no scene, death-art or final-hit pass is
inferred. Evidence is retained in `qa-intro-bundle-20261021`, including the
initial roof receipt's explicit `disposition.json` correction.

The bounded source review finds no proved CP3 table/loader-order defect.
Original CP3 uses the incoming beam, not immediate on-foot control. The paired
teleport also suppresses native camera-edge placement admission by setting
current and previous origins equal; the placement scan is not proved to be
background tile streaming. No speculative production change follows. One
future debugger observation at the actual checkpoint/initial placement/arrival
boundaries is recorded in `subagent-x4-qol/intro-cp3-loader-setup-disposition.md`.
Further intro setup variants stop here; the stable earned-special batch can
check F50/F59 together, with F174 excluded because weapon8 is a rising tornado,
not the guided beam family. Counts remain58/1/175.

## Earned special bundle: two comparative closures (2026-10-21)

One shared stage1 setup on unchanged94AA9338 checks F50/F59 together. The
actual second-Dynamo card is cold-loaded natively; earned F9 and Zero/P1-X/P2
remain intact. Native R1 selection8 and P2-owned menu/resume precede two
original Triangle-primary attacks: clear-space presentation/expiry, then real
native-enemy contact. Square in this profile charges the secondary buster;
its earlier non-launch is retained as a setup failure, not a tornado defect.
No ammo, request, animation, target HP/contact or success flag is written.
Position/history and HP-only protection bypass traversal; P2 incoming contact
protection applies only while positioning for the separate outgoing-hit check.

| Overlapping observation | Decisive evidence / disposition |
| --- | --- |
| Native input/private resources | Actual request8; P2 ammo288->276->264, P1 ammo unchanged. Original primary SHOTS parent is PLAYER inside its P2 context; children point to its private primary80098120. |
| Appearance/motion/real contact | Original normal8 rising primary and trailing children draw in both composed panes. Actual native class0E target loses16->13HP; no target spawn/HP/success write. Primary/children retire, native98/99 return to baseline and P2 resumes Right. Together with prior distinct CShot/FX5 evidence, F50 passes bounded for the representative ownership failure class. |
| Exact original pixels | Live graphics table48B, assembly224B, compressed atlas1194B and selected7801 palette match immutable DAT88. The actual composed band matches original native64x16 frame5/facing64: all1300 opaque2x samples per pane have zero channel error. F59 passes bounded. Full transparency/overlapping bands and broader families/churn are not inferred. |
| Guided/homing beam | F174 remains unverified. Normal8 is a segmented rising tornado, not that family; no amount of tornado variants substitutes for the actual beam check. |

`qa-special-bundle-20261021/qualification.json`,
`normal8-native-primary/{report,original-pixel-review}.json`, composed PNGs and
three retained GPU/present-ring frames preserve the evidence. The original
extractor and twelve source frames live under
`subagent-x4-qol/original-normal-weapon8`; the pixel-review script is retained
beside the qualification receipt. The later composed capture/source reads are
explicitly not atomic with earlier context/GPU observations. No frame is
assigned from a different tick; the pixel review searches the twelve original
authored frames using the actual inherited facing.

The existing diagnostic body copy omits private SHOTS. A QA-only Windows
read-only observer uses exact executable symbols and owned-module identity,
equal private-context copies, unchanged host world tick and idle projection
guards. It adds no instrumentation/build and never writes process memory.
The first socket-bracket observer fails because round trips span native world
updates; both unsuccessful receipts remain retained. The corrected direct
observer qualifies the actual attack. Normal debug19/19 CTests remain the
unchanged candidate's build evidence; these additions change only QA tools
and documentation. Production/current netplay and charged/armor/all-special
families remain open. The ledger is60 passed,1 failed,173 unverified;174 open.

## Actual refight roundtrip: five comparisons together (2026-10-21)

Candidate84EDFD5A closes legacy21/F47/F156/F165/F206 on one P2-led Grizzly
refight roundtrip. Actual earned F9/second-Dynamo card Load and original Zero
selection are retained; original hub12/0/CP1 and approach-position setup bypass
the preceding fortress route. No admission, boss defeat, return, pad-completion
or reward flag is written. Native pad2 collision starts the actual stand14,
departure16, negative exit/mode9/CP2 loader, both arrivals and native release.
HP-only protection supports combat setup; it is disabled after actual bossHP0
and before the original hub return. Contact protection is not held over native
hurt recovery. Boss HP is lowered once to positive1, never directly to0.

| Shared encounter evidence | Individual comparative disposition |
| --- | --- |
| P1 remains at hub3408 while P2 approaches pad3584; one original P2 handoff; native loader and both grounded arrivals; actual P2Right19.5px and P1Right11.5px | legacy21 and F165 pass bounded for complete safe admission/release and native controls, adding the feet/movement proof missing from the prior eight-pad sweep. |
| Both private current/target camera bounds remain X464..688/Y16 after release, and both current origins follow actual independent motion | F206 passes bounded. FC838's temporary688 lock is intentionally released by original FB49C restoring min464. The earlier source recipe and fixed688 assertion were incorrect; their correction is retained, not classified as a game failure. |
| Native CShot spends only P2 ammo288->282; original cycle returns to buster; actual buster hit causes GrizzlyHP0/action2, original child/death retirement and automatic CP2->11 hub loading | This is an actual X5 refight defeat/return, not an ordinary reward report or a newly clickable return portal. Native pad2 progress0->1->2 disables activation but retains its support collision. |
| Both native feet1576 on actual hubpadtop1576, own top-contact histories, live pad72=8/carry76=1, later P2Right3px/P1Left7.5px and composed bodies/HUD | F47/F156 pass bounded. Five of eight fresh coherent samples show both exact feet and positive current support. The first roundtrip return row preceded a separately queried grounding predicate; actual movement/visual evidence remains valid, and actual-hub-contact-audit.json supplies the corrected coherent contact proof before any refill/reload. The future harness now qualifies the captured row itself. |
| Actual return preserves P1 42HP/288ammo and P2 32HP/actually spent282ammo, plus both F9 inventories | Native/private continuation ownership passes after the cross-seat copy correction. No healing during return hides a bad landing. |

The baseline94AA9338 roundtrip finds two actual failures: P2 remains JOINING
atY1563 while P1 is supported at padtop1576, and P1 inherits P2's32HP/282ammo.
Restoring P1's singleton continuation cache fixes resource ownership onB257D10B,
but support qualification at the early player boundary still cannot see the
later original solid contact. Final84EDFD5A reuses the existing strict dynamic
support predicate at the end of the original world update. It advances neither
the beam animation nor world clocks again, adds no new saved ownership state,
and retains scene/mount/terrain restrictions. The watchdog is unchanged.

The full receipts and candidate hashes are under
`G:/Temp/Codex/psxrecomp/x5-parity/qa-refight-bundle-20261021`:
`late-entry-safe-anchor/report.json`, `late-roundtrip-safe-anchor/report.json`,
`actual-hub-contact-audit.json`, `late-support-candidate.json` and
`qualification.json`. Original failures, native source/camera corrections and
the superseded intermediate receipts remain retained. The generic boss-80px
approach could enter the lower room hazard; the qualified finisher instead
uses the measured native entry ledge. An attempted CShot final hit spent ammo
without damaging the boss in that setup; the qualified defeat uses the proven
native buster. No weapon-matchup or additional rendering variant follows.

Debug build and19/19 CTests pass1.36s. The same84EDFD5A passes the existing
moving-owner terrain return with exactly one incoming start,223.5px P1 motion,
retained P2HP17/ammo73 and no reload. Controlled native continuations retain
different HP/ammo for alive, absent and fallen P2, without resurrection or
extra retry. The original stage2/section1 -> stage3 water-opening setup also
passes four actual native jump/rise/landing probes, two heroes at each entry,
with no party loss. The auditor's broad mechanic categories remain unverified;
these are the same bounded arrival checks as the original crush regression.
Current production/netplay, seven other refights, owner-death
timing F103, fortress routes/endings and manual Select admission on dynamic
supports remain unverified. The ledger is65 passed,1 failed,168 unverified;
169 open out of the frozen234. This is one shared encounter with individually
bounded evidence, not five independent encounter variants.


## Actual pad-owner fatal: one closure, overlapping checks bounded (2026-10-21)

Unchanged84EDFD5A closes F103 on actual hubpad2. Native card Load and original
hub12/0/CP1 loader are retained; collider-derived position/history bypasses the
route. HP-only setup protection is disabled before seeding P1HP13/ammo121 and
P2HP17/ammo73; contact protection stays off. The actual P2 stand14 hides living
P1 (scene2/3,body4=3/draw0) before fatalHP is seeded. No scene/death-action,
ownership, completion, boss or release flag is written.

Exactly one recovery transfers the committed pad script to P1. Original
departure/checkpoint2/mode9 loading, actual Grizzly intro/full127HP and native
release finish. P1 retains13HP, full32B ammo and retries, then moves2.5px on
native Right; P2 staysFALLEN. Fatal/recovered composed images were inspected.
There are27244 private-render verify checks with zero mismatch, watchdog,
device read or VRAM leak; the intermittent watchdog issue remains unresolved.

The first movement assertion hit CP2's brief unlocked frame before the next
native stand14. Its receipt is retained. The final helper requires actual
intro/fullHP/release before probing input: corrected observation timing, not
a game fix or configuration variant. No build or CTest repeat was needed.

F4/legacy3/F133 stay unverified:99 asynchronous pool samples contain no active
death particles and the enabled native allocation trace records no calls
before the room change. That does not qualify particle color/cleanup or an
authored dying pose. Prior ordinary-fatal evidence remains partial. The sole
approved source worker supplied original X body/static-particle indices,
UVs/palettes/scripts/hashes under subagent-x4-qol/original-x-death; all13
index/alpha references validate. Transport can intentionally hide a body;
native radial scripts loop/cull without fixed age deletion. Initial7802 is
black placeholders, not evidence of displayed live colors.

Full evidence is under
G:/Temp/Codex/psxrecomp/x5-parity/qa-pad-owner-death-20261021:
qualification.json, actual-pad-fatal-native-intro/report.json and
final-owned-stop.json. The source card is unchanged. Owned game/worker are
stopped. Debug quit returned a delayed busy response and the process then
disappeared; no forced termination or observed exit code is claimed.
No production/current-netplay, other refight/death paths, full-route/ending
or new owner-payload claim. Frozen234 ledger: **66 passed,1 failed,
167 unverified;168 open**.


## Actual capsule bundle: two comparisons and overlapping partial checks (2026-10-21)

Candidate9FFB6E38 (SHA2569ffb6e380ebe4908bd8561e068fe01c32f4f59a82b406a178f874df3bb88b903)
closes F154/F167 together. Debug19/19 CTests pass1.50s. Scope remains234 frozen
X4 rows: **68 passed,1 failed,165 unverified;166 open**. One original capsule
setup supplies two actual native grants through a genuine pregrant host restore,
rather than two route/roster/camera/loading variants.

| Shared setup/observation | Individual outcome and limits |
| --- | --- |
| Native Zero/P1-X/P2 Load of earnedF9 card, stage8/0/CP2, original placement87/sub5 at2440/1584 | Native loader and small original2B070 rescan bypass the preceding route and viewport-edge admission. Original actor initializes at2440/1620; actual collider/top determines P2 placement. No actor, phase, contact, program or completion flag is manufactured. The first guessed pair teleport fails setup; its receipt remains, and no route pass follows. |
| X/P2 enters native proximity, talks to Dr.Light, uses actual capsule contact and gains original program40 | F154 passes bounded. Native admission executes one proximity/one top-contact handoff per case. All69472 private message bytes match original DAT88; script3F is654B/SHA37ced8314ea217bfa55df30db0deeecac01403d10d9f6915ae4bb92cd0672ac5. Eight native ready-page Cross acknowledgements, original grant and normal return complete. Shared canonical Zero message root801515CC remains unchanged. Actual composed text/Light/capsule/both return bodies are inspected. Other capsule variants and deliberate queued contention remain open. |
| Save6 during first native text page, real Cross to next page, actual load6 | Original cursor/private bank restores exactly; native remaining conversation/grant/return complete. Immutable messages are covered by existing complete guest-mod memory snapshots; provider/schema3 validates their addresses/size/hash without adding transient recipient ownership. This is actual warm mid-text restore, not cold-host snapshot or rewind qualification. |
| Normal return then actual load5 of genuine pregrant state; real second grant40; native fatalHP during original command18/F5=4 return | F167 passes bounded: exactly one owner recovery, real phase11/program retained, P2FALLEN, P1HP13/full32B ammo/retries retained, actual grounded return/native Right9.5px. Normal controls separately move P1Right9.5 and P2Left7.5px. Only setup/fatal HP is cheated; no reward/scene/release flag is set. |
| Actual capsule program retained into stage2 native final hit, native report/Save/selector, then a fresh process cold native Load | B1E5473E writes genuine40/clearFB card(checksum6429,SHA b4c4bbeccccd72af7cb870339a730e58242840b794b39185f945e8b19c37b9bf). Final9FFB6E38 reads the actual copied card and retains40/FB/Zero-P1-X-P2. No custom card writer or progression flag setup; this native-card evidence does not establish cross-build simulation/netplay compatibility. |
| One program plus normal/fatal capsule visibility | F55/F52 remain partial. Original X5 evaluates completeF0/0F program sets after native reports before adding armor availability; one program is not an equipped-art/ability pass. Distinct intro/water boundaries remain open. |
| Same actual fatal supplies original death trace and17 asynchronous pool samples | Legacy3/F4/F133 remain partial.152 original/wrapper entries form76 equal adjacent pairs:44 random/32 radial native attempts. Distinct calls with identical arguments are retained. Original pose/color, complete cleanup/vehicle paths and generic slowdown remain open. No further death setup variant is added. |

Native source and original immutable message-table matching confirm all three
actual findings before their fixes: P2 singleton admission, committed-return
owner fatal, and wrong recipient text. CB9A1E55 proves P2 admission failure with
native P1 control; D4255625 passes normal grant but fatal return strands the
survivor at command18/F5=0. B1E5473E fixes that fatal path; source/live matching
then proves its Zero-message variant. Final9FFB scopes the recipient at native
2385C initialization/replay; original deferred pages retain their bound cursor.
Campaign story/global resources and authored prose are preserved. Original X5
capsules accept both heroes; no X-only admission policy is introduced.

The reused native rescan is offline debug setup only, bounded to512x480 and
native placement guards. It does not broaden ordinary supplementary spawning.
All snapshots/cards/captures/build archives are ignored/local onG:, owner
payload unchanged. Evidence root:
G:/Temp/Codex/psxrecomp/x5-parity/qa-capsule-bundle-20261021/qualification.json,
with recipient-text-qualified/report.json, program-persistence-boss/report.json,
program-native-save/report.json, earned-program-cold-load/report.json,
native-program-persistence.json and death-overlap-review.json.

Private-render verification records1094 checks with zero mismatch/watchdog,
device-read or VRAM-leak counters in this bounded run. Existing intermittent
watchdog remains unresolved and unchanged. All owned games are stopped; debug
quit returned busy before each process disappeared, with no force or observed
exit code. The approved source worker is finished. Current production/netplay,
other capsules, full routes/armor/endings and remaining open rows are not
qualified by this milestone. Next shared setup is the named Chaser bundle;
reuse its admission/dash/draw/death/section observations before changing content.

## Shared Chaser presentation/death/retry: four comparisons (2026-10-21)

Unchanged9FFB6E38 closes legacy16/F15/F188/F11 together. No product source
change, new build, CTest repeat or new instrumentation variant. Prior19/19
CTest1.50s remains that exact candidate's build evidence. The234 frozen
comparisons now total **72 passed,1 failed,161 unverified;162 open**.

| Reused native observation | Comparative outcome |
| --- | --- |
| Actual native card Load, original stage5/0/CP0 arrival, both original car modes0->grounded1 and one live CAR+A0 parent8009EB90 | One shared class7/sub0 READY panel8009F9A0 and one composed READY per pane. Old-stage UI while loading is excluded from the car-parent count. Genuine native release snapshot7 is retained; a replay supplies the bounded presentation window. |
| Original READY children advance and all retire; no class7 children after260 native updates; clear composed screen | Legacy16 passes bounded. Original type1B controller legitimately persists; its completion16/count14 alone is not disappearance. The actual composed READY is gone and does not restart in the inspected window. No UI phase/counter/draw bytes are changed. |
| Native P2 Circle+Right, clear original mounted boost/car animation | F15/F188 pass bounded: P2 mode5/boost, vx0->3.9375, actual X frameA2->AE/carframe0->3; grounded neutral P1vx0 in all four coherent samples/no boost. Composed dash is visible after READY, unlike earlier obscured captures. Native camera remains1108, separating own acceleration from authored camera carry. No velocity/pose/boost flag is written. |
| Measured native overlap, actual P2 boost, composed cars/riders | F16 remains partial. Body/car sort words0500/00B0 agree between seats and clear overlap is retained, but exact exhaust interleaving is not established by positions, bytes or these frames. |
| Native X/P2 car fatal,140 native cleanup updates, victim car active/draw/action bytes0 and body draw0/state3/lifeFALLEN, surviving Zero/car visible | F11 passes bounded with no lingering beam/phantom vehicle in either composed pane. Shared retries stay2; real survivor Cross produces native rise/land. F133 gains a distinct vehicle cleanup observation but original full pose/color contracts remain open. |
| Genuine surviving-P1 snapshot9, native P1 fatal, actual original retry/loader/dual entrance | Exactly one retry2->1; both fresh native cars are grounded with correct HP42/32, then both actual native Cross inputs rise and land. No team retry, scene, arrival or death-action success flag is seeded. |
| Same original entrance/controller evidence plus stationary presentation | F93 remains partial: no original full entrance-speed baseline is inferred from host elapsed time or a camera freeze. |
| One late car-position handoff attempt | Legacy18 remains unverified: native car is clamped back to opening, camera1114..1420, CAMERA51 always0, no native phase2; living rider falls before the actual endpoint. This is a setup failure, not a native handoff failure. The source confirms road/factory remain raw5/0; native CP2 loading would not prove this boundary. Further approach variants are stopped this milestone. |

The first attempted stationary setup used a bad source label: CAMERA+32 is
the original horizontal follow upper threshold320, not scrolling speed.
CAMERA+47 is signed bound/origin interpolation step6. Correct original readers
2A720 and2A0A0/2A184/2A278/2A36C are retained under
subagent-x4-qol/chaser-bundle-20261021/camera32-correction.md. The earlier
source recipe explicitly carries the correction. Failed guard/follow-field
attempts and native GameOver fixture timeouts remain QA failures; none is
promoted to a game defect. Only one source-supported stationary trial follows.

The qualified trial restores original32=320, freezes interpolation47=0 on the
three actual camera layers, and changes only car/rider position histories to
the measured original native release positions.40 native updates prove healthy
grounded cars and actual stationary camera before presentation is checked.
This bypasses road traversal/scrolling; it does not seed READY completion,
native animation, velocity, contacts, death, retry or progression. Actual
host snapshot8 retains the completed native READY/grounded setup for overlap
and fatal without repeating entrance variants. Actual team retry restores
normal authored camera behavior; the same bounded interpolation setup is
applied after its real control release for input observations.

Evidence root:
G:/Temp/Codex/psxrecomp/x5-parity/qa-chaser-bundle-20261021/qualification.json.
stationary-from-release/report.json supplies presentation/dash/fatal;
survivor-and-handoff/report.json has survivor_jump_landed=true but the whole
handoff driver fails setup; actual-team-retry/report.json and
post-retry-controls.json supply the real retry and both native controls.
Original executed harnesses, raw observations and composed images are retained.
No all-green helper status substitutes for the individual observations above.

The same owned run records14218 private-render checks with zero mismatch,
watchdog, device-read or VRAM-leak counters across its bounded cases and genuine
restores. This does not resolve the existing intermittent watchdog. Owned games
and the sole approved source worker are stopped. Quit returned busy, then the
process disappeared; no force or observed exit code is claimed. Owner payload
unchanged; no remote publication or tracker sync. Next named shared mechanic
is an actually earned boss weakness/accepted-hit/recontact bundle; leave the
remaining Chaser obligations explicit instead of generating further variants.

## Earned weakness and guided owner; deferred reproductions (2026-10-21)

Unchanged9FFB6E38 closes F26/F129/F174 and reopens F33. Net72->74 passed:
**74 passed,2 failed,158 unverified;160 open of234**. F181's missing scaling
contract remains the other failed row. No game-source/executable/instrumentation
change or rebuild; prior exact-candidate19/19 CTests1.50s remains build evidence.

| Shared observation | Disposition |
| --- | --- |
| Genuine F9 card, native Zero Load, original stage2/1/CP2 door/intro/fullHP62 | New entry-only QA mode preserves boss HP/phase and parks after native controls. Preceding traversal is bypassed. Stage2/class39 is Dark Necrobat/Dark Dizzy, not Duff McWhalen. |
| Ordinary CShot control and actual earned F-Laser6 | F26 passes: ordinary CShot62->60/result2 is not the weakness. F-Laser retains a miss62->62, real62->57/result7/native32188 damage5 and phase0A, original recovery, then fresh57->52/result7. P1 ammo unchanged; no boss-state writes. |
| Actual recovery rejection | F129 passes: native3225C sets shot98, result7F and three nonzero P2 contact results occur while HP57 stays57; no accepted HP store. p2-hits counts contact results, not damage. Last-result can be stale after a miss. |
| P2-only Up then Right; original primary/attached/trail ownership | F174 passes closest manually guided X5 analogue: actual primary velocity/direction responds to P2 while P1 is neutral; X pose2A/selected6, child parent80098120 and exact original table/assembly retained. Composed colored primary and moving trail visible in both panes; native boss contact/control separately qualify above. No X4 automatic target-acquisition claim. |
| Source/composed art review | Original primary opaque488/576 and authored black-trail mask704/704 samples match per pane; frame18 is authored blank. No wholly missing primary effect. Asynchronous frame/occlusion and missing exact live palette/packet limit full-pixel/color-animation claims. No speculative art patch/capture matrix. |
| Earlier intercepted follow-up | Position-only cheat near left boundary4504.39/306.15 causes actual P2 fatal/life4 with pluginfailed0. Preserved as setup failure; safe-interior native-entry retry qualifies the attack. |
| Original pad2/CP2 and normal7 preparation | F130/F131 stay unverified/deferred: no ball fired. First approach predicate misses; next prep stalls in ordinary hurt action11. Source proves fixture12 pins61=2 while native hurt initializes100 and exits exactly60; this is QA interference, not ice/capture failure. HP-only attempt still misses bounded approach. Future needs genuine pre-hurt entry/decisive positioning; no extra variants this milestone. |
| Actual snapshots | F33 reopens: genuine save5 succeeded, save6 last_ok0/generation2, load5 last_ok0/generation3; flushed log says cannot capture rollback. Cause unverified; preserve original state and defer diagnosis. No provider bypass, forced restore or snapshot parity claim. |

Evidence:
G:/Temp/Codex/psxrecomp/x5-parity/qa-weakness-bundle-20261021/qualification.json.
This indexes ordinary-cshot-control, normal6-guidance-qualified,
normal6-accepted-rejected, safe-normal6-contact, native-pad2-for-spikeball and
all three normal7 preparation receipts. Helper-level failures stay failures;
named subchecks are qualified individually. Source receipts are under
subagent-x4-qol/{weakness-bundle-20261021,normal6-art-20261021,
grizzly-native-freeze-20261021}.

Owner now prioritizes breadth before depth. Defer F130/F131 and snapshot
diagnosis with concrete receipts/prerequisites; select a new content/mechanic
bucket from actual tables and named open rows. Overlap P2 triggers, collision,
spawning, camera/background and actual exits where they share a location.
No roster/camera/loading/network matrix. Source worker stopped; owned43376
quit normally with observed exit0, no force. Watchdog and owner payload unchanged.
Local only: no publication or tracker synchronization. Full parity remains open.

## Breadth: shared menus, two placement families and visible scenery (2026-10-21)

Unchanged9FFB6E38 completes three bounded comparisons and exposes one real
presentation failure: **77 passed,3 failed,154 unverified;157 open of234**.
No game source, executable, instrumentation or build change; prior
19/19 CTests1.50s remains exact-candidate build evidence.

| Named check | Evidence and disposition |
| --- | --- |
| F42 shared-frame/private-view recovery | Native P1 and P2 menus with distinct camera origins: world clock frozen while native frames advance, zero pair commits during menu, own private HUD/camera returns and pair commits resume. Canvas1280x720 remains stable; composed original menu duplicates into both panes. Local natural shared boundary only; forced/load-induced refusals and current peers remain open. |
| legacy13 ordinary P2 placement | Fresh original raw1CP0 proves target800FBEFC/class0E absent first; P1 rectangle[-48,368] excludes528, actual P2[144,560] includes it. Original2B070 calls, live actor origin+10, initialized HP16 and actual P2 buster16->13 pass. |
| F80 distinct later-content placement | Raw17CP0 original member27 record800FC2A4/class2A: pre-absence/init32, P1 rectangle[-48,368] excludes496, P2[112,528] includes it, actual native jump/buster32->29. Numeric authored later-content analogue; no guessed stage name or native campaign/route-reachability claim. |
| Ground/setup correction | Initial raw1 attack relocationY400 is below observed native floor379; raw17 initial Y2160 is unsafe at advanced X400. Native fatals/life4 with pluginfailed0 are retained. One correction uses observed floorY379/2331 and native attacks/jump; both succeed without actorHP, placement, hit or success writes. No contact-timer protection. |
| F175 native CPU clocks | Four original565BC/56604 controllers:138 countdown writes each over137 bracketed updates, one natural105-update loop and three42-update loops. Native source/destination color rows change. This proves clocks, not presentation. |
| F175 actual visible cycle | Failed. Seven composed images across92 native updates: stationary living heroes, P1 background has5 distinct states and returns to first; P2 same fixed background crop is byte-identical in all7. The extra bounded cycle rules out the first two snapshots' possible phase alias. No palette/clock/phase writes. Green RAM/VRAM restoration checks do not establish scenery fidelity. Cause/repair deferred. |
| F18 fades | Both-owner native transitions and29 nearby captures retained after correcting the original separated/off-camera/hurt setup. Continuous transient pop/current Quit-page suppression not established; remains unverified rather than a pass from retained flags. |
| F144 heavy-effect/HUD | Auxiliary normal8 primary was not observed while X/P2 was in native hurt11; defer it and continue independent menu checks. |
| F95 sky gaps | Actual black rectangular sky blocks before menus and afterward are retained. Original intended completeness/cause not established; no menu-causality or wrapping pass. |
| F107/F109/F200 | No decisive special-predicate/scripted enemy-group source found within bound. Ordinary0E/2A admissions and emitted projectiles cannot substitute; defer. F28/F78/F189 retain stricter room/progress/layer-origin obligations. |

Receipts:
G:/Temp/Codex/psxrecomp/x5-parity/qa-breadth-menu-world-20261021/qualification.json.
The initial aggregate helper completes its sweep but has failed subcases; use
individual statuses, not its top-level success. Corrected placement helper
passes both cases. P1 menu helper's later exact passive-X assertion was invalid
during native P2 hurt/knockback; completed menu evidence remains separately
qualified. The guest helper and nearby transition helper complete normally.
Nearby helper retains an old descriptive400/256 bypass label; actual executed
position176/2331 and observed native feet are authoritative.

Source-only shortlist:
subagent-x4-qol/breadth-world-20261021/README.md and original excerpts/hashes.
Source recipes alone close no comparisons. Last sampled restoration evidence
has28274 checks with zero mismatch/watchdog/device reads/VRAM leaks, alongside
the visible F175 failure. Prior baseline watchdog remains unresolved/unchanged.

Owner breadth-first cutoff remains active: no further setup variants or
renderer investigation here. Preserve these failures/prerequisites and move to
new mechanic/content buckets. Quit is requested for owned52780, debug endpoint
closed; owned-stop.json records actual final process state. No force/restart,
no owner payload changes, publication or remote tracker synchronization.

## Existing evidence and elective option breadth audit (2026-10-21)

Evidence: `G:/Temp/Codex/psxrecomp/x5-parity/qa-comparison-evidence-audit-20261021/qualification.json`
and its reproducible read-only `audit.py`. The47-file inventory hashes original
runtime reports, composed captures, source contracts and frozen X4 copies.
Current game candidate stays9FFB6E38; this milestone performs **zero new game
runs/builds**. Existing19/19 CTests1.50s remain prior candidate evidence.
Ledger **80 passed,6 failed,148 unverified;154 open of234**.

| Individual comparisons | Evidence and disposition |
| --- | --- |
| F5 / P2 advances into a boss encounter with P1 behind | Passed bounded84EDFD5A: original P2 pad2 contactX3584/P1X3408, sceneowner2, exactly one handoff, native CP2 loader/Grizzly full127HP intro/release. Both grounded/living/visible and each native motion follows. Meaningful refight encounter analogue; no intro/other-door/current-candidate rerun claim. |
| F124 / extra walk after a refight beam | Passed on the same original encounter: entry loop supplies only native text Cross acknowledgements after activation, no extra directional approach/teleport. Native class36 active127HP and both controls occur before the later attack setup. Reuses late-entry-safe-anchor/late-roundtrip-safe-anchor; other refights remain open. |
| F134 / partner teleports out and back after vehicle-stage final hit | Passed bounded9796C215 on X5's original on-foot class32 Chaser-stage boss. Real P2 final hit1->0 after one53->1 positiveHP setup; both actual/held action19h victories, no hidden partner during celebration.103 retained transition samples show one expected final departure with no P2 reappearance before report; actual rewards/Save/selector follow. Road/vehicle handoff is bypassed and remains legacy18; no continuous every-frame claim. |
| F37 / simultaneous enhanced heroes | Failed feature contract: elective automatic enhancements/variant projection absent. Native X5 special capsule8 proves meaningful X/Zero enhancement branches. Exact title-code/stat/native-save semantics remain untraced; X4's cosmetic Black Zero contract must not be copied blindly. |
| F63 / elective boss scaling | Failed feature contract: no option/native scaling or recovery integration. Existing arithmetic helper and unscaled class35 recovery receipts do not implement the feature. |
| F99 / shared versus per-player recovery | Failed feature contract: no dropdown/timer integration. Final frozen X4 has scalingoff/per_player default; BOTH policies halve ordinary recovery rounded up. Historical full-duration wording is superseded by F152/F153. Phase-protection timers must retain native semantics. |
| F12 / exact dash colors | Existing3FE6E7DC composed X/P2 and Zero/P2 dash captures inspected; own original assemblies and expiry retained. Exact original/live palette comparison is still absent, so remain unverified rather than closing from identity alone. |
| F22/F56 / weapon as a solid support | Approved worker's bounded native selected/charged/armor path audit finds no established support analogue but not exhaustive absence. Dispatch beyond0..2F and indirect/out-of-span/terrain-emission coverage remain incomplete. Both unverified, not inapplicable; no additional source variants this milestone. |
| F76 / discoverable options | Generic catalog/UI source selects settings on checkbox; actual launcher discovery, PLAY persistence and runtime behavior remain unverified. Missing controls above are separate feature failures. |
| F92/F98/F100 / Armor remainder | Earlier native mount/move/voluntary dismount and survivor controls remain valid partial evidence. Reports retain no distinct nonfull ammo/resources or grounded post-dismount check; composed images cover stationary mount, not moving layering. Fatal was after voluntary dismount, not forced mounted transfer. Keep all three open and target only these missing observations in a future shared session. |

Exact option contracts/source hashes live in
`subagent-x4-qol/option-contracts-breadth-20261021`; weapon-content limits in
`subagent-x4-qol/weapon-support-breadth-20261021`. Frozen reference stays through
Playtest42. The sole approved worker completed both bounded source tasks and
stopped; no children or overlapping runtime work.

Owner policy2026-10-21: save states are build-specific disposable snapshots.
No backward compatibility/migration is required. Same-build save/load, rewind,
rollback and safe invalid-input rejection still matter. F33's actual same-build
save/rollback-capture refusal remains failed; native memory-card saves are a
separate preserved contract.

The previous hidden/muted owned PID52780 stayed CPU-idle at1068.765625s with
its debug endpoint closed after normal quit and a completed60-second wait.
Exact executable/command/start/candidate identity was rechecked before stopping
only that stale QA process. The targeted forced-stop request returned, but
Windows still reports one thread/process; disappearance is not established.
`qa-breadth-menu-world-20261021/owned-stop-forced-cleanup.json` preserves the
receipt. No replacement game was launched, no shared SDK change was made, and
the underlying shutdown cause remains unverified. Do not repeat force/restarts
or turn this into a speculative instrumentation campaign.

All work remains LOCAL. Existing owner playtest payload, native cards, watchdog
and game defaults are unchanged. Next runtime batch should use the named Armor
remainder or a new authored stage mechanic when the owned runner/host is usable;
ordinary setup still gets one attempt plus at most one source-backed correction.
The deferred snapshot/scenery/persistent-weakness/intro investigations remain
open without blocking independent breadth work.

## Native Armor and withdrawal-pad breadth batch (2026-10-21)

Unchanged9FFB6E38, one fresh hidden/muted owned process9564. Evidence:
`G:/Temp/Codex/psxrecomp/x5-parity/qa-armor-breadth-20261021/qualification.json`.
Ledger **83 passed,6 failed,145 unverified;151 open of234**.

| Comparison | Actual new native evidence |
| --- | --- |
| F92 | P1 controls/dismounts real Armor; P2 stays on foot and independently moves. Both land natively and move(-5.5/+3.5px), retaining distinct13/11HP and9ammo slots51/73. |
| F98 | Composed actual moving-rider image inspected in both panes: native rider head/cockpit/body occlusion and nearby on-foot partner layering; grounded dismount image also retained. |
| F147 | Coherent actual P2LEAVING1/P1sceneowner1 overlap at original pad2; native withdrawal reaches ABSENT2, actual loader/Grizzly full127HP intro releases, absent P2 retains17HP/9ammo73 and P1 moves on native input. |

Armor's original unprotected location is lava: initial native deaths are retained,
not labeled a co-op defect. One source-backed coordinate/history-only relocation
of the existing Armor uses the native starting platform. P1 already atX96 mounts
it natively; an empty-vehicle assertion was wrong, so the same encounter continues
without another load/teleport. Two Armor setups total, one correction, one same-
encounter continuation; then one hub/pad race. No flags for mount/ground/contact,
lifecycle, boss admission or success are seeded. F100/mounted transfers, vehicle
damage/destruction, other locations/rosters/current peers remain open.

`tools/test_mmx5_coop_refight_withdraw_runtime.py` makes the executed race reusable
with an exact owned-session check and bounded controls wait. AST/--help and scoped
diff checks pass; the actual native receipt is the saved G harness, not an extra
run of the promoted CLI. No game source or executable changes; prior19/19 CTests
remain prior evidence. Native source/private memory card hash is unchanged.

The previous52780 process is shutdown residue, not a general QA blocker: fresh
boot9564, actual native card Load and both mechanic batches work. Continue using
the current paused owned QA session; avoid repeating stale-process diagnosis.
Owner-facing relaunch was explicitly withdrawn as a wrong-session message; no
owner handoff was performed. Everything remains LOCAL; defaults/watchdog intact.

### Both-refight gate / diagnostics-off / local product docs (2026-10-21)

Candidate BBCFCBC1: bbcfcbc113f5c79ea49b39d3df1d9a771c2c86706c8ece9371de1aa278b7164c.
Evidence root G:/Temp/Codex/psxrecomp/x5-parity/qa-both-refight-20261021/qualification.json.
Original EXE regeneration and max-two-job build pass; all19 CTests pass2.16s.
F123 actual native idle callback wait, animation/support, movement abort, both-player
transport/release/resources and native Select withdrawal fallback pass. Pad support
is body71/89 bit8, not terrain70 alone; the initial failed harness assertion and
one same-encounter source-backed correction are retained. Original Either default
is preserved. No scripted scene-success, arrival, lifecycle or victory flags seeded.
No boss defeat/weakness/full lifebar-readiness or all-pads claim.

F70 passes in its own isolated diagnostics-off BBCFCBC1 profile, genuine F9 native
Load and actual current-build state save/load/native movement. No session recording
folder before/after load and no REC overlay. Production default-on remains.
qa-diagnostics-off-20261021/report.json retains operations and inspected composition.
The same current candidate passes274 unique combined core/mod CRC samples through8768,
resolved8796, both menus/attacks/glow/withdrawal/resources/hints and actual P2 boss-owner
fatal/hidden survivor/native controls. qa-both-pad-bundle-netplay-20261021/report.json.
Actual online pad admission and frontend Host/Join/offer/rematch remain separate gaps.

F67 product docs and genuine unchanged32568930 screenshot/source mappings now exist;
existing PolyForm Noncommercial license is preserved. Actual bundled UI/controller,
settings persistence and window/audio walkthrough is still required; no owner handoff
has been launched. F33 current325 probe saves5/6 at5.68/6.00MB; old9FF failed rollback
capture remains unresolved,16MiB cap only a hypothesis. F130/F131 convenient follow-up
found an intervening native team retry/hub before any shot; timeboxed, no new setup.
No cache-cap, armor variant, arbitrary phase or success-flag workaround was added.

### Native story opponent / concrete retained-cache blocker (2026-10-21)

qa-story-roster-fixed-20261021/qualification.json retains BBC original boss
delete and915 native original intro/art/AI/actualhits/survivor/finalhit evidence.
Pure originalF4D90 predicate uses canonicalhero0/1, preserving nearest targetbody
and resources. Native2D6FC otherwise clears allocatedactor0..7 when projectedX
is mistaken for story availability. F82/F85/F87/F117 pass bounded. Class41 art,
elective sequential policy and complete story branches remain unverified.
Actual postfightrequest16 reaches initialized native mode20/raw11/CP14 chapter;
source transient19 is not directlycaptured. P1frontendCross advances16pages;
fullchapter/nextdestination/ending stays deferred after160s ordinarytimebox.
A later4168215us watchdog abort is explicit in runtime.log. Exact5,969,713B ring
is preserved in that milestone's watchdog/starvation_dump.jsonl; cause pending,
watchdog unchanged, no stability pass.

Actual915 save8 rejection now has a decisive cause: stablepaused309+309 caches,
320fullatlases,42,127,760spritewirebytes exceed16MiB. Exact618buffer lossless
corpus reduces spritewire to1,338,962bytes, retaining all pixels/IDs/hashes.
Current correctedE61EFF70 schema/provider5 preserves SDKcap, rejects oldstates without
migration, validates packets before offsideallocation and restores without
allocation.20/20CTests2.31s plus actual618corpus pass; native warm save/load/
rewind stillpending, F33 remainsfailed.75C0B160 stage18Bothinitdeferral stalls
actual phase2 at heroes6401/6398.5 with class40uninitialized; extension removed
before this milestone. CorrectedE61EFF70 real native intro/release passes. F91
is failed from native early stand/text admission; no speculative camera fix or further gate variants.

F175 consumer audit rejects the privatecharacter-cache hypothesis. Scenery
uses bank0/liveGPUVRAM; logicalrows40..43 -> CLUT7900..7903 at VRAMx0..63/y484.
Existing captures lack actual per-eyeGPUrow evidence. Cause/fix stays deferred;
no speculativecache/SDK patch or newinstrumentation build.

### Large-cache current-build snapshot repair (2026-10-21)

Candidate E61EFF70 retains the tested story-roster predicate and adds a portable
lossless pixel codec to snapshot schema/provider 5. The SDK 16 MiB limit stays
unchanged; no retained sprites, menus or pixels are omitted. All 20 CTests pass,
including malformed packets, untouched live state on rejection and the complete
snapshot walker. The exact 618-buffer original-art corpus also passes.

qa-story-snapshot-fixed-20261021/qualification.json retains the actual native
story entry and source art identity, warm state operations and cold/mapped
operations. The warm native cache contains 26,655,128 bytes in the previous
raw wire format. Real state8 save is 2,221,667 bytes; after native movement,
actual load restores both positions, HP, ammo, world clock and P2 menu ownership.
Every retained cache metadata field and pixel SHA256 matches. Cold load before
enrollment restores the same private menu, mapped Square performs a real save,
and mapped rewind/Cross returns to the earlier P2 menu and native movement.
F33 passes bounded; physical keyboard assignment, audio listening and full
campaign coverage remain separate. Older states need no compatibility work.

The actual native story stand/text starts while Both-door phase2 is unfinished;
F91 is failed. A full class40-initialization deferral in candidate75C0B160 stalls
both heroes at the aperture instead. It was removed before this milestone.
Further timing changes are deferred until native camera/setup and stand/text
admission are separated by source evidence. No further gate variants now.

Composed snapshots preserve the native guest menu and heroes/effects. The
private P2 W-background plane is already missing before capture and remains
missing after restore; F175 stays failed, with no scenery/palette parity claim.
Current-build codec success does not resolve watchdog or long-session stability.

F37 source mapping now establishes native Ultimate DAT93 and saved FC rows,
but private Ultimate draw geometry/CLUT still needs a distinct native renderer
path. No partial option or fabricated flags are included.

The same E61EFF70 combined loopback passes 264 unique matched core/mod CRC
samples through 8480 (minimum final resolved 8501). Actual P2 native
boss-scene owner fatal, hidden living passenger/control release, both menus,
attacks/glow and default hint suppression pass together. qa-story-snapshot-fixed-netplay-20261021/report.json.
Actual online Both-pad admission, launcher Host/Join/START-offer and same-process
rematch remain unverified. All owned warm/cold/peer runners stopped.

### Other campaign story entry: bounded failure retained (2026-10-21)

E61EFF70 genuine card Load selects X/P1 and Zero/P2, then the original
raw18/0 CP2 approach allocates native class41 through its one-shot producer.
The native initializer executes, but the intro stays2/0 with HP0 through180s.
Original F8D08 waits for PLAYER action14; both actual heroes retain action4
during Both-door phase2. Native pause also fails to acknowledge. This is an
additional concrete Both-story admission blocker under F91; F84 art stays
unverified behind it. qa-story-other-campaign-20261021/qualification.json.
No stand, warning, scene, ready or success flags are injected. Runner11076
was stopped; further timing variants are deferred for breadth.

### Native Wing Spiral weakness and finite recovery (2026-10-21)

qa-weakness-airborne-20261021/qualification.json closes F125/F207 together on
unchanged E61EFF70. Native slot8 is Wing Spiral, verified by the actual selected
weapon menu; raw harness GroundFire labels are naming errors, superseded by
that qualification. Actual original class34 phased boss accepts P2's owned shot:
native32188 writes HP71->66, hitcount rises once, only P2 ammo is spent and the
authored wind reaction0B starts. Composed images retain the actual reaction.

During continued native attacks another12ammo is spent. Original0B progresses
1->2->3->4 through world8814..9027 without repeated HP/reaction restart. It exits
to native phase7; real recovery56->0 restores80075FC0 and original AI moves.
No boss HP, phase, immunity, reaction, cooldown or success writes are used.

Native checkpoint entry bypasses prior route; the existing stage4 clear bit
is removed only to reopen the encounter, with no defeat/reward/card claim.
The first left-relative aim placed P2 in the closed aperture and caused native
fatal before firing. That receipt remains; one verified floor correction
4720,171 completes the test. No further setup variants or rebuild. New elective
balance policy, other boss families, scenery and network remain separate.\n\n## Native boss balance milestone (78E06414)

Default-off scaling and the per_player(default)/shared recovery dropdown now
operate at the actual accepted-hit boundary. X credits5/10 and Zero4/10 into
one bank-owned carry. Native class36 X raw1 accepts with HP71/carry5; Zero
raw2 then debits71->70/carry3. Actual current-build state9 restores the changed
native boss, carry, private clocks, menu and world. A real scaled Zero final
hit after positive1HP setup clears carry and reaches original death, paired
victories and initialized12/1 report. No success/reward/card flag is seeded.

A separate shared run records original F8870 installing15 instead of30, with
no private clocks. Eight ordinary Mavericks and first Dynamo are guarded
source/unit scope; intro, second Dynamo, refights and final/multibank paths
retain native fallback. Actual other-descriptor, protected-phase/weakness,
Zero aerial/finisher/short-chain and ready-partner timing remain unverified.
The latter missed the nine-tick remaining window during native menu fade and
setup. Preserve that timing deferral and the earlier three-shot miss receipt;
do not create more aim/timing variants.

Resident hooks were regenerated;21/21CTests pass in2.30s, including the actual
hook protocol and schema6 snapshot walker. Low-level native31F60 duplicate
gating retains the outer31DD0 no-overlap scan/token cleanup. Snapshot6 rejects
older build snapshots; no migration or backward compatibility is required.
Shared SDK/UI code and native memory cards are unchanged.

The same candidate's combined compatible loopback passes267 unique matching
core/mod samples through8544, final resolved8556/8557. Both peers' effective
scaling/per_player flags are1/1; menus, attacks, charge/release, diagnostics,
Alia suppression and actual P2 scene-owner fatal/hidden-survivor/native release
remain coherent. One initial direct-LAN setup deliberately had different
balance settings and was safely rejected before frame0. Direct CLI does not
perform launcher host-option transfer; retain this as setup evidence, not a
launcher-negotiation pass. No more network variants this milestone.

Evidence: qa-boss-balance-20261021/qualification.json and
qa-boss-balance-netplay-compatible-20261021/report.json. F63/F99/F181 remain
**failed/partially implemented** for the explicit applicable scope and runtime
gaps. Ledger unchanged: **100 passed,6 failed,128 unverified;134 open of234**.
Continue breadth using actual native post-scene state and bounded evidence
reuse, rather than expanding this one feature into a configuration matrix.

## Post-scene dash / gate and death-camera evidence reuse

Candidate78E06414 closes F14 with actual P2-owned native class36 intro/release
followed by Zero/P2 dash while X/P1 remains composed in both panes. One native
floor correction moves the heroes away from the actual boss body; the initial
P1 dash was accepted and interrupted by natural hurt, not a co-op failure.
Both original dash poses/effects retire and native control returns. Exact
live/original trail palette remains F12, not inferred from visible bodies.

F23 additionally requires the real door. A Both attempt gives P1 ownership
when both native hit queries qualify (source2125), so preserve that failed
ownership setup. One source-backed Either correction uses the established
P2-door helper: actual owner2, native checkpoint3->4, both footpoints96 and
both original Right controls. Native P1 dash then moves65.625px over44 world
updates, retains both composed bodies/terrain/HUD and returns idle; no private
render mismatch/watchdog/device-read/VRAM leak. The corrected scene's comparable
48-tick idle/dash intervals take2.343/2.391s (ratio1.0205). This is a bounded
dash-linked stall check, not a60FPS or long-duration performance certification.
No new game build, instrumental variant or authored success flag.

The approved worker inspected five exact recovery rows using immutable
receipts; main verifies original hashes and decoded fields. F57 passes actual
81262804 P1FALLEN before P2 native gate admission through original closing,
P2grounded23HP/91ammo/no immunity, Right+17.5 and no retry. F58 passes the
07854014 vertical-descent analogue: distant old P1corpse, last-survivor fatal,
15 consecutive death/wipe samples with canonical+both private cameras1961/794,
then actual mode9 loader, one real retry2->1 and fresh grounded pair. The
next-world camera reset is not corpse-directed panning. These are individual
bounded analogues, not current-candidate or every-controller regression.

F20 arrival spacing/timing, F18 transient fade gaps and F170 scenery integrity
retain the audit's specific missing observations; no blanket pass from
ground/control/screenshot existence. Evidence:
qa-post-scene-dash-20261021/qualification.json and
subagent-x4-qol/control-recovery-evidence-reuse-20261021/audit.json.

Ledger: **104 passed,6 failed,124 unverified;130 open of234**. No CTest repeat
was needed: game sources/candidate are unchanged after7a1be51. Continue one
authored content/midboss batch next; no further dash/door ownership variants.

## Actual dynamic midboss / initial camera and native content breadth

AFC81AB3 repairs a reproduced P2-only progression blocker at raw3/0CP3.
On78E06414 P2 nativeRight reaches3037 and the pre-fight wall withP1still2880,
but no encounter appears. In that same world, P1Right to3025 immediately
starts original stand14 and allocates class1E/sub09 plus six linked components.

The fix guards original EFC34 by title area/logical8, original table/words,
controller phase3/0 and actual P2-forward/P1-behind predicate. It executes
that one native admission in P2's body/camera/stage-palette context. Existing
scene_owner2 stage dispatch then retains the same stand14 for the next native
EFC34/EFCAC calls. The accepting camera transfers as with a native P2 door.
No stand, camera-success, controller, health or defeat flag is manufactured.
No shared SDK, new persistent field or snapshot schema change.

Actual P2-only admission now produces owner2, seven native components and
normal ground control. Native EFCAC locks both current/targetX2880; authored
Ybounds240 remain while the original boss bob/motion continues. Actual composed
initial arena, native art and both bodies are inspected. Real X buster hits
write initialized shortened positive childHP1->FE at original32188, with
original C/D component deaths. F184/F191 pass bounded for initial camera and
active combat admission; this is more than allocation or screenshot existence.

The source recipe initially omitted mandatory opening combat. Waiting for
the later core phase4 without attacking was a setup error: original FC030
runs phase1 until genuine component-defeat bits permit the next stand/HPfill.
Native phase1 counters90/91 are not yet the later coreHP banks. The original
unattended P1 control really spent a retry2->1/resetCP3; do not infer an actor
deletion. The closely watched first fixed attempt eventually falls from the
changing terrain (P1Y505.86/fatal), retained in fixed-P2-native-entry.

One source-backed correction stops at live opening/control and saves native
state10. Six initialized positive childHP fields are shortened to1; no zero,
26/2A completion bits, phase or vulnerability table is written. One ground/
aerial sequence and one continuation really kill C/D; lower E/F remain and
the encounter is still phase1. Defer F166/F187 with that actual prerequisite.
No further aim/phase variants now. The authored later exit is an internal
raw3/0CP3->CP4 camera/terrain/controller boundary, not a3/1 load.

Regeneration0.656s, current build9.594s(max2jobs/G),21/21CTest2.797s pass.
Current combined loopback passes268 unique core/mod samples through8608,\nresolved8639/8639, including both effective balance flags, menus,\nattacks/effects, diagnostics, Alia suppression and P2-scene-owner fatal/survivor\nrelease. Actual online midboss admission and launcher/rematch remain unverified.\nEvidence: qa-midboss-current-netplay-20261021/report.json.
Evidence: qa-native-midboss-20261021/qualification.json and the exact corrected
native-parent-phase1.txt; worker original-content recipe and original hashes
are in subagent-x4-qol/midboss-native-20261021.

Additional native content breadth on unchanged78E06414: raw4/0CP1 andCP2
load via actual checkpoint records and each hero independently rises/lands
while the partner stays still (31.0078px each atCP1,26/24.4531px atCP2).
Those four launch-point checks do not close terrain routes, hazards, enemies,
audio, retries or exits. The generic boot receipt lacked character metadata;
a separate observed session.json corrects that harness prerequisite without
altering the original receipt. qa-content-breadth-20261021 retains both attempts.

Ledger **106 passed,6 failed,122 unverified;128 open of234**. Remaining failures
F37/F63/F99/F91/F175/F181 preserve their individual partial evidence and gaps.

### Native corridor-arrow comparison and source inventory (2026-10-21)

Original BIN3 class15 is a paired corridor transfer rather than a proven gravity
or terrain-material toggle. The frozen X4 switch failures map to its same
singleton admission/shared camera/player-transform boundary. A685 retained
P2 physically inside the exact original window without activation; P1's
position-only ownership control immediately committed the genuine transfer,
leaving P2 at upperY395 while the displayed cameras moved downstairs.

96AA8916 scopes the original F26AC admission to the eligible hero with P1
priority, then registers its observed native PLAY10/FA238/actor-phase lock in
the existing scene owner/passenger machinery. Native index selection, effect
cleanup, fade, camera bounds, destination, release and subsequent control stay
original. No new persistent state or SDK change.

The actual P2-owned forward transfer returns both at1624.5,Y874/875; real
nativeLeft moves P1/P2 by-23.5/-17.5. The distinct P1-owned nearby-partner
inverse returns both at2024,Y394/395, with nativeLeft-19.5/-21.5. Both cases
have one admission/scene-start and no fatal, with composed output inspected.
A first setup's floor assumption, first patch's wrong table-slot guard and
an inverse setup rejected by authored minX remain explicit failed receipts.
The corrected immutable guard is F9DE0=F26AC, global sub0/phase1; F9DDC is
the no-op phase. Actual arrows online and snapshots during transfer stay
unverified, as do all other arrows/routes and literal gravity inversion.

Source-backed inventory correction follows separate primary and secondary
native terminators. The original raw2/1 secondary list startsF8EF4, contains
six controller records and13arrows, and endsF8F8C. Independent source/receipt
hash checks confirm113checkpoint and1535primary records unchanged plus206
secondary entries in21sections. Inventory presence proves neither allocation
success nor gameplay. See subagent-x4-qol/secondary-placement-source-20261021
and qa-corridor-arrow-20261021/content-inventory-primary-and-secondary.json.

### Native field recipient and closing-gate evidence (2026-10-21)

The actual class39 field exposed a separate X5 bug: nativeF8050 set one hero's
F7, but nearest-body selection later let F80C8 clear the other already-zero
body. The original recipient retained nativeF7=2 after the first field. Source
2C300 confirms2 is a native palette intermediary, so the value alone was not
treated as a foreign capture state.

39161AF2 retains the unique living held recipient only through original BIN3
phase9/sub4..7. Both-held/death/interrupt ambiguities preserve existing behavior
and remain unqualified. No separate owner state was added. Source review
includes the complete eight-entry table and BOTH native releases: F5B60 at
sub6 entry and F80C8 at sub7 timer100.

Fresh actual firstcycle completes the latter branch with both F7zero, native
nextattack and real controls23.5/25.5. A single position-only nearer-bystander
ownership setup exercises7targetoverrides and the former branch's actual
F7=2->0/PLAY10=2->0 writes; the held hero moves13.5 after release. Native120-update
work observations are6.719/5.422wallseconds before/after, with no render/watchdog
faults. Initial budget/misread-table and Left-into-wall attempts remain retained.
No broad capture/field camera/performance/netplay claim.

F35 reuses original D508/CAC gate hold with a living upstream partner, then
actual full-box clearance/nativeclosure. Main independently verified13 hashes
and the original door/hero boxes. F183 remains unverified: one-beyond/one-inside
closing-region occupancy is missing; stage8 retained samples have both overlap.
Older named-build gate evidence does not become a current-netplay blanketpass.

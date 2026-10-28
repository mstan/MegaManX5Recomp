# X5 co-op manual QA handoff

Current follow-up candidate: **EBDE0959 / source404ec42**, local only, with
25/25 title CTests passing. The [v0.2.5 follow-up board](COOP_X5_025_FOLLOWUPS.md)
has **6 validated / 1 partial / 3 unverified / 10 total**. Four entries remain
open; historical comparison coverage and LAN remain parked.

The established station at
`G:/Temp/Codex/psxrecomp/x5-parity/current-owner-candidate/Validation.ps1`
now groups those entries into three episodes:

| Prepared episode | Individual acceptance still needed |
|---|---|
| Intro, at the foot of the genuine roof climb | V05 actual Both-door carry animation, closure and control recovery; V06 intact statue, reveal, skipped co-op dialogue and Sigma initialization. Climb normally: earlier roof teleports did not establish native floor prerequisites. |
| Dinorex part1, checkpoint2 ledge | V09 P2 camera stops at the intact fire barrier and releases after actual flame destruction. Also inspect the part1 atlas and P2 flame-trigger fixes along that route. |
| Dinorex part2, two original native suits | V10 normal boarding and the full armor-only branch, with separate controls/riders. An optional fourth card starts with both original boarding animations completed if setup is difficult; it does not pass normal boarding. |

All four current-schema15 snapshots were prepared on the current executable,
then loaded in fresh muted processes from the actual station profile. Native
pause exit, living heroes and separately addressed synthetic input pass for
each. This establishes fixture readiness, not the outstanding mechanics.
No floor, barrier, door, rider, occupancy or success flag was authored; the
mounted fallback uses disclosed position-only boarding overlap. Automatic HP
refill and contact protection are off. Earlier native cards and owner notes
are preserved. Each case has its own candidate-specific observation key.

P1 is keyboard and P2 targets the owner's USB DualSense. Owner launches use
normal windows/audio, exclude the development input merge, and begin paused.
Previous physical input-isolation acceptance is retained because routing code
is unchanged; verify the connected device in the current launch log. Record
each named check separately, including Not reached. Close the game before
switching/restarting cards. The delivered Nextcloud0.2.5 archive is unchanged.

Setup/qualification evidence is retained under
`G:/Temp/Codex/psxrecomp/x5-parity/owner-025-reports-20261027`:
`owner-current-setups-report.json`, `owner-current-cold-qualification.json`
and `two-armor-final-controls-report.json`. The original overlay-cache package
is unqualified and disabled; matched performance evidence for the corrected
candidate remains a future release gate.

## Earlier opening-report handoff

Earlier opening-report candidate: **92866EAD**, local only. Gameplay is the
tested765AC663 scene/scenery candidate; the rebuild changes a source comment
and its diagnostic identity. All24 CTests pass. The
[v0.2.2 opening-report board](COOP_X5_022_OPENING_BURNDOWN.md) has
**3 passed / 3 unverified / 6 reports**. Building scenery, screen-relative
lower scenery and the visible companion's native stand/control recovery pass
bounded checks. Buster artifacts, the exact Alia-return graphics report and
the clarified Zero/P1 HUD appearance remain unverified.

The established local station is
`G:/Temp/Codex/psxrecomp/x5-parity/current-owner-candidate`. Four focused cards
are prepared: Alia's actual first page before X returns, an early buster/scroll
window, Zero/P1 with X/P2 for the HUD, and the authentic roof before Both doors.
P1 uses keyboard and P2 targets the USB DualSense. The Alia card begins in
dialogue: press P2 Cross through the pages, then inspect X's real return.
Other cards begin paused: Enter resumes. This QA profile deliberately enables
visible Alia and Both doors. Release defaults and the single delivered
v0.2.2 archive remain unchanged. Preserve the owner's existing notes/runs.

## Earlier release and follow-up evidence

Local release v0.2.2 uses **F7408EFC**, a version-only rebuild of the
D20C946D gameplay candidate below. The portable package refreshes the intro,
Grizzly and duel snapshots for this build and checks fresh-process load and
control recovery. The old Gaea-specific snapshot/launcher is retired; no
old snapshot conversion is required. Gaea gameplay and its retained visual
evidence are unchanged. Historical package recipes below describe their
named builds, not an additional current delivery. The remaining three intro
checks stay unverified; release authorization does not qualify them.

Earlier follow-up candidate: **D20C946D / sourcebbf2b3d**, local only.
[Latest owner reports](COOP_X5_021_FOLLOWUPS.md), after autonomous follow-up:
**13 bounded passes / 3 unverified**, out of16. Remaining: exact hovering-platform
smoke, and actual Both-door camera/visible-closure validation during Sigma.
Roof smoke passes its actual native producer/color check. X/P2's lingering
charged-orb seams are fixed and pass native visual/packet/expiry checks.
All24 CTests pass. The old234 comparison campaign and LAN
checks remain parked.

The current two-case station is
`G:/Temp/Codex/psxrecomp/x5-parity/current-owner-candidate/Validation.ps1`.
It uses one current executable, separate fresh native X/Zero campaign
fixtures, autosaved notes and image capture. P1 is keyboard; P2 explicitly
targets the previously verified USB DualSense. The owner run logs confirm
that the DualSense opened for P2. Owner launches
use normal audio and a normal window. Prior owner notes are preserved in
the station's history directory.

The remaining intro case requests U07/U10/U11 along the actual authored
route. Record each of those results separately in its notes; a partial route
does not pass the later checks. Previous B5B77098 owner notes and captures
remain preserved. U02/U03 have bounded early-stage owner acceptance; the
older Fourth-card caveat led to the now-fixed lingering effect. Both-player
doors and visible Alia hints are intentional QA settings. The shipped hint
suppression default remains on. Current-build paused snapshots replace
all older fixture expectations; native campaign cards remain separate.
The delivered Nextcloud ZIP remains unchanged pending these checks.

## Previously delivered release and evidence

Current local release **v0.2.1-coop-alpha-debug / 4EBA758E**, 2026-10-26, on
feature/mmx5-x4-parity-local. No publishing, pushes, PRs or remote tracker sync.
Requested local delivery:
G:/Nextcloud/Games/PSX/MegaManX5Recomp-v0.2.1-coop-alpha-debug-windows-x64.zip.

The owner accepted11A66B52's duel defeat timing and confirmed physical input
isolation. Recent follow-ups: **8 resolved, 2 deferred, 10 total**. Remaining
manual work is reproduction of the older Both-door intro fatal and the solo
Grizzly background pause. Neither is a release-qualified fix. Use the two
focused cases in the owner station; LAN and the old234 campaign stay parked.

Current scope is the [user-reported issue inventory](COOP_X5_USER_REPORTS.md).
The old comparison campaign remains parked. Owner82A36EAA station notes were
read and preserved: Grizzly graphics/deaths, Gaea and solo Alia passed; duel
arrival/meters failed. LAN is deferred. Coupled controls invalidate independent
ownership conclusions from that owner run. The large-health helper did not
establish human pickup validation; native pickup automation remains separate.

[Grizzly](COOP_X5_GRIZZLY_BURNDOWN.md): **9 passed / 0 open / 9**.
[Armor/starting bonuses](COOP_X5_ARMOR_SELECT.md): **6 passed / 1 unverified / 7**;
remaining X/P2 LAN check deferred. [Graphics/pickup follow-ups](COOP_X5_POST_GRIZZLY_QA.md):
**4 passed / 0 open / 4**, bounded as recorded. New input, intro-block and solo
background-pause reports are tracked separately. Duel arrival/meters now pass
bounded automated checks after the owner's failure; visual/balance acceptance,
native winner persistence and story validation remain open. Not all reports
are closed.

11A66B52 refines1864D0A4's native crouch/no-orb defeat: the loser now vanishes
during the original white-flash ramp, while explosions are still running,
before the screen reaches full white. Check that earlier disappearance when
finishing the prepared fight. Other stages retain ordinary death behavior.

8EA4F4CA fixed the owner's duel defeat report: only the losing meter empties;
the living boss participant retains actual HP; defeat explosions originate
at the fallen loser. P1's own HUD body/art remains selected after P2 becomes
the surviving world owner. These changes preserve the private native result
sequence without ordinary game-over or a spent team retry. Prior story/save
and LAN limits remain open.

9D02E26B fixed the delayed Zero arrival, lingering intro meter, inherited96HP
red drain and P2 entrant HUD art/packet path. The actual P2 entry shows both
heroes by the first dialogue page and aligned32HP meters when controls return.
Movement, native menu, current-build active save/load and an actual2HP X shot
also pass. This does not qualify winner-selected stories/endings or native
cold-save winner persistence. Solo scrolling and the old Both-door fatal were
not reproduced with a qualified setup and remain deferred, as documented.

This candidate fixes explicit unbound keyboard slots and removes the station's
PSX_DEV_INPUT all-controller merge. Actual OS keyboard input moves only X;
DualSense button isolation is now accepted by the owner. Roof blocks wait until
all living heroes cross, independent of door setting. If either hero dies or
P2 withdraws, the survivor may activate them alone. Five actual native admission,
motion/completion and linked-piece checks pass. These do not certify LAN or
normal-route collision traversal. No new Select behavior was added.

The owner station's first two cases now target the remaining reproductions:
normal intro with Both doors, then solo Grizzly scrolling. The intro uses
P2 Sony DualSense/P1 keyboard; solo uses the controller as P1. Both start
paused, with normal audio/window and diagnostics. Prior notes are preserved.

Retained evidence includes24 CTests, actual new games in both seat assignments,
native selection/entry and warm snapshots, all five P2 X forms, Falcon flight,
Gaea's repaired initialization, P2 Ultimate Nova with independent P1 movement,
unchanged native card Load/progress, and a real co-op-disabled native Zero
regression. Eight local two-peer checkpoints match core/mod CRCs and private
draw checks. Physical LAN and X/P2 online armor selection remain unqualified.
An additional input-only native-card test now passes X/P1 armor selection
online at three matched checkpoints, including wrong-seat rejection, Cancel
and actual entry. X/P2 armor selection and physical LAN remain unqualified.

## Controls and bonuses

Choose P1's hero at initial New Game; P2 receives the other hero. P1 handles
boss selection and the other frontend menus. At unit selection, the original
selector first offers forms of P1's campaign hero, then forms of P2's opposite
hero. Only the selecting seat controls its choice. Zero's native normal/Black
previews are separate choices; P2 can cancel back to P1. One X and one Zero
remain fixed by the campaign.
Fourth is always available to X in co-op, including X/P2, and Zero always has
the Z-Buster. Other X forms follow native availability. Solo behavior stays
native. The two in-stage Start weapon menus remain independent.

## Prepared tests

Extract into a fresh folder on F: or G:. Launchers use a normal window and
normal audio, P1 keyboard/P2 controller, local Split, Either doors, default
diagnostics and default Alia suppression. Rebind or use LAN through the normal
launcher. The external USA disc path works on this PC; select the other PC's
own disc for LAN.

| Launcher | Prepared state | Qualification / bypass |
|---|---|---|
| Play X5 Co-op.cmd | Normal launcher and Host/Join. | Use matching builds. No snapshot import for LAN. |
| Test Grizzly Stage.cmd | X/P1 Fourth, Zero/P2; actual fresh Grizzly CP0, paused. | Normal HP/contact rules, clear mask0 and unseen cannon hint. Route to this stage is bypassed; no cannon/timer/capsule/door/boss result seeded. |
| Test X vs Zero Duel.cmd | Zero/P1, X/P2 Fourth; paused outside the real inner duel door. | Actual native Zero campaign and original18/0CP2 loader bypass the route/outer door. No role, winner or result seeded. Actual current-candidate entry has one intro/arrival and equal32HP. |
| Test Gaea Armor.cmd | Zero/P1, X/P2 Gaea at Grizzly start, paused. | Optional visual acceptance. Gaea availability is a setup grant; acquisition is bypassed. Normal HP/contact rules. Reference captures are in qa/gaea-reference. |

Press P1 Start (Enter) to resume a prepared state. Each launch creates disposable
private saves under qa/*-runs. Native campaign saves stay separate. Snapshots
require this exact executable; older-build compatibility is not required.
Prepared states are offline evidence, not LAN or campaign-completion evidence.

## Remaining focused owner checks

1. Optional device isolation and roof: use Test Intro Blocks.cmd. Keyboard controls
   only X and DualSense only Zero. In the roof climb, one living hero alone
   must not trigger the moving blocks; the second hero crossing releases them.
2. Duel: use Test X vs Zero Duel.cmd. Watch one visible arrival during dialogue,
   no meters during the conversation, then two stable aligned32HP meters at
   control release. Try each hero's moves and Start menu, then fight normally.
   If the entering hero loses, their left meter must empty while the boss's
   right meter keeps its remaining HP. The loser crouches, then vanishes into
   the explosions during the white flash, before full white; ordinary player
   death orbs must not appear during this duel.
   Restarting can test the other entrant; no full configuration matrix needed.
3. Optional evidence collection: if solo Grizzly's background pauses again,
   note the location/action and keep the recording. The bounded900-frame
   check did not reproduce it. The older Both-door X fatal also needs a
   normal-route reproduction; direct checkpoint setup did not qualify it.
4. LAN remains deferred at the owner's request. Existing Grizzly/Gaea graphics
   and solo Alia owner passes stand; repeating them is optional.

Capsule regression now passes an actual native part grant, both controls and
own HUDs during unlocked return. Boss intro/visible passenger/control return
passes with each true leading seat under Either doors. Keep the non-entering
hero behind the aperture when choosing which seat should lead.

## Duel acceptance

Move either hero right through the inner door; the non-entering hero becomes
the boss, including a hero who died earlier in the stage. Restart the fixture
to test the other entrant. Normal moves and independent weapon menus remain.
Both start at32HP, healing and damage bonuses are disabled, and native boss
appearance/meter/one-arrival polish is retained. Current bounded entry passed;
winner-selected story, native cold-save winner persistence, endings and LAN
duel acceptance remain open. Cold native-save winner persistence is not
implemented. Basic damage tuning remains2HP buster,3HP opening slash,5HP
full charge/final slash, at most6HP for a large attack.

## Bundled checks

Keep one recording per useful episode and report the stage, hero/seat, last
action and expected result. Both launchers record beneath the extracted
build's `saves/diagnostics`. Preserve that directory with any screenshot.
Disposable duel memory cards and snapshots stay under `qa/duel-runs/<run>`.

| Episode | Checks to bundle | Limits / remaining gaps |
|---|---|---|
| Intro with Zero leading | Intact statue before explosions; native Sigma reveal; no reunion NPC/dialogue; trailing X survives and finishes teleporting; both move and attack in the first fight. | Owner confirmed reunion skip and reported no other intro problems on FA55C32F. Owner original33E9009F normal-route recording now confirms intact statue/explosion/reveal art; CE9B51FB fixes P2 head visibility. Sprite ordering is corrected by preserving the complete P1 base/armor sequence; the original Both-door X fatal remains separate. Distinct authored descent, death during WARNING and Zero-campaign X/P2 final hit remain separate comparisons. |
| Separated local play | Start shows each player's own character and inventory; large health pickup preserves private cameras; Zero airborne dash trail; one death then team death; Grizzly scrolling/black bars. | These combine recent owner regressions. R9 dash presentation remains failed pending visual retest; broader stage checks do not inherit a pass from one location. |
| Prepared X-versus-Zero duel | Choose either entrant; other hero teleports into the enemy area; correct NPC appearance; only entrant's personal meter plus right-side opponent meter; normal movement, attacks and each native Start menu; reciprocal damage; winner-selected result. | F62 remains unverified. Exact appearance, remaining story/ending, native cold-save winner persistence and current LAN duel are not fully qualified. Cold native-save winner persistence is not implemented. Test opposite entrant by restarting the fixture. Prior-stage-dead revival has older bounded evidence, not a new current-build manual pass. |
| One LAN session | Same build/options on both machines; distinct seat ownership, movement/attacks, personal menus, health pickup, death/retry, leave/restart. | Current local two-process regression is bounded evidence, not physical LAN validation. Moving-support and Ride Chaser network route checks remain open. |
| Remaining encounters | Follow the shared encounter batches in COOP_HUMAN_SETUP_QUEUE.md; reuse one setup for its listed comparisons. | Boss rewards, actual exits, branches, saves/endings and unresolved native analogues retain individual statuses. Do not mark a comparison passed from a load or screenshot. |

Local source milestones: f651130 fixes the static projectile/exhaust images;
c32de77 removes ordinary co-op pickup freezes. The graphics fix also retains
Gaea's two native compressed uploads; no stage sprite is globally remapped.
All24 CTests pass. owner-effects-20261025/combined contains the19-sample actual
graphics check; pickup-qualified contains the three native item cases and1250
clean private-render checks. Failed baseline and setup receipts are retained.
Six matched local network checkpoints and all three fresh-process prepared
fixture restores pass on82A36EAA, with native card bytes preserved. The refreshed
duel setup also passes actual intro/arrival and equal32HP; it does not qualify
the remaining winner/story/save checks.

The historical comparison ledger is retained under `docs/` as reference and
is outside the current user-report scope. Current graphics fixes do not close
R9 or A05 before
the owner retests the original symptoms. Solo cannon and physical LAN remain
manual; no new setup matrix is requested.

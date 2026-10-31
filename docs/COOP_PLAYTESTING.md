# Local X5 co-op playtest

Use the local build supplied for this campaign. Work remains on
`feature/mmx5-x4-parity-local`; no new release, remote branch or pull request
has been published. [The burndown](COOP_PARITY_BURNDOWN.md) and
[X4 comparisons](COOP_X4_REFERENCE.md) retain individual evidence and open gaps.

## Shared validation with the owner

Use one controller for P2 and the keyboard for P1. Most remaining tests have P2
lead while P1 waits safely; the agent handles setup cheats, teleports, options,
logging and measurements, and releases synthetic input before handing over.
Use normal windowed presentation and normal audio. The owner can also test LAN.

There are 42 setup-deferred comparisons in 15 tracked encounter bundles. Two
pairs share stage preparation, and the damage-accounting probes fit existing
fights plus one Dynamo visit. These are short targeted sequences reached with
cheats where safe; full ordinary route replays are not the plan. Where automated
positioning could not establish genuine terrain/camera state, the owner may
need to cross that short section normally. A shared validation attempt may
still expose a bug or leave a prerequisite unresolved.

| Shared sequence | Comparisons | What the owner does or judges |
|---|---:|---|
| Opening rescue and boss | 9 | Keep P1 safely behind while P2 leads the roof/rescue route and boss entrance; judge visibility, landing, passenger return and control recovery. An intentional death check uses a preserved genuine start. |
| Mattrex route | 7 | Descend the real shaft, attack the gate, cross the section boundary and doors; judge scenery, effects, collision and both heroes' recovery. Vehicle transfer is checked only if that route actually permits it. |
| Train and its boss | 3 | Move P2 over the pit, check the surviving-player camera, then make a persistent attack touch the boss, separate and touch again. |
| Lift and optional route | 2 | Ride the native lift with the pickup, then enter and leave the optional upper area with P2 while P1 waits outside. |
| Ride Chaser | 2 | Inspect both riders and exhaust, then steer a survivor through the actual road-to-foot handoff after the other rider dies. |
| Rosered | 3 | Let a natural attack capture P2 and release them; repeat only for the distinct captured-death check. Try the two owned effects together when their real prerequisites are available. |
| Later section and phase boss | 4 | Cross the real section exit, hit the low enemy, then align a charged hit and respond to natural homing/shared-field attacks. Judge targeting and camera/control recovery. |
| Water midboss | 3 | Stay on the moving ledges through the descent, defeat the real cores, and cross the actual exit; judge hazards, cleanup, scenery and controls. |
| Delayed enemy | 1 | Pass the real ledge, wait for the native emitter and interact with its child; judge visibility, travel and disappearance. |
| Fortress eye to next stage | 1 | Land the real final hit with P2 already fallen, then follow report/Continue; judge healthy fresh heroes, resources and controls in the next stage. |
| Final bosses | 2 | Land the descending weakness attack, then give the later fist attack a genuine P2 target; judge injury, animation, target and recovery. |
| Earned special armor | 2 | Enter the real pod with X/P2, follow its native report/equipment path, re-enter, save and cold-load; judge body art and abilities. |
| Protected boss hits | 3 | During the Mattrex/phase-boss visits and one Dynamo visit, attack in natural ordinary/protected/weakness windows. The agent measures damage, rejection, cooldown and ownership accounting. |

These rows total 42. Individual IDs and exact evidence remain in the
[shared validation plan](COOP_HUMAN_SETUP_QUEUE.md). The first opening sequence
covers nine comparisons using one route; begin there when ready.

The current prepared B4EC5680 local-validation profile is at
G:/Temp/Codex/psxrecomp/x5-parity/owner-human-batch-B4EC5680-20261023.
It includes the P2 camera-emitter fix and has 23 passing title tests plus native
component evidence. Its final combined CLI peer run stopped after 10/14
checkpoints on a debug TCP timeout; that cause and the remaining peer checks are
unresolved. The older qualified 7336E94F profile below remains preserved.
No owner game has been opened by preparing either profile.

For LAN, use the exact same candidate on both machines with the host's agreed
options. The agent addresses the current automated peer-test transport issue
before the LAN handoff. Bundle the 11 attended checks into session startup and
Host/Join/START, physical controls, settings, leave/restart/rematch, then the
moving-lift and Chaser/two-door routes. Bind each local device as Player 1; the
room assigns the gameplay seat. Online setup uses native replicated input, not
offline RAM writes.

## Earlier qualified campaign profile

The preserved 7336E94F profile is at
G:/Temp/Codex/psxrecomp/x5-parity/owner-human-batch-7336E94F-20261023.
Run **Play X5 Co-op.cmd** to open the launcher. It starts windowed with normal
audio, local side-by-side co-op and Both-player doors. Choose distinct P1/P2
devices on the Controller page before PLAY. Its copied native cards,
diagnostics and caches stay in this G: directory; original cards stay intact.
Diagnostics and Skip Alia Hints are enabled by default; both automatic forms
and boss scaling remain off. The launcher restores default display capture
even when started from a controlled ring-off QA environment.

The included card is the unmodified native campaign save. Use
CONTINUE and Memory Card 1. New Game remains available for the genuine
intro/campaign setup. The older CDD2FE2A, C72CD101 and D1B93AFC profiles remain intact alongside this
one; use the current candidate for new campaign evidence.

The [setup queue](COOP_HUMAN_SETUP_QUEUE.md) groups the remaining68 checks,
including13 technical source prerequisites distinct from human setup.
The agent prepares each shared batch and records any genuine starting state
that still needs owner movement. Leave the game running after the requested
sequence so the agent can inspect it. A current-build save state can preserve it.
The launcher's debug address is recorded in owner-launch.json when launched;
diagnostics-location.txt identifies the current input-recording directory.

7336 passes23 automated tests,11 watchdog timing checks and a14-checkpoint/
260-sample current CLI peer bundle with default diagnostic capture. It includes
the Chaser debris fix, native paired water-head ownership, P2 temporary-platform
admission and guarded memory projection. Actual P2 usable support and native
expiry/release passed; distant water visibility remains unverified.
A separate native boss encounter stopped under the standard watchdog; its
cause remains unresolved. Preserve diagnostics after a stop. This is an
interim human-setup candidate, not full parity or a stability qualification.

Preparation verifies paths, launcher syntax, window/audio configuration and
private card copies. It does not qualify physical controllers, visible
frontend or speaker output. No owner session was opened by preparing it.

## Start a local session

1. Keep this playtest folder, cards, cache and recordings on F: or G:.
   Select your USA **SLUS-01334** disc in the launcher and use **OpenGL**.
2. Open **Mods** and enable **X + Zero Co-op**. It defaults off.
   Use a fresh boot after changing co-op options.
3. Assign different devices to P1 and P2. Two keyboard players need different
   bindings; the supplied keyboard defaults overlap between seats.
4. Start in a normal window with sound enabled at a normal volume, then press
   **PLAY**. Owner handoffs should retain these settings; automated QA is muted.

P1 is the natively selected campaign hero. P2 is the other hero when story
availability permits. Both share campaign progress and acquired upgrades;
health, weapon selection and ammo are per player. Ride Armor is P1-only.

## Choose the display and participation rules

| Mods setting | Choices / default |
|---|---|
| Local co-op display | Unified (default), Side by side 8:3 (Experimental) |
| Netplay cameras | Unified (default), Split |
| Life meter layout | Stacked (default), Side by side |
| Separate E-tank contents | Off (default); On separates contents, with shared tank acquisition and W energy |
| Ultimate Armor X | Off (default); equip X's native Ultimate Armor without awarding an unlock |
| Black Zero | Off (default); use Zero's native black form without awarding an unlock |
| Keep Zero playable | Off (default); local-only availability override |
| Door and refight entry | Either player (default), Both players (Experimental) |
| Co-op boss scaling | Off (default); elective balance for regular Mavericks and the first Dynamo |
| Boss hit cooldowns | Separate for each player (default), Reduced shared cooldowns; applies when scaling is enabled |

With scaling enabled, X deals half native damage and Zero deals 40%, and
ordinary hit recovery is shorter. Other encounters retain native balance.
Encounter coverage and some cooldown interactions remain open; retain the
chosen settings with your test report.

The two form options work independently and need a fresh boot. Ultimate Armor
X overrides the selected X armor while enabled; Black Zero uses X5's native
damage and weapon-energy behavior. They do not award capsule rewards or saved
unlocks. Use the co-op settings for both players; the separate **Cheats**
entries apply to offline single-player play. Netplay uses the host's choices.
The bounded form contracts include native abilities, genuine progress,
death/retry and current-build states/rewind. General rollback, rematch and
unexercised native contexts retain their own coverage limits.

Local side-by-side uses two 4:3 panes and requires **Custom Renderer**,
which defaults on with **Aspect: Adaptive**. Shared menus and story scenes
appear in both panes. The netplay camera choice is separate from the local
display choice. Life meter layout applies to the shared display; independent
displays keep the local player's meter in its native position.

Both-player doors and refight pads wait for living teammates at the same
entrance. Local refight-pad checks cover waiting, both-player admission and
withdrawal fallback. Online pad admission remains unverified. Fallen or absent
partners follow the existing single-survivor participation policy.

For netplay, both peers need the exact same build and compatible mod choices.
Use the launcher's Host/Join flow; each peer binds its local device as Player 1,
and the room assigns the gameplay seat. The host chooses shared options.
Internet sessions and the full campaign remain alpha testing coverage.

## Controls

These are stock bindings; saved controller or in-game remaps take precedence.
The complete PSX button tables are in [README](../README.md#controls).

| Action / PSX button | Default keyboard | Xbox-style pad |
|---|---|---|
| Move / D-pad | Arrow keys | D-pad or left stick |
| Jump / Cross | X | A |
| Primary attack / Square | Z | X |
| Dash / Circle | S | B |
| X special weapon / Triangle | A | Y |
| Previous / next weapon, L1 / R1 | Q / W | LB / RB |
| Giga attack / R2 | R | RT |
| Own menu / Start | Enter | Menu |
| P2 withdraw / rejoin, Select | Right Shift | View / Back |

The player who opens a menu owns its input. Hold P2 Select to withdraw; release
it, then press it again to request a rejoin near P1 when safe. The forced Ride
Chaser sequence does not allow voluntary withdrawal. Co-op uses one shared
retry after a team wipe.

## Saving and restoring

Use native **SAVE** and **LOAD** for campaign progress on PS1 `.mcd` / `.mcr`
cards. Back up a card before testing. Co-op uses an isolated mod save profile;
P1 owns the campaign. P2's private in-session state is not a second native
campaign save, and original card Load may reset consumable contents.

Savestates and rewind are current-build QA aids. They include co-op host state,
but refusal/restore coverage remains incomplete. Keep them disposable and use
the same executable and mod settings. A rejected state request does not
establish that the native card save is bad; retain the diagnostic evidence and
use the game's native card flow. Old release notes that disabled co-op states
describe earlier builds.

## Diagnostics and Alia hints

Leave **Input Display and Recording** (in **Playtest Diagnostics**) on for
testing; it defaults on. The overlay shows accepted P1/P2 inputs and an input
counter. Sessions are recorded locally under
`saves/diagnostics/session-*` beside the executable. `diagnostics-location.txt`
points to the recording; startup logs and `startup-log-location.txt` also work
when a Windows launch has no console.

The session includes `inputs.csv`, mode changes, initial card copies or absence
markers, build identity and mod choices. Restoring a state starts a new recording
baseline segment. These records contain game-save data; keep the complete
session local for the test owner.

**Skip Alia Hints** also defaults on. It removes automatic in-stage hints while
keeping native story, boss and capsule conversations. Netplay always suppresses
these automatic hints and includes diagnostics.

## Report an issue

After reproducing it, quit normally and zip the complete session folder for the
local test owner. Include the build identity, stage/area, player seat, display
mode, what you expected and what happened. Include a video timestamp if
available. For netplay, retain both peers' folders. Leave the disc, BIOS and
unrelated personal files out.

Developer position/HP/stage fixtures are separate setup tools. Record their
use and any bypassed prerequisites; entering a stage or taking a screenshot
does not prove a mechanic. Direct offline fixtures add `D` provenance rows;
input-only replay rejects those traces because pads cannot reproduce them.
Ordinary builds keep replicated qualification cheats disabled.

For developers, offline playback uses `tools/replay_mmx5_diagnostics.py` with
the original executable, configuration, BIOS and disc. It uses private copies
of starting cards/settings, rejects incompatible identities and stops on
state divergence. Network traces support inspection; independent offline
replay of them is unsupported. Replay checks inputs and a game-state subset,
not complete GPU/SPU output or every rendered pixel.

## Screenshot and verification scope

[The earlier local capture](images/mmx5-coop-local-20261021.png) is an unchanged
composed capture from candidate
`32568930ffaef00aea193e3217e6ae407f7dabc1d420493ebc725f2e1369bb1d`
on 2026-10-21. It shows actual X/P1 and Zero/P2 local side-by-side gameplay
during the earned Zero-technique check. Position and HP setup assistance was
used; no ability grant was used. It predates the Both-pad fix and does not qualify that change.

The README now shows unchanged gameplay and owned-menu captures from candidate
89E80004's two-peer test on one PC. Their captions distinguish that view from
local side-by-side play and disclose setup assistance. They do not establish
full routes or Internet compatibility.

On the same candidate, actual bundled Mods controls and PLAY saved six changed
choices, and a fresh launcher reloaded them. Native automatic forms, local
side-by-side display and seven Both-door checks matched those choices. The
control pages displayed the stock keyboard bindings. This was a hidden, muted
automated check with no gamepad connected; physical two-device input and an
owner-facing window/audio handoff remain unverified. Neither screenshots nor
these bounded checks certify all stages or every enhancement.

The raw4/1 CP1 boss-room fixture skips an earlier door that uploads the alternate
background. Its retained initial atlas causes noisy scenery and black gaps.
A rendering-only native upload request on89E80004 restores coherent artwork;
it cannot validate the skipped door trigger. F95 remains unverified, including
that actual trigger and the separate raw1 black-sky coverage. This checkpoint
setup does not establish an ordinary-play or automatic-Ultimate rendering fault.


Later local candidate 4DB4A5F3 fixes the final Both-player door's early boss
introduction. The first native introduction now finishes with both players
present and controllable. Actual later-phase defeats, death/Continue and
endings remain unverified; use the individual burndown for those gaps.

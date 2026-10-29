# X5 v0.2.7 owner follow-ups

Local branch: `feature/mmx5-master-native-waits-local`.
Owning local issue: `beads-eio.10.44`. No remote publication or tracker sync.
The original comparative campaign remains parked. This list preserves each
owner report individually; a bounded pass does not qualify every stage or route.
The multi-stage batch adds20 entries below, for24 tracked reports total. A01's
recurring armor complaint remains one issue with additional stage checks.

| ID | Owner report / earlier reference | Bounded check and evidence | Status |
|---|---|---|---|
| A01 | Visible Alia still returns P1 X with corrupt armor. Reopens O03 for this build. | Native road/building episodes, then both corresponding states from the owner's exact v0.2.7 recording, with real page confirmations and P2 movement during return. | Fixed; bounded return graphics checks passed, including the supplied recording's states on current20e92f4c. Owner visual confirmation remains useful. |
| A02 | P2's view focuses on P1 while P1 teleports after a P2-owned hint. Owner clarified that Zero's pane actually moves away from Zero. | Exact original replay, all four P2-owned hints; retain500 consecutive presented images per interaction, with body/camera observations. Matched original/current building-state continuations also retain presented history while P2 moves during X's return. | Open owner-observed focus failure; not reproduced by these checks. Inspected views follow Zero. Requested the video's exact failing timestamp; the video itself remains inaccessible. No camera policy change or claimed fix. |
| A03 | Both doors work, but P2 returns to standing before the walk. Extends R02. | Actual Both admission/open/carry/close after original building and climb-area commands. Compare displayed sprite frames during opening separately from carry. | Fixed; bounded opening and carry checks passed. The final run sampled12 distinct displayed P2 frames in opening and13 in carry. No additional position/collision or door-clock changes. |
| A04 | With Alia skip enabled, trailing P1 misses respawning intro cars after leading P2 advances. | Silent-hint current setup: P2 encounters original class0D car736, advances the actual checkpoint0 to1, the original car retires offscreen, and trailing P1 reenters. | Car736 native offscreen-retirement/respawn passes, with both heroes alive. Broader reported recurrence and a defeated-car path remain unverified; exact failing encounter still needed. The new ZIP has skip off and cannot qualify the earlier skip-on run. No speculative spawn changes. `supplied-report/car-cycle-report.json`. |

## Multi-stage owner batch

Source: `E:/Downloads/Multiple stage bugs 0.2.7.zip`, SHA256
`3da85afa8e37570a9efa2d2c16ede8e360965fb21d8af4b96ebebf50950b0e2c`.
Session1793294523194715: original v0.2.7/bed0aeca, Both doors, split cameras,
Alia skip enabled, original starting cards and27MB input trace. It predates
the A01/A03 fixes. Evidence root:
`G:/Temp/Codex/psxrecomp/x5-parity/owner-027-multistage-20261029`.
Video: <https://youtu.be/VcSXm8-Isx0>; web fetch failed, not reviewed.

| ID | Individual report | Bounded reproduction / acceptance | Status |
|---|---|---|---|
| G10 | P2 train background is static and incomplete before the countdown area. | Original destructible train; compare animation intervals and complete layer art in both panes. | Open; require both animation and missing-part checks. |
| G11 | First P2-owned boss introduction kills hovering Fourth Armor P1 through the floor; retries return safely. | The supplied recording's first living-P1/P2-owned Grizzly introduction, native dialogue and passenger return. | Fixed in current local candidate; bounded recorded-state continuation passes after23 actual pages. X returns at9676.375/379 with native collision, Zero at9658.5/378; both alive. Original/door-only continuation instead lets airborne X keep falling during the hold. Fresh admission and owner visual retest remain useful. |
| K01 | Riders draw over the other hero's Ride Chaser. | Overlap both native bikes; check rider/bike packet order in both panes. | Open. |
| K02 | P2 cannot shoot bike blockades until P1 dies. | Both alive, real P2 bike shots, actual blockade HP/destruction; compare survivor path. | Open. |
| K03 | Alia dialogue still appears after bikes with skip enabled. | Execute that native hint, preserve orb-shooting action and progression while suppressing text/hold. | Open; an authored action does not by itself justify displaying the hint. |
| K04 | P2 Zero gets X's capsule dialogue when P1 X is dead. | Original capsule, dead P1, live P2 Zero; inspect actual recipient message bank. | Open. |
| K05 | The same surviving Zero cannot collect the capsule part. | After real conversation, actual top contact and native grant/return without setting progress flags. | Open. |
| K06 | Kraken boss doors do not use scripted traversal. | Original Both admission/open/carry/close and native control recovery. | Implemented through generic native Both carry; actual Kraken door check remains open. Grizzly native carry closes after both movement inputs are released, with both heroes alive. |
| M01 | P2 camera sticks during whale sequence. | Actual first whale sequence; check camera follows the eligible hero and retains authored stops. | Open. |
| M02 | Whale parts float after being hit. | Actual attacks/HP change and subsequent native part positions/parent relation. | Open. |
| M03 | Checkpoint respawn leaves P2 in a free-roam area. | Actual retry/next-checkpoint initialization and both authored camera limits. | Open. |
| M04 | First whale miniboss has missing P2 graphics. | Native fight admission and animated whale/layer art in both views. | Open. |
| M05 | Second vertical-scrolling section breaks. | Its original controller, scrolling, transitions and both views. | Open. |
| M06 | Both heroes die after jumping down. | Actual drop after the vertical section, native destination/collision and individual survival. | Open. |
| I01 | P2 cannot stand on chained spike platforms. | Native jump/landing/contact/carry on one original platform. | Open. |
| I02 | Leading P2 does not trigger stair-area Sigma viruses. | P2 crosses the real admission boundary; observe actual native virus allocations. | Open. |
| I03 | Leading P2 does not trigger the stair-area miniboss. | Real admission, fight start and control recovery with P1 behind. | Open. |
| I04 | P2 stair background is not animated. | Original stair controller across animation intervals in both panes. | Open. |
| I05 | Glow boss doors do not use scripted traversal. | Reuse native Both-door carry check at Glow's actual door. | Implemented through generic native Both carry; actual Glow door check remains open. |
| I06 | Defeating Izzy Glow with both alive freezes the game. | Actual native boss defeat, paired victory and real reward/return transition. | Open; progression blocker. |

Shared A01 follow-up: repeat retained-armor return qualification for a real
P2-owned stage dialogue, alongside G11. Keep original report history and do not
count release0.2.7 as containing the later source fix. All new entries remain
unverified until their mechanics run; loading a checkpoint does not pass them.

First multi-stage milestone: candidate SHA256
`2dd6574de46594f7b73b729c5d8f84da06eba2653f8f9fd9e18b5365494993f7`.
The native Both carry now applies to ordinary doors across stages rather than
only the intro. Nearby airborne companions use the existing outgoing/return
sequence; proximity alone no longer leaves a hovering/falling actor ticking
under the owner's room lock. On-foot returns discard the departed collision
suspension, restore the native standing box and release the passenger's old
command through original3D2C4. Duel-specific release remains separate.

`return-complete-candidate-check-report.json` passes the supplied Grizzly scene
through all23 pages, native beam/materialization and grounded recovery.
`return-fix-candidate-check-report.json` passes actual Grizzly Both admission,
opening animation, native carry and closing after input release. Its separate
scene case ended at a60-second observer limit while pages were still advancing;
the same binary passes with the established longer native-dialogue allowance.
Do not count that observer timeout as an additional game regression.25/25 CTests
pass after the source changes. Returned X armor is visually intact; the full
new-stage incoming-art path is not blanket-qualified from that final image.

The exact original multi-stage replay verifies a prefix through input150538,
then diverges at150539/game frame63968 during the post-Kraken transition. No
full-recording pass is claimed. Five original Grizzly/Kraken snapshots from the
verified prefix remain usable for bounded checks. Later stages will use native
checkpoint setup rather than repeated full-route replay variants. The initial
content inspection retained original placement tables, overlay bytes and images
for Grizzly, Kraken and McWhalen, but its requested snapshot slots21..23 were
outside the supported0..11 range; those are harness setup failures. The later
stage requests were rejected while McWhalen's authored scene was active. Neither
failure qualifies a stage mechanic. Use isolated native setups next.

Owner acceptance from2026-10-29: boss counterpart dialogue skip and complete
character whitening during the explosion pass; Both-door functional traversal
passes. Preserve these passes without broad repeat testing. The previously
partial continuous Dinorex graphics route remains separate in
`COOP_X5_026_FOLLOWUPS.md`.

## Implementation and limits

A01: the retained body painter already handled P1's compressed image, but a
private redraw still allowed native27370 to draw its armor from shared VRAM.
That VRAM can hold Zero during a P2-owned scene. P1 now uses the existing
retained armor painter too; the later duplicate singleton pass is suppressed.
The original assembly groups, sprite priorities and native palette fades remain
in use. This is a draw change; native scene timing and passenger movement stay
authoritative.

A03: P2's ordinary body pass is suspended during door opening. Its retained
animation previously advanced only after the original carry began. The same
native animation helper now consumes the entry program during opening as well.
An ended transient switches directly to the authored entry loop26. The helper
restores its player scope and does not advance movement or shared door clocks.

Evidence root:
`G:/Temp/Codex/psxrecomp/x5-parity/owner-027-reports-20261029`.
The baseline executable is the exact delivered v0.2.7 SHA256
`fea3ae1dd615d1e978ed8962635ad8af41fe4faf02f2b3b1833e81cee8b93576`.
`baseline-alia-return-report.json`, `candidate-alia-return-report.json` and
`combined-report.json` retain body/camera observations, actual original page
confirmations and setup snapshots; their session files identify binaries.
`ctest.log` records25/25 passing checks after the two fixes.

Final candidate SHA256:
`20e92f4c88e94b4545211f5832d552ea89f80380619cb0b49b9890f05227158c`.
The earlier isolated road-return run used the armor-only candidate; the final
combined building/door check includes both source changes. The building hint
uses original atlas2, P2's actual hint rectangle and native page confirmations.
Teleporting just before that rectangle bypasses its preceding physical obstacle;
it does not set hint or atlas results.

Three prepared owner cards are available at
`G:/Temp/Codex/psxrecomp/x5-parity/owner-027-reports-20261029/manual-qa/Validation.ps1`:
visible Alia armor/camera, Both-door opening/carry, and an Alia-skip-on fresh
start for the actual missing-car history. Each card has isolated notes and
current-build snapshots, normal window/audio, P1 keyboard and per-launch actual
DualSense targeting for P2. No owner-facing window is opened automatically.
Read individual notes before counting any later bundle acceptance.

Cheats position/protect heroes and admit original placement scans. They do not
write hint completion, checkpoint advancement, door closure or boss success
flags. Initial incompatible snapshots and obstacle setup failures are retained
separately; neither is a gameplay pass. Unattended sessions are hidden/muted,
have private disposable cards and use capacity reservations. Physical controller
ownership, LAN and a performance improvement are not claimed by these checks.

The delivered Nextcloud v0.2.7 archive is unchanged while these follow-ups are
qualified. Useful current setups/captures remain outside the delivery folder;
agent-created scratch and superseded copies are disposable after qualification.

Final verification: `final-gate.json` passes both actual building/door mechanics
and all three prepared setups' cold restoration/presentation. Its capacity
receipt declares1 physical CPU,20 percent GPU and2GiB RAM. Earlier GPU jobs
mistakenly declared0.15 percent rather than a realistic percentage; those
receipts do not establish adequate GPU reservations or performance. The final
gate corrects that declaration. No performance win is claimed.

`cleanup.json` records removal of task-local cache/scratch, a redundant runtime
profile and repeated text-page captures. Decisive before/after art, original and
candidate binaries, native setups, logs and the prepared station remain for
ongoing QA; these are bounded campaign evidence, not delivery-folder builds.

## Supplied v0.2.7 recording (2026-10-29)

Archive: `E:/Downloads/Opening stage bugs 0.2.7.zip`, SHA256
`7e1f73bc190568e0c4ac2c743452e9b4de2c8faf3908e6907da352d2528ccae5`.
Originals remain unchanged in `supplied-report/original` beneath the evidence
root. Main session1793277689363082 declares release v0.2.7, co-opbed0aeca,
execution6702ced5, Both doors, local8:3, widescreen off and Alia messages on.
It predates local fix7e39091; these are the same reports, not four new entries.
The other session contains only a few startup inputs. There is no restored-state
baseline in the archive.

`supplied-report/original-replay-report.json` verifies all15,593 recorded inputs
on the exact released executable, including state hashes and native read order.
It captures all four P2-owned Alia episodes and the actual Both door. The current
fix candidate then cold-loads the original road/building departures and opening
door state: `current-exact-report.json` passes all three continuations, with both
heroes alive, original page confirmations, actual P2 movement during return,
animated P2 opening/carry and native closure. No positions or success flags were
authored in these owner-state continuations. They are mechanic checks, not a
cross-build hash replay. No new code fix or new release is implied by a replay.

The distinct car check retains its original P2 allocation, actual offscreen
retirement/cleared allocation flag, and original new allocation on P1 reentry.
A bounded positive1HP/actual Z-Buster attempt did not land a qualifying hit, so
`car-defeat-report.json` is a setup failure, not evidence about defeated-car
respawning. Stop setup variants and keep that path open for a real failing
encounter or the prepared human route.

Video <https://youtu.be/0u_6jZeFJfA> could not be fetched by the web tool; an
ordinary local download attempt also returned403. It has not been reviewed.
The local diagnostic replay's composed captures are the visual evidence.
Useful exact owner setups, baseline/current images and reports remain local;
temporary download metadata/cache is disposable. Owner notes and the sole
delivered Nextcloud archive remain unchanged.

## Clarified camera report and displayed history

The owner confirmed actual loss of focus on Zero, rather than requesting earlier
independent cameras. A02 stays open. `supplied-report/focus-ring-report.json`
records matched original/current building-state continuations: each retains the
existing600-entry presentation ring after Zero moves during X's return. These
captures do not reproduce the reported focus loss.

`supplied-report/focus-full-history-report.json` verifies the original15,593
inputs again and retains500 consecutive presented swaps around each of the four
actual Alia interactions. A host-only hold after each completed interaction lets
the existing ring be exported without losing later gameplay frames. Overviews
and the largest adjacent-frame changes during recovery were visually inspected;
neither that review nor sampled cameras reproduced the failure. Image ranking
is inspection triage, not an automatic correctness pass. Renderer watchdog,
aborted-pass and VRAM-leak counters remain zero. No new instrumentation, engine
change, camera policy change or new source fix was introduced.

An earlier exploratory export froze the image ring while gameplay continued,
leaving gaps during export. It is superseded by the full-history run and is not
evidence of uninterrupted visual coverage. Its redundant payload/captures and
task scratch are disposable; retain the decisive full-history images/reports,
original/current identities and prepared owner cards. The prepared checklist's
stale candidate label and car-check description were corrected; owner notes
were not changed. A04's broader missing-enemy route and defeated-car path also
remain open. No further setup variants were attempted.

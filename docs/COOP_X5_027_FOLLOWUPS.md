# X5 v0.2.7 owner follow-ups

Local branch: `feature/mmx5-master-native-waits-local`.
Owning local issue: `beads-eio.10.44`. No remote publication or tracker sync.
The original comparative campaign remains parked. This list preserves the four
new reports individually; a bounded pass does not qualify every stage or route.

| ID | Owner report / earlier reference | Bounded check and evidence | Status |
|---|---|---|---|
| A01 | Visible Alia still returns P1 X with corrupt armor. Reopens O03 for this build. | Actual P2-owned road hint from a fresh v0.2.7 New Game; native pages, departure and return captured before/after. Repeat at the building after its actual original atlas admission. | Fixed; bounded return graphics checks passed. Owner visual confirmation remains useful. |
| A02 | P2's view focuses on P1 while P1 teleports after a P2-owned hint. | Road and building hint episodes: retain body positions, scene phases, both cameras and composed return images. The building owner stays at x2852 and P2 camera x2692 throughout phases3/4/5. | Unverified owner recurrence. These checks follow P2 correctly; do not mark the report passed or invent a camera fix. Exact latest recording requested. |
| A03 | Both doors work, but P2 returns to standing before the walk. Extends R02. | Actual Both admission/open/carry/close after original building and climb-area commands. Compare displayed sprite frames during opening separately from carry. | Fixed; bounded opening and carry checks passed. The final run sampled12 distinct displayed P2 frames in opening and13 in carry. No additional position/collision or door-clock changes. |
| A04 | With Alia skip enabled, trailing P1 misses respawning intro cars after leading P2 advances. | Genuine native New Game with skip on, P2 advances checkpoint0 to1 while P1 stays at x144, then P1 approaches the original class0D car736. Distinguish first admission, activation, retirement and respawn. | Unverified owner recurrence. Original car736 does appear at checkpoint1, with both heroes alive. The original P2 encounter/retirement/re-entry history remains needed; no speculative spawn changes. `checkpoint-cars-report.json`. |

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

# X5 Grizzly train owner QA burndown

Focused campaign opened 2026-10-25 on `feature/mmx5-x4-parity-local`.
Local only. This supplements the frozen X4 comparison campaign; it does not
change the 234-comparison denominator or reopen unrelated manual checks.

Owner evidence: `Grizzly train bugs.zip` / [first recording](https://youtu.be/N2HLLoMPSSE)
and `More Grizzly bugs.zip` / [Alia-on recording](https://youtu.be/aZoaH0i66Lc).
Both archives identify build 084830CA, execution `5ca1047f`.
Raw evidence and reproduction receipts are retained under
`G:/Temp/Codex/psxrecomp/x5-parity/owner-grizzly-20261025`.

## Individual checks

| ID | Report / failure mode | Bounded check | Status / evidence |
|---|---|---|---|
| G01 | Hidden Alia messages leave the laser cannon invulnerable, including solo. Suppression retired the actor before native admission and lost its heard bit. | Compare native hint admission/completion, then actually damage and destroy the main cannon with messages hidden. Include solo because co-op is not required for the report. | Passed after owner82A36EAA solo station Pass (?alia is fixed?); earlier receipt: Owner reports current co-op Grizzly works after the requested main-cannon/camera check on170B32A6, 2026-10-25. Actual P2 hint9 admission/heard bits and hurt-box activation have earlier automated evidence. No solo pass is inferred from the co-op fixture. Failed automated shot setups remain recorded. |
| G02 | Train-front camera stops can be crossed while both players live. Owner says they work after either player dies. | Bring P2, then P1 to a live cannon boundary; confirm each native stop before destruction and release after an actual defeat. Use one-survivor behavior as a bounded reference. | Passed owner co-op retest on170B32A6, 2026-10-25: after the explicit cannon/stop/release instructions, owner reports Grizzly seems to work now. X/P1 keyboard and Zero/P2 DualSense, Both alive, Either doors and default Alia suppression. Owner recording retained separately; this is manual acceptance, not a new automated cannon-defeat pass or broad stage completion. |
| G03 | P2 does not start the exploding-train countdown. | P2 crosses the original trigger while P1 remains behind; observe the actual countdown and its native consequences. | Passed, bounded automation on39DA2E69: native CP2 setup; P1 at4496, P2 enters hint10 at4536. Previously clear heard bits become set through native admission; grounded train actor changes phase7/entered7E/parent87 to1. Native GO/countdown and actual explosion effects are visible. No hint, timer or explosion flag is seeded. Timed route traversal is bypassed. |
| G04 | Final escape train is absent for P2 at the end of the timed area. | Reach its native spawn window as P2 first; observe spawn, authored boarding and continuation. | Passed, bounded automation on39DA2E69: original reserved controller admits P2 at7424 while P1 stays behind; native class10/sub80 train appears, command1E carries P2 through7730..8632, original checkpoint3 activates and P1 returns grounded/alive with controls. Position/HP/contact setup bypasses the timed floor route; this does not qualify normal route timing or ordinary platform contacts. |
| G05 | Electric chains are static. Native area9 advances source rectangles in the retail tile ring; the full-map renderer was drawing static source tiles. | Observe the same native chain across multiple animation intervals in both panes; check the native clock and private graphics without ticking the world twice. | Passed on39DA2E69: actual wire-shape changes in both panes across12 composed captures; native actor frame1B selects the18 original rectangles, including pulse frames15..17. `fifth-trains-chains/chain-pixel-qualification.json`. No new clock or persistent simulation state. |
| G06 | P2 enemy hit effects are corrupted because static character-sheet glyphs outlive the P2 graphics projection. | Strike an enemy with normal P2 attacks; inspect actual damage, effect ownership/resources and composed frames. Ordinary P1 attacks also run in the paired regression. | Passed, bounded automation/visual review on39DA2E69: normal Zero/P2 slashes damage the native gun24->4HP; retained P2 static-hit glyph draws increase, both heroes survive and private drawing does not leak state. `grizzly-stage-fixture/actual-p2-hit-0.png` and `actual-p2-hit-3.png` show clean white/gold impact art and the normal green blade. Other weapons remain part of owner route acceptance. |
| G07 | Capsules appear to work. Preserve that behavior. | Observe actual native grant, both controls and return in the same G09 session. | Passed on170B32A6: actual capsule dialogue/sequence sets native PLAYA1 from0 to8 through original55DC8; both heroes return visible/alive, both move from real input. Route is bypassed by CP3/position setup. grizzly-qualified-final/session.json records this passed individual check; its other P1-door setup failed separately. |
| G08 | Either hero can become invisible when the other triggers Grizzly; actual leader ownership matters. | Repeat each leading seat with an unbeaten Grizzly; observe actual intro, passenger visibility/position and controls. | Passed, bounded on170B32A6, Either doors: P2-leading case in grizzly-qualified-final/session.json and true P1-leading case in grizzly-true-p1/session.json both finish the actual intro with both heroes visible/below the upper ledge and passenger controls restored. Earlier P1 setups placed Zero within the aperture, so he touched first; with the passenger behind at9152, P1 actually owns admission. Prior39DA2E69 Both/P2 pass remains recorded. No cleared91-card boss revisit is counted. |
| G09 | After the capsule conversation, P2 briefly shows P1 HUD. | Capture the actual capsule return and identify each pane HUD during unlocked passenger recovery. | Passed, bounded on170B32A6: the real capsule grants A1 bit08 and returns both controls. Recovery scene1/phase5 has native lock0; private pair draws continue with X HUD left, Zero HUD right at the same height (grizzly-qualified-final/capsule-phase-5.png). The authored camera stays shared until return completes. The teleported upper-room backdrop is not normal-route graphics qualification. |

Current inventory: **9 passed / 0 open / 9 total**. The owner's current co-op
cannon/camera acceptance closes G02 and the co-op portion of G01. Owner82A36EAA solo station Pass (?alia is fixed?) accepts G01. The final automated shot
attempt remains failed reproduction evidence; owner acceptance does not turn
that harness into an automated pass.

The current candidate also has native X-owned armor selection, always-available
Fourth and Zero Buster; its separate seven-check list is in
[COOP_X5_ARMOR_SELECT.md](COOP_X5_ARMOR_SELECT.md).
The frozen234-comparison campaign remains unchanged. No full parity is claimed.

The owner's later health-pickup pause, garbled Zero Triangle projectile and
existing R9 airborne dash trail reports have local fixes recorded in
[COOP_X5_POST_GRIZZLY_QA.md](COOP_X5_POST_GRIZZLY_QA.md).
Pickup automation passes; the graphics fixes await the owner's visual retest.
The closing-walls report was withdrawn; it adds no check to this list.

## Bounded strategy

1. Freeze and index the original recordings; reproduce the important portions on
   the exact original build with logger hash/read-order verification.
2. Resolve native Alia side effects separately from two-player stage ownership.
3. Bundle camera gates, timer, escape train and chains on one train-route setup;
   reuse combat there for hit effects and preserve the capsule check.
4. Validate the combined candidate and current-build save/restore, run relevant
   regression checks, commit a local milestone and prepare an owner retest.

Add cases only for distinct mechanics, ownership rules or observed failures.
Cheats may position or protect players; never set the result that a check tests.

Owner agreed direction: finish Grizzly as one stage-sized batch, then obtain a
two-player route pass through timed trains and the boss entrance before moving
to another stage. Automated setups and overlapping mechanic checks precede the
owner pass. X-owned stage armor selection is implemented separately; see its individual
checks and remaining manual visual/LAN acceptance above.

## Evidence discipline and remaining owner bundles

Both original084830CA owner traces reproduce completely with native input,
read-order and CRC verification:19,717 and38,580 intervals. Original videos,
archives, parsed RAM observations and failed setup receipts remain in the evidence
root. Failed harnesses are not aggregate passes: qualify the individual mechanic
from its actual observations. In particular, admitting the later escape script
does not prove the earlier exploding-train countdown.

Ordinary cannon setup investigation is timeboxed. G01/G02 shared one owner
co-op run: the170B32A6 manual acceptance is now recorded above. Repeat the
hidden-hint destruction in solo because it has its own reported failure.
No more speculative attack-position variants are required before that check.

G07/G09 share a capsule run: activate the capsule, check the grant/control return,
and watch P2's HUD during the return to independent views. A final two-person
stage route is still needed for real timed-train play, combat polish and LAN;
the bounded automation does not replace that acceptance pass.

2026-10-25 armor candidate5E0BCCE0: one final source-qualified weakpoint shot
check was attempted after the original hint enabled the main hurt box. The
cannon was not defeated and P1 died during setup. This is failed reproduction,
not defeat/release evidence. G01/G02 remain deferred for manual validation;
do not continue speculative attack-position variants. The first capsule setup
also placed P2 over unsafe terrain and triggered a scene before a second
teleport; that receipt is unqualified. Follow-up uses the current-build native
snapshot, retains P2 on the actual checkpoint floor, and restores independently
before capsule and each leading-seat door check.

170B32A6 qualification supersedes the first failed capsule setup: keep P2 on
the real checkpoint floor, scan the original placement, then enter as X.
The actual grant is PLAYA1 bit08, not the complete-armor availability bytes.
Cold fixture restore isolates each capsule/door case. Individual passed checks
are preserved even when a different case made the harness aggregate fail.
The P1-leading door pass uses a genuinely trailing passenger outside the
aperture; no visibility, death, intro-complete or boss result is seeded.

Owner82A36EAA notes are preserved in owner-input-walls-20261026. New solo background-scroll pauses and coupled physical inputs are separate .10.29/.10.27 findings; this acceptance does not prove independent device ownership. LAN deferred.

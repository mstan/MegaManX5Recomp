# X5 v0.2.5 owner follow-ups

Local branch: `feature/mmx5-master-native-waits-local`; Beads `beads-eio.10.40`.
Source starts at543801c, delivered runtime SHA256f93e1410 and execution6702ced5.
Owner archive: `E:/Downloads/Opening stage bugs 0.2.5.zip`; extracted evidence
under `G:/Temp/Codex/psxrecomp/x5-parity/owner-025-reports-20261027`.
All three supplied sessions match the delivered execution/co-op identities.
Owner settings use split local cameras, Both doors, separate E-tanks, keep Zero
and Black Zero preference, with the optional widescreen feature disabled.
The linked YouTube page could not be fetched; do not claim video review.

This is a ten-item follow-up, independent of the parked historical comparison
campaign. Grizzly looks good to the owner; their additional testing is pending.

| ID | Individual report / intended behavior | Bounded check | Status |
|---|---|---|---|
| V01 | P1 can select any hero/form, then P2 selects a form of the remaining hero. One X and one Zero; retain native selector and capsule-gated Black Zero. | Native card Continue, both selection orders, wrong-seat confirmations, remaining-only lists and actual entries. Locked save hides Black; disclosed unlocked UI fixture tests P1 form choice and current-build restore there. | Passed, bounded native checks on2f7cef8a. `selection-report.json`, `unlocked-picker-report.json`. Capsule acquisition and cold card persistence are separate. |
| V02 | P2 entering the falling-debris building changes P1's background. A shared atlas/controller command must not change the trailing view's region. | Fresh native New Game; wait for original camera/viewport admission, then P2 Right across2728..2751. Native atlas completes2; P1 camera remains city/mode6 and P2 building/mode2, both alive. Composed image inspected. | Passed on2f7cef8a. `native-boundary-report.json`, `native-boundary/after-atlas.png`. |
| V03 | P2 does not trigger falling debris. The native placement/controller must admit either eligible hero. | P1 remains far behind; P2 position bypasses the block, then the original slab arms once with acceleration1000 and both heroes alive. | Passed on2f7cef8a. `intro-report-native-anchor.json`; no full-route or retry claim. |
| V04 | P2 wall-contact dust appears at P1. Personal effect updates must retain their allocator and position. | Same episode as V03: actual P2 wall-contact input, native class3/sub1 effect near P2 and over500px from P1, deferred callback runs. | Passed, bounded owner/position check on2f7cef8a. `intro-report-native-anchor.json`, composed wall-dust capture. |
| V05 | P2 freezes during Both-player boss-door carry. Both bodies must retain the native animation while shared movement advances once. | Actual native Both door, animation cursor changes through carry and native control recovery. | Unverified on this candidate. Earlier accepted implementation is retained, but checkpoint/teleport shortcuts did not establish the native roof floor/closing-structure prerequisite; no forced result or carry pass. |
| V06 | Skip opening-stage boss dialogue in co-op. Preserve intact statue, reveal/explosion, boss initialization and control return. | Actual door-to-reveal-to-fight sequence without counterpart meeting or subsequent dialogue stops; solo unchanged. | Implemented, runtime unverified. Binds only the exact original Sigma post-reveal text request; same roof prerequisite as V05. |
| V07 | Dinorex background becomes corrupt. Preserve each region's native atlas, origins and upload sequence. | P1 triggers the original lower-area map/atlas while P2 stays on the starting ledge. Compare both images; verify canonical replacement bytes, private journal and current-build cold restore. | Passed on39c21d7c for the reproduced part1 atlas boundary. `dinorex-atlas-corrected-v2-report.json` records the corrected image and native upload hash; `dinorex-atlas-cold-final-report.json` passes restore. No full-stage graphics claim. |
| V08 | P2 does not trigger the Dinorex flame wall. | P1 stays behind while P2 crosses the original fire trigger; observe native fire motion and barrier destruction. | New owner report, unverified. |
| V09 | P2 camera bypasses the barrier the flame wall must destroy. | Bundle with V08: P2 camera remains at the authored gate, then releases after actual native destruction. | New owner report, unverified. |
| V10 | Provide a second Ride Armor for the armor-only route in Dinorex part2. | Two original suits with separate riders and controls; mount, traverse the lava route and dismount without transferring ownership. | Requested feature, unimplemented. Current code deliberately supports P1-only Armor; requires a bounded ownership change. |

First check the current native-overlay path against the previously accepted
interpreter-backed co-op fixes. Do not accumulate independent patches before
checking that common failure mode. Use one combined intro/building/door pass
where possible, plus the distinct selection flow. Cheats establish setup only;
never write the scene/door/debris success flags being checked.

The later `Opening stage bugs 0.2.5 (1).zip` is byte-identical to the first
archive (SHA25694cf4d93c786ae192384dc062ad59606b9cdf2741644359438b948ee23e7591e).
It supplies no additional Dinorex trace. The new Dinorex video could not be
fetched; the owner's descriptions remain the report, not a claimed video review.
V08/V09 use native raw4/0 fire/gate mechanics; V10 uses raw4/1 original Armor.

## Current local evidence

Five validated / five open / ten total. Current candidate is39c21d7c6bc013781d39569cebf2846173fd843718eef5526aaac2bcae87e18f,
versionv0.2.6-coop-alpha-debug;25/25 title CTests pass. V01/V03/V04 were bounded
checks on the earlier2f7cef8a candidate. The generalized atlas code also passes
the original native intro boundary on39c21d7c (`native-boundary-dino-regression-report.json`).
These results do not authorize replacing the delivered0.2.5 archive.

The Dinorex original subtype0 callback repeats request1 while the upload
completes2. Two initial cold-restore observers incorrectly required an
asynchronous later RAM sample to remain2; their assertion failures are retained.
The final check requires actual native completion, correct composed views,
canonical replacement SHA2560d38231c621a7a4d8165c585251da0fba6b7e8c41451baf87cbd660ff47c733e
and clean private-view journals together. Both current-build restored views were
visually inspected. No request/result field was changed by the observer.

Exact same-binary/snapshot/config A/B (`overlay-probe/results.json`) found native
overlay cache ON bypasses the selector hook (roster000105, zero P1 hook calls),
OFF restores it (filtered0001, four hook calls). The shipped native package also
stripped canonical hook metadata and used the default full-function emitter;
this does not isolate an SDK-only defect. Disable that unqualified path by
default, retain audited resident native waits, and preserve the metadata for
future qualification. Windows/Linux release staging and the prepared playtest
tool now respect this choice rather than requiring an unused native cache.
Original performance wins from the unqualified package cannot substitute for
matched measurements of the corrected delivery candidate.

P1's complete native list fits the original six slots. Choosing Zero after the
genuine unlock uses its existing Normal/Black selector before P2's X turn;
there is no seven-slot overwrite or new menu design. Locked source cards retain
their genuine bit. The unlocked check sets an explicitly disclosed UI input
fixture and claims no capsule collection or native save migration.

Setup failures remain in their reports. In particular, moving P2 before its
camera/viewport catches up skips the narrow intro subtype80 trigger; waiting
for actual native admission fixed the observer. CP2/CP3 or a roof teleport alone
does not establish the original closing structures' completed roof collision.
Do not spend more variants on that: use a genuine climb for the next door/reveal
check. Native New Game story pages are now confirmed with separate ready-page
edges, allowing fresh current-build intro setups without a human startup seed.

Dinorex source/probe: raw4/0 uses BIN member5/logical9. Original area callback
EEAF0 checks CAMERA.Y against F4828+12 (2192), then PLAYER.Y against2228 before
its original28B18 map copy and E92F8 request. In `dinorex-fire-baseline-report.json`,
P2 is alive at160,2330 with camera2192 and that class1/sub0 actor stays idle while
P1/camera remain635/512. P1's matching native control executes it and exposes
the upper-view atlas corruption. This is a source-qualified area prerequisite,
not yet the actual flame chase. Do not simply project this closing-map callback
under a distant P2: it can close terrain while P1 is still above. An initial
unqualified wrapper was removed before staging. The atlas correction is draw
only; identify and test the actual flame controller for V08/V09 separately.

The native part2 start has one initialized Armor at296,400 in physical slot0;
slot1 is empty there. The original content later spawns slot1 at3997,379.
Adding an early second suit must preserve native slot pointers and handle that
later placement, rather than overwriting an occupied rider. Two independent
riders remain unimplemented (V10); current behavior is deliberately P1-only.

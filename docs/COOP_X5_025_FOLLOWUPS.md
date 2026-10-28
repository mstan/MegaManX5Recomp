# X5 v0.2.5 owner follow-ups

Local branch: `feature/mmx5-master-native-waits-local`; Beads `beads-eio.10.40`.
Source starts at543801c, delivered runtime SHA256f93e1410 and execution6702ced5.
Owner archive: `E:/Downloads/Opening stage bugs 0.2.5.zip`; extracted evidence
under `G:/Temp/Codex/psxrecomp/x5-parity/owner-025-reports-20261027`.
All three supplied sessions match the delivered execution/co-op identities.
Owner settings use split local cameras, Both doors, separate E-tanks, keep Zero
and Black Zero preference, with the optional widescreen feature disabled.
The linked YouTube page could not be fetched; do not claim video review.

This is a fourteen-item follow-up, independent of the parked historical comparison
campaign. Grizzly looks good to the owner; their additional testing is pending.

| ID | Individual report / intended behavior | Bounded check | Status |
|---|---|---|---|
| V01 | P1 can select any hero/form, then P2 selects a form of the remaining hero. One X and one Zero; retain native selector and capsule-gated Black Zero. | Native card Continue, both selection orders, wrong-seat confirmations, remaining-only lists and actual entries. Locked save hides Black; disclosed unlocked UI fixture tests P1 form choice and current-build restore there. | Passed, bounded native checks on2f7cef8a. `selection-report.json`, `unlocked-picker-report.json`. Capsule acquisition and cold card persistence are separate. |
| V02 | P2 entering the falling-debris building changes P1's background. A shared atlas/controller command must not change the trailing view's region. | Fresh native New Game; wait for original camera/viewport admission, then P2 Right across2728..2751. Native atlas completes2; P1 camera remains city/mode6 and P2 building/mode2, both alive. Composed image inspected. | Passed on2f7cef8a. `native-boundary-report.json`, `native-boundary/after-atlas.png`. |
| V03 | P2 does not trigger falling debris. The native placement/controller must admit either eligible hero. | P1 remains far behind; P2 position bypasses the block, then the original slab arms once with acceleration1000 and both heroes alive. | Passed on2f7cef8a. `intro-report-native-anchor.json`; no full-route or retry claim. |
| V04 | P2 wall-contact dust appears at P1. Personal effect updates must retain their allocator and position. | Same episode as V03: actual P2 wall-contact input, native class3/sub1 effect near P2 and over500px from P1, deferred callback runs. | Passed, bounded owner/position check on2f7cef8a. `intro-report-native-anchor.json`, composed wall-dust capture. |
| V05 | P2 freezes during Both-player boss-door carry. Both bodies must retain the native animation while shared movement advances once. | Actual native Both door, animation cursor changes through carry and native control recovery. | Passed by owner: "both player boss door animation works." Preserve the new backward-drift/death report separately as V14; this is not blanket door acceptance. |
| V06 | Skip opening-stage boss dialogue in co-op. Preserve intact statue, reveal/explosion, boss initialization and control return. | Actual door-to-reveal-to-fight sequence without counterpart meeting or subsequent dialogue stops; solo unchanged. | Passed by owner: "statue reveal and diagloue skip pass." Prior setup failures remain evidence of unqualified shortcuts, not successful route checks. |
| V07 | Dinorex background becomes corrupt. Preserve each region's native atlas, origins and upload sequence. | P1 triggers the original lower-area map/atlas while P2 stays on the starting ledge. Compare both images; verify canonical replacement bytes, private journal and current-build cold restore. | Passed on39c21d7c for the reproduced part1 atlas boundary. `dinorex-atlas-corrected-v2-report.json` records the corrected image and native upload hash; `dinorex-atlas-cold-final-report.json` passes restore. No full-stage graphics claim. |
| V08 | P2 does not trigger the Dinorex flame wall. | P1 stays on the checkpoint ledge while P2 crosses1137 and descends naturally. Original240/180-tick admission runs, then an actual burst; separate cover, death and current-build cold-restore checks. | Passed, bounded flame/cover checks onca61eab6; death and current-build live/dead cold restores also pass onb11c5082. `dinorex-fire-atomic-cover-report.json`, `dinorex-fire-postchecks-final-report.json`. Actual barrier destruction is V09, still unverified. |
| V09 | P2 camera bypasses the barrier the flame wall must destroy. | P2 camera remains at the authored gate, then releases after actual native destruction. | Partial owner confirmation: "dinorex fire barrier camera seems to work." Stop/destruction/release clarification pending; do not infer all three from that statement. Class24/sub1 is the actual barrier; class14 smaller flame effects are separate. |
| V10 | Provide a second Ride Armor for the armor-only route in Dinorex part2. | Two original suits with separate riders and controls; native mounting, steering, first lava drop and dismount without transferring ownership. | Passed, bounded native checks on ebde0959 and owner acceptance: "2x ride warmor works." `two-armor-final-controls-report.json`. Complete alternate-route traversal and late backup recovery are outside this acceptance claim. |
| V11 | After X drops in Dinorex part1, Zero cannot follow through the floor. | Original floor seal waits for both living bodies to descend; actual follower movement opens the original native copy/retirement. A dead partner must not block the seal. | Passed on83044bec. Old ebde0959 reproduces the trapped follower. Current candidate admits both living heroes; the one-living check observes the original20 map words copied and controller retirement. `drop-follow-candidate-report.json`, `drop-one-living-ram-report.json`. |
| V12 | X alone dying to lava appears to restart Dinorex despite a surviving Zero. | Observe the actual death order, then test native lethal contact with a known living survivor and retained control. | Exact lava case unverified. The owner's recording has Zero die before X in all three retry pairs. Current native primary flame death leaves P2 alive, advancing and controllable for90 ticks. `drop-follow-candidate-report.json`; this is not a literal lava pass. |
| V13 | P2 shows the smaller flames but misses the full-screen Dinorex fire sheet; owner later reports the sheet disappears after X dies. | Same-location living/living burst, P2 ownership with P1 above, native clearing and cold restore; additionally reproduce the recorded early X death before Zero enters the lower corridor. | Bounded passes on715005eb and owner "burn is fixed" covered the living/living case. The later owner survivor failure reproduces by exact original replay (2181 verified inputs). Draw-only correction258e2aa7 uses the survivor world's authoritative native layer instead of stale pre-death fields. Both spectator panes show matching full fire and native clearing; Zero survives and the world continues. `survivor-draw-fixed-report.json`. Owner survivor retest pending. |
| V14 | Zero drifts backward at intro Both-door admission, walks into the closing door and dies after the scene. | Retain the original opening/carry, apply a disclosed backward position setup, observe collision suspension and actual closing-plane clearance. Full scene/death validation remains separate. | Implemented; bounded native opening/carry/clearance pass on7530c12e. Both collision boxes share the original suspension, conveyor drift stops, the companion retains its native animation, and one actual crossing restores both boxes with positive closing-plane clearance. Full authored cutscene and post-release survival need owner retest. `intro-door-drift-clearance-report.json`. |
| V15 | Substantial slowdown while Dinorex's fire effect is active. | Match the actual covered burst across LLE/HLE builds, then separately check non-fire gameplay and the final owner's setups. | Partial: existing HLE batching substantially improves the matched loaded-host fire workload in both ABBA comparisons. Final candidate also confirms the lower cost;25/25 tests, current cold states, native clearing and input isolation pass. Stable60fps and the owner's smoothness judgment remain unverified. LLE remains explicitly buildable; no shared-engine source change. `fire-batch-matched-abba-report.json`, `batch-nonfire-control-report.json`, `final-fire-owner-setups-report.json`. |

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

Eleven bounded validated / three partial / one unverified / fifteen total.
Four entries remain open: V09 needs clarification of stop/destruction/release;
V12 needs confirmation of the exact X-only lava death with a surviving Zero;
V14 needs full authored cutscene/post-release confirmation of the implemented
backward-drift correction; V15 needs owner confirmation of the measured slowdown
improvement. V13's new
survivor case passes bounded automation but still awaits owner confirmation.
The owner tentatively places the
Dinorex corruption in part1, matching the reproduced V07 atlas boundary.
Atlas candidate is39c21d7c6bc013781d39569cebf2846173fd843718eef5526aaac2bcae87e18f;
flame candidate isca61eab614ed66bf29d958f4c45a74f0a771e39edb5b54906f05e073c50d8ec3,
final fire candidate isb11c50823c164c86b397c06c671bf32e199dd11377cacee7234a5debf13f0960,
versionv0.2.6-coop-alpha-debug;25/25 title CTests pass. V01/V03/V04 were bounded
checks on the earlier2f7cef8a candidate. The generalized atlas code also passes
the original native intro boundary on39c21d7c (`native-boundary-dino-regression-report.json`).
These results do not authorize replacing the delivered0.2.5 archive.

The literal fire is primary enemy25, distinct from the closing-map area.
Nearest-body AI kept selecting P1 behind the controller even after P2 entered
the corridor. Select its actual admitting hero and run the original controller
once against that hero's camera. Retain that camera through an active burst
when its hero dies; a distant survivor's view must not relocate the fire.
The retained context entered snapshot schema14 and the simulation digest;
the current two-Armor candidate uses schema15. Old states remain disposable;
the fire setups were booted from real native
card/selector confirmations, not converted or forced through validation.

An intermediate camera wrapper copied an old P1 view over current native layer
initialization and disabled the layers. Before/after camera-byte comparison
identified the error; P1 now keeps the current authoritative camera. Failed
reports and the unqualified intermediate binaries' hashes remain in the local
evidence. The corrected native descent reaches the lower corridor with both
heroes alive. A meaningful setup stop is the original lower camera2656 and
X>=1408; a previous arbitrary1536-pixel stop also timed out after enemy knockback.

For the active burst, read the original80028FF0 map/block indexing and F29B4
cover bit8000. A position-only cheat moves P2 into authored cover near2336,2842;
no immunity, fire, timer, map, barrier or result flag is granted. Prepare that
position fixture before the burst and publish its single sequence byte when
the native flame starts: assembling it over many TCP/frame round trips after
the lethal front starts was too slow. This is setup timing, not another game
variant. Both heroes survive the actual covered burst. Separate unprotected
P2 burn death and current-build live/dead-owner cold restores pass without
moving the fire into P1's distant camera. LAN and full-stage traversal remain
unqualified.

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

The native part2 start has one Armor in physical slot0; the original content
later creates slot1 at3997,379. The extra early suit uses the original EE988
factory and its original initializer in physical slot1, with its own eight-byte
placement record. It does not clone initialized pointers or consume the late
backup. A later placeholder cannot overwrite an already active physical slot.
Each co-op seat controls only its matching physical suit. The original pool
dispatcher retains scene/freeze policy; active P2 runs input, vehicle, then
body, while inactive seats and scenes retain the shared dispatcher.

Current candidate ebde0959f75001c50e1b6632ea7ecae4e3c0f7c76420b740c2842da59da56207
passes25/25 title CTests. On the same candidate, actual native boarding ends
with P1 FF and P2 FE, as original39AC8 negates pending slot1/2. Return anchors
check the matching marker and occupied suit rather than borrowing another
rider's floor. The unique placement record is explicitly serialized and hashed.
The mounted snapshot restores in a fresh process; P1 steering leaves P2's
suit stationary and vice versa, and coherent native/world clock deltas match.
Both suits move and jump from the start across the first lava drop and dismount
through the original controls. Composed images were inspected and private
journals remain clean. No immunity, rider, occupancy or result flag is set.

Earlier failed probes are retained. Falling directly onto the extra suit's
helmet does not establish its native boarding overlap; the successful bounded
check uses a disclosed position-only overlap and waits for the full original
animation. The owner subsequently accepted the requested two-suit behavior.
This does not establish the entire alternate route, and another complete-route
playthrough is not required to accept the requested second usable suit.
Full-route recovery after losing a suit and the authored late backup also
remain outside this first-window mechanics check.

## Dinorex follower drop and apparent premature retry

The owner's frame2015 screenshot places X at1685,2843 and Zero at1573,2618.
The subtype1 EEAF0 controller seals a20-tile row atY2640 after its projected
player descends below2692, so the leading player can close the trailing hero's
floor. The correction holds only that exact native seal while any living,
positive-health co-op body remains above2692. When all living bodies are below,
the original callback performs its original map copy and retires. Subtype0's
earlier atlas transition and the native floor-copy/result fields are untouched.

`drop-follow-baseline-report.json` reproduces the blocked follower on ebde0959.
On83044bec, actual P2 Right crosses the opening, both heroes remain alive and
the native controller completes. `drop-one-living-ram-report.json` separately
observes the original20 destination map words copied from the original source
when only P1 is alive. Two earlier probes incorrectly expected a function trace
entry for the directly executed native copy; they are observer failures, and
the decisive check reads the actual map row instead. No success flag is set.

The owner's recording contains three retries: Zero reaches zero health at
native-input620/2169/3189, before X at624/2175/3190. Therefore that recording
does not establish a retry with a living survivor. A separate current-build
primary flame death keeps P2 alive, advances the world90 ticks and permits its
native jump. Preserve the exact lava report as unverified instead of applying
a speculative progression change. The later read-only live capture was taken
after the owner had switched to part2 and is not the screenshot's failure RAM.

Current floor-fix executable SHA256:
`83044bec7e7cd9d35214903a9714bad9fb4574d41819a1d5244a31a7ad374d6c`.
Snapshot schema remains15;25/25 title CTests pass. The owner finished the run,
so the prepared station can be refreshed without interrupting active testing.

The station is refreshed to83044bec with newly saved current-build fixtures.
Four fresh-process checks pass: whole-game pause retains both living heroes,
resume works, synthetic P1/P2 inputs move only their owning hero, and no render
watchdog, aborted journal or VRAM leak is reported. Evidence:
`owner-floor-current-setups-report.json` and
`owner-floor-global-pause-qualification.json`. This verifies prepared setup and
TCP input ownership; the brief physical keyboard/DualSense isolation check is
still part of the owner handoff. Per-launch scanning selects the actually
connected USB/Bluetooth DualSense; development input merging is disabled.

The on-screen checklist has three required cards: genuine intro climb/door/
reveal, the reported follower-drop/single-hero-death location, and the real
flame-barrier approach. Its fourth mounted-Armor route card is optional.
Every card starts globally paused with Resume test / Pause both buttons;
native Start alone does not freeze both heroes. Prior notes, recordings and
the exact ebde0959 owner-tested binary/configuration remain retained outside
delivery folders. No Nextcloud archive or remote tracker was changed.

## Full Dinorex fire sheet in the second view

The owner confirmed the follower drop and supplied a new same-location image:
P1 has the large fire sheet while P2 draws only the smaller effect actors.
The retained83044bec baseline reproduces this in a real native covered burst.
Native F302C/F3338/F354C publishes one background layer's visibility, origin,
wrapping and parent relationship to the admitting camera. Private P2 draws
retained the inactive cave layer instead. Project those already computed fire
fields only inside the affected lower-corridor draw transaction. Preserve its
native parent relation, actual fire range and per-view foreground camera;
do not rerun the hazard or change its timing, damage or map state.

An initial candidate accidentally used the simulation-only stage guard, which
excludes private draw scope and made the correction a no-op. Its captured image
is retained; it is not a V13 pass. The corrected candidate separates the shared
content/signature predicate from simulation admission. Its same-viewpoint
captures have matching fire-sheet coverage in both panes, native clearing
remains matched, and no simulation-digest leak or render-journal fault occurs.
A separate real P2 admission shows its full fire while the distant upper P1
view remains clear. Current-build active-fire cold restore and a whole-game
pause also pass. All images were inspected;25/25 title CTests pass.

Final executable SHA256:
`715005eb2a12bb4e0720b82ce96b7741cfad628d44afb58448b00b660d90fede`.
The quick owner card starts with the actual burst in progress and both heroes
in original cover, globally paused. It bypasses route setup only. The launcher
allows three restored frames before pausing, so its initial view shows the
actual game instead of retaining the black startup surface. At that handoff,
the prior door/reveal, barrier and exact lava-retry checks were still unverified;
later owner acceptance of animation/reveal is recorded above.

## Backward drift during the native intro door

The owner accepts V05's animation and V06's statue/dialogue behavior, but reports
a distinct post-entry death if Zero moves backward when the door admits both.
Original54D24 removes the entrant's collision box while opening/carrying; the
held companion retained its box and still accepted the roof conveyor. The
baseline native door at4464,352 reproduces that drift: after a disclosed16px
backward position setup, X closes at4503.9 and Zero at4401.9, outside the room.
This establishes the causal drift and unsafe arrival, not the reported death
after a complete cutscene. That full scene did not complete in the route-bypass
fixture, so no success flag or forced scene release was added.

Suspend the companion's box when the original removes the entrant's box.
The original restore and existing abort path reinstate native character boxes.
At actual native carry completion, correct only any missing collision clearance
beyond the closing plane/room minimum, preserving feet, animation and spacing.
No invulnerability, extra world tick, cloned door controller or scene/result
flag is introduced; snapshot schema remains15.

On7530c12e, the same native opening/carry restores both boxes and commits one
crossing. Zero stops drifting and retains its native animation, and finishes
at4489 with its left collision edge one pixel beyond the native right4480.
X remains at4503.9. Both bodies remain alive through the bounded closure check;
25/25 title CTests pass. The earlier observer incorrectly required a later
full-scene release after its bounded check, and an earlier observer could miss
phase4 when the boss acquired its scene in the same frame. Those observer
failures remain retained; the final observer follows the actual crossing event.

The actual roof approach is beside the door at4400,363, not the earlier4232
climb shaft. This makes a bounded door check possible without synthesizing roof
geometry. A full scene retest still starts from the genuine preclimb setup,
so the owner executes the authored prerequisites. Test the backward movement
once, then watch through the cutscene and verify both heroes survive and recover
normal control. Do not repeat already accepted animation/statue checks as an
independent matrix.

Candidate SHA256:
`7530c12e79968e9c1bad20121aa4d5676109ede72665becaaba815fed3e8a446`.

## Fire after early X death and active-effect cost

Owner frame1309 shows Zero in cover after X's earlier death, with only the small
flames in both spectator panes. Session1793219669283364 reproduces it on the
original7530c12e runtime:2181 native inputs and state hashes verify. The exact
failure snapshot is obtained through the original replay, with no authored
death, hazard or result fields. The first replay launch needed its bundled BIOS
and the baseline placed in the normal slot; those preparation failures are not
mechanics failures or successful reproductions.

When P1 dies, P2's survivor world executes the original AI directly. The ordinary
two-body AI scope no longer refreshes `dinorex_fire.camera`. Private draws still
replayed those stale fields over the actual native layer, suppressing a later
burst. Use the authoritative canonical layer in that survivor draw, retaining
the existing spatial/signature guards and the ordinary two-body retained path.
This changes only the private rendering transaction: no extra AI, hazard clock,
map update, immunity, ownership state or snapshot fields. A simulation-scope
experiment was rejected after two covered-survivor setup failures; its reports
are retained, and none of its native-controller changes remain in the candidate.

On258e2aa7, cold restore of the exact failed snapshot produces full-sheet orange
coverage0.75842 in each spectator pane, clears through the native retraction,
and leaves Zero alive without retry. Images were inspected. Render journals,
watchdog and simulation-digest checks are clean;25/25 title CTests pass. The
original two-living and upper/distant-view checks remain separately traceable.
Evidence: `owner-survivor-full-replay-verification.json`,
`owner-survivor-exact-replay-report.json`, `survivor-draw-fixed-report.json`.

The separate slowdown report is V15. Existing host sampling shows substantial
driver time during full fire; LLE isolates each transparent primitive. X5 now
selects the existing HLE painter-order batch tier on fresh configurations.
`-DPSX_GL_BATCH_IMPL=LLE` retains the functioning reference. No live selector,
shared-engine source change, resolution reduction or hazard change was added.
Initial pilot timings overlapped a build and are excluded as gain evidence.

The subsequent matched ABBA uses the same native covered-burst snapshot,
settings, host and content, after this task's build completed. PackageA is
258e2aa7 (survivor draw fix, native waits, trusted-overlay fallback, LLE batching).
PackageB isb034e32f (same package with HLE batching). Both referenceA runs take
156.25–260.83 process-CPU milliseconds per world tick and advance5.43–5.90 ticks
per second; both HLE runs take20.42–24.38ms and advance30.97–36.37 ticks/s.
This is a large loaded-host improvement, not an idle-machine60fps guarantee.
Host load remains high and variable. A separate non-fire intro comparison finds
20.15ms/tick reference versus16.87 candidate; treat that one pair as neutral/
inconclusive, with no demonstrated regression, rather than another proven win.

The original ABBA report's `idle` fields were sampled after canonical layer57
cleared, which can precede the retained burst's actual retraction. Its images
still contain fire. Do not present those fields as the non-fire control or a
clearing pass. The separate intro sample above supplies the non-fire control;
`final-fire-clear-and-remaining-report.json` waits for actual controller phase5
and5965 to clear, then verifies both living heroes and inspects the cleared
composed image. No completion flags are written.

Final candidateabafd6b6 includes the identical HLE renderer implementation and
the committed title default. Its own full-fire confirmation measures23.19ms
per tick on the still busy host. The exact survivor image, actual P2 admission
with upper P1 unaffected, current-build cold restores, global holds, strict
per-seat TCP controls and25/25 CTests pass. Images were inspected. Watchdog and
journal behavior are retained; the checked last-run report exits through
`tcp_quit`/`atexit`, not an interpreter fault. Existing freeze captures remain
available outside delivery folders rather than being suppressed.

The current normal-window/audio owner station groups survivor fire and two-live
fire into two short repeatable cards, followed by the three older optional
checks. The exact survivor seed contains a genuine earlier X death, so both
panes correctly spectate Zero. P1 remains keyboard and P2 the actual DualSense
selected at launch. Prior notes and native saves are preserved. The owner still
needs to confirm the new survivor appearance and practical smoothness; do not
claim full parity or stable60fps. Owning local game issuebeads-eio.10.40 is related
to optimization campaignbeads-2wy8.1; raw package-specific samples remain local.

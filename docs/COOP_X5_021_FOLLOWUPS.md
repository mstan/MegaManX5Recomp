# X5 0.2.1 owner-report burndown

Owner reports received2026-10-26. Local branch
`feature/mmx5-x4-parity-local`; work solo. No remote publication or tracker
sync. Preserve the single current delivered build until a replacement passes.

Evidence: `G:/Temp/Codex/psxrecomp/x5-parity/owner-021-reports-20261026`.
Original ZIPs remain in the owner's Downloads. The two archives overlap;
common files are byte-identical. Five sessions include a long opening route
and a later menu/Grizzly route. Metadata matches v0.2.1, execution5ca1047f,
co-op5938c41a. These are current reports, including the visual symptoms.
The opening video link0Lx8hNp71do is unavailable to the current fetcher;
the second video-IOi587Pfeo was subsequently downloaded and reviewed locally.
Inputs/cards/settings are available independently; no baseline.pst was supplied.

Current inventory after autonomous follow-up: **13 bounded passes /
3 unverified**, out of16. Remaining checks are the exact hovering-platform
smoke producer and complete camera/visible-door validation in Sigma's room.
Current source `bbf2b3d`, executable **D20C946D**. The intro card's overall
Pass does not qualify areas the recording did not reach. Passes cover the
specific checks below; final combined-candidate regression remains separate.
This campaign covers user reports; the old234-comparison inventory is parked.

| ID | Owner report or request | Failure mode to investigate | Bounded check / evidence | Status |
|---|---|---|---|---|
| U01 | P2 follows P1 during the teleport animation. | Both the incoming beam and resolving animation reselected the moving leader's X every update. | `candidate-current-bundle-2` catches the remaining incoming-beam drag. `candidate-current-bundle-3/report.json` records a real Select departure/rejoin: P1 walks during the beam, P2 retains its legal landing through completion and both recover controls. Re-anchor only if the floor/support becomes unsafe. | Passed, bounded runtime. |
| U02 | Zero's Z-Buster graphics are corrupted. | Personal shot assembly, source atlas or CLUT is shared incorrectly. | Owner marked the B5B77098 intro case Pass, without a visual caveat; the case explicitly requests Zero's Z-Buster and air-dash in both panes. Acceptance is bounded to this early-stage run/form, not every intermittent effect. | Passed, bounded owner QA. |
| U03 | P2's bottom background follows/offsets to P1. | Secondary-layer camera or retail tile-ring state leaks between views. | Same owner-accepted intro case; input recording reaches X1093 / Zero1445 maximum, providing early-stage separated views. Later roof/arena scrolling is not qualified. | Passed, bounded early-stage owner QA. |
| U04 | Bring a distant teammate to an Alia message's triggering hero. | Native shared hint scenes captured an offscreen partner without gathering. | `candidate-batch-resumed/report.json`: real native hint5 admitted by P2, distant P1 gathered by normal beam, actual text acknowledged and both retained HP/control. Positioning and placement admission bypassed approach only. | Passed, bounded runtime. |
| U05 | The second Alia hint triggered by P2 kills P1. | A late P2 scene left P1 exposed to native fatal camera bounds during the shared world pass. | Original opening replay reproduces death at input6815. Same real hint5 now gathers P1 and safely returns both heroes; original camera update projects the actual scene owner. | Passed, bounded runtime; final combined regression pending. |
| U06 | Falling-debris dust has the wrong palette. | A P2 scenery predicate could select the private character palette; its initial common rows were also incorrect. | `candidate-hint-dust-palette/report.json`: actual P2-triggered native slab admission after a real hint5/gather. Original class9 smoke uses CLUT7806, class4 debris uses7804; composed captures show native gray dust/stone in both panes. Private source/live common rows separately match original DAT1/2. No effect phases seeded; route positioned. | Passed, bounded runtime. Hover-platform/wall producers remain separate checks. |
| U07 | Broken hovering-platform smoke has the wrong palette. | Native shared smoke assets inherit another actor's palette or atlas. | Actual platform destruction/emission and effect lifecycle, combined withU06 where possible. | Unverified. |
| U08 | Pre-boss moving-wall smoke has the wrong palette. | Shared structure smoke uses the private player effect path. | `candidate-remaining-route/report.json`, `actual_structures`: original class3/sub0/1/2 and linked pieces execute F65C8/F677C and emit actual class9 smoke using CLUT7806. `actual-structure-motion-6.png` and subsequent captures show native gray puffs in both panes. No structure/effect phases seeded. Positioning bypasses ascent; this passes smoke color only, not route/camera traversal. The later static-character-image fix does not change this shared scenery painter. | Passed, bounded native producer/color check. |
| U09 | Descending after the roof climb locks the camera and kills either survivor. | Native roof record7C798 retained a one-way Y maximum; reverse traversal reached native fatal camera logic. | `candidate-cp1-batch/report.json`: actual left walk/drop after roof lock reaches P2 Y504 and P1 survivor Y511 alive. Ascent was positioned; no health protection during either descent. Original replay reproduces both fatalities. | Passed, bounded runtime. Actual authored camera bounds reopen only inside this climb. |
| U10 | Boss camera remains between the doorway and arena. | Sigma initializer stopped the entering hero before Both-door clearance, leaving the camera between rooms. | Original exact opening replay reproduces it. Candidate reaches an actual Both crossing and checkpoint3 in `candidate-remaining-route/both-actual-approach.json`. However the teleported route omitted earlier native vertical-camera/atlas commands; its camera retained incompatible Y bounds and its scenery was invalid. Later native CP3 and source-coordinate guide attempts also failed setup. Do not qualify arena camera/control from these runs. | Unverified candidate fix; valid authored route still needed. |
| U11 | Boss door remains open during Sigma combat. | Same premature Sigma admission prevented original door closure. | Actual original closing branch advances Both phase2->4 and checkpoint3 with two living heroes. Native Sigma warning/reveal/48HP follows a placement rescan. This proves intermediate mechanism only: visible door closure during a correctly presented fight remains unverified with U10. | Unverified final visual/control validation. |
| U12 | P2 Fourth Armor shots are corrupted. | Native reflected image FT4 endpoints can precede the real tile by one pixel; taking minimum UV samples the preceding row/column. Page-edge clipping can also shrink a piece to15px. | `owner-orb-qualified-replay` reproduces the owner's actual lingering class15/table0 effect with8501 native input/state-CRC/read-order checks. `bbf2b3d` retains the authored tile, reflects pixels and paints16px geometry; native HUD sampling remains unreflected. `candidate-fixed-plasma` reaches the actual charged effect on D20C946D with clean composed captures, natural expiry and draw verification mismatch0. `plasma-seam-qualification.json`:6 clipped triangles before /0 after. All24 CTests pass, including Y-reflected source-tile coverage. | Passed, bounded runtime and visual check. |
| U13 | P2 cannot use Select to quit from pause. | Native shared Quit flow only consumed P1's input. | `candidate-batch-resumed/report.json`: actual P2 pause/Select, both native confirmation pages and actual frontend return. P1 cannot move P2's prompt. Current-build save/load preserves its owner and native Continue return. | Passed, bounded runtime. |
| U14 | P2 owns a selection limited to X forms, keeping P1 Zero. | Selector needed a campaign-consistent choice for each seat. | Owner decision: one X and one Zero; campaign fixes P1 hero. Original unit selector runs for P1's forms, then P2's forms. `candidate-selector-fixed-0/1/report.json`: both New Game campaigns, input ownership, cancel/back, current-build save/load and real Grizzly entry retain identities. | Passed, bounded runtime. Original art/layout; no custom selector UI. |
| U15 | Black Zero should be separately selectable. | Normal and Black Zero shared a cached native preview. | Same selector runs: separate original Zero/Black palettes, native preview invalidation, selected appearance at actual entry. Does not write persistent progress. | Passed, bounded runtime. |
| U16 | Exploding-train timer was not visible on a cleared stage. | Revisit-specific countdown admission/HUD path may differ from uncleared mission. | `candidate-native-timer-early/report.json`: original cleared mask31 copied as a setup prerequisite, native CP2, P2 actual hint10/contact admission. Native child class10/sub24 and visible5.60/GO countdown appear in both panes;32 observations follow the real timer/explosions. No countdown/GO/result flags seeded. Default hint suppression on; no defeat, full escape-route or visible-hint timing pass inferred. | Passed, bounded runtime for the default configuration. No custom timer UI. |

Use one combined scene/camera episode forU01/U03/U04/U05/U09/U10/U11;
one effects episode forU02/U06/U07/U08/U12; one menu/selection episode
forU13/U14/U15, and the distinct Grizzly timer producer forU16. Add cases
only for a different native mechanic or observed failure. Ordinary setup
failures are deferred with evidence rather than expanded into matrices.

Both original recordings replayed from cold boot with their original cards and
settings on the delivered executable: opening57908 inputs and More34598,
matching state CRC and native input-read order. This qualifies reproduction;
it does not establish a candidate fix or a visual pass. Captures and RAM
observations are retained under `opening-original-replay-full` and
`more-original-replay-full`. Current snapshot schema13 intentionally rejects
older snapshots; native memory cards remain unchanged.

Current candidate passes all24 CTests (`ctest-current.log`). The earlier
original-input replays used turbo presentation and their screenshot requests
timed out; their successful input/CRC qualification is not visual evidence.
Fresh candidate captures show a clean blue native Zero/P2 Z-Buster. Subsequent
owner acceptance closes U02 only for the bounded early-stage run/form.

Local milestone `e081f4e` contains the scene/camera/menu/selector changes.
The next graphics fix initializes each private mutable palette from the
original DAT1/2 type5 common buffer, retaining type9 as its immutable
character color-program source. Previously the private buffer used type9
rows directly and patched only selected common rows. Original common smoke
CLUT4..6 differ from those type9 rows. Native P2 scenery predicates also
retain the canonical stage palette/campaign, matching existing shared-AI
ownership rules. These are source/resource fixes, not a claim that every
reported visual symptom is reproduced or closed.

`candidate-palette-qualified/report.json` verifies both retained source
palettes against original DAT common rows and reads the real host-backed
private palettes through the existing read-only process observer. The same
run captures native P1 Zero dash as a reference and an initialized, visible
P2 Fourth Armor charged projectile, with both heroes alive and no private
draw mismatch. `candidate-common-palette-bundle` also captures P2 Zero's
real shot/dash. Its distant-background setup places Zero in a real pit and
therefore does not qualify traversal or camera safety; the later teleport
rejection is a setup failure, not a new product failure. Earlier direct
debug RAM reads of private9F memory returned zeros and are not valid private
palette evidence.

Door setup is deferred after bounded attempts: loading an intro checkpoint
does not reconstruct earlier scenery/map/atlas commands, and ad hoc roof
positions can leave the authored route blocked or the wrong tiles loaded.
No further checkpoint variants are planned. The actual owner route is
required for U10/U11 candidate acceptance. The original Both-door failure
remains reproduced and the initializer guard remains pending validation.

The remaining checks fit one native intro route: actual U07 smoke, then
U10/U11 with X/P1 + Zero/P2 alive. U08 smoke and U12's lingering effect now
have bounded passes. The original234-comparison campaign and LAN expansion
remain parked.

The final palette/scenery candidate repeats U04/U05 successfully: original
P2 hint5, actual text acknowledgement, distant P1 beam gather and safe control
return, followed by the real slab trigger. The source patch keeps the stage
palette/campaign during P2 camera-area/checkpoint and falling-slab predicates.
`opening-original-graphics-focus` additionally qualifies19022 original reads
and four composed frames after temporarily disabling turbo for capture.
Several frames are native pause/text, so they do not establish a graphics
failure or a candidate visual pass. No further blind capture variants are
credited.

## Owner QA, 2026-10-26

Station: `G:/Temp/Codex/psxrecomp/x5-parity/current-owner-candidate`.
The owner's original `owner-observations.json` and run data are preserved.
Intro card saved Pass at12:09:14-07:00; Fourth card saved Pass with the orb
caveat at12:12:10-07:00. Candidate B5B77098 / source2a7802f, normal audio,
windowed, physical DualSense opened for P2 and keyboard assigned to P1.

The intro diagnostic segment `session-1793035616469027` contains367856
native reads, many paused/idle; maximum world X is1093 for P1 and1445 for
P2. The run therefore does not reach the roof climb or Sigma door around
X4300..4552. U07/U08/U10/U11 remain open despite the card's overall Pass.
There is no new assertion that these candidate fixes failed; their actual
producer/door validation is still missing.

Fourth segment `session-1793041761539586` contains10161 native reads.
`runs/20261026-120917-fourth-followups-b5b77098-7f5251/121141-owner-capture.png`
preserves the lingering-orb symptom. The final runtime report records a
normal SDL window close, not a crash. These recordings and notes do not
establish LAN, later-stage coverage or full X4 comparison parity. No source
change or delivered-archive replacement was made during this QA review.

## Autonomous follow-up, D20C946D

The owner's request to complete the remaining checks autonomously was
attempted without changing success flags or weakening diagnostic replay.
Position-only approach, actual checkpoint retry, native roof checkpoint and
a small-step guide derived from the original recording were tried. Setup
failures are retained under `candidate-remaining-route`,
`candidate-roof-checkpoint-native` and `candidate-native-path-guide`.
They are not new confirmed product failures or final route passes.

The door's initial wait with P1 distant is correct Both-player behavior.
After P1 approaches, both heroes walk through; the original closing branch
records checkpoint3. The later Sigma sequence is real, but the earlier
camera/atlas setup was incomplete. A post-load movement assertion also
failed in that invalid camera context; it does not establish a control fix
pass or a new reproducible control regression. Do not add more checkpoint
or geometry variants. Bundle U07/U10/U11 into one owner-assisted native
route, with individual results for smoke, camera and door.

U12 is fixed locally in `bbf2b3d`; candidate SHA256
`d20c946d98e364a04bd000a9c99bfeeb651f51c3fca260a5ff7bab030bde0433`.
`build-static-plasma.log`, `ctest-static-plasma.log` and
`plasma-seam-qualification.json` retain build/test and actual before/after
packet evidence. The owner subsequently authorized local v0.2.2 packaging
with the remaining three checks explicitly unverified. Release authorization
does not change their results or establish full parity.

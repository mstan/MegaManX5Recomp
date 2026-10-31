# X5 v0.2.2 opening-stage owner reports

Local branch `feature/mmx5-x4-parity-local`. Work solo; no publishing,
pushes, PRs or tracker synchronization. Keep the verified v0.2.2 delivery
until a replacement is qualified. Owning local issue: beads-eio.10.33.

Evidence: `G:/Temp/Codex/psxrecomp/x5-parity/owner-022-opening-20261026`.
Owner archive `E:/Downloads/opening stage bugs (1).zip`, SHA256
`e4250432f2ae3a57ffd43ec3295e7d34a79d7325c906d5f7d9b8ce23efe565c2`.
Video: https://youtu.be/FBJzM5rLSCE.

Three sessions identify v0.2.2, co-op b27da12d, execution5ca1047f.
The full session1793059179650600 contains30467 native input reads and
67352 co-op reads. It starts from cold boot with original memory cards;
Both doors and visible Alia are selected. Preserve input CRC/read-order
qualification when replaying the original executable. No baseline snapshot
or direct developer setup was supplied. Earlier bounded U02/U03/U04 passes
do not resolve these new reports.

Current bounded inventory: **11 bounded passes / 0 failed / 0 unverified / 11 reports**.
The final four checks are qualified below. This is not full campaign parity.
The older234-comparison campaign and LAN remain parked.

Latest integrated mechanic candidate: `7122CA29` (full SHA256
`7122ca29ca207ac82ce8bff0d1aa7b5363612a1f64a4cb267134628006b604cd`). Release metadata and delivery are recorded separately.
Latest evidence: `G:/Temp/Codex/psxrecomp/x5-parity/owner-022-final-four-20261026`.
Earlier owner failures and intermediate evidence remain recorded below.
The owner failed three of the previous four QA cards; those findings remain
recorded below. The prior buster card was explicitly a pass.

| ID | Report / prior comparison | Concrete hypothesis | Bounded check | Status |
|---|---|---|---|---|
| V01 | Artifacts when P2 uses buster; U02/U12. | Shot, muzzle or impact may use a different owner/source path from the prior charged-orb check. | Native samples retained; owner card `buster-x-92866ead` marked Pass, "looks good" on2026-10-26. | Passed, bounded owner buster check. |
| V02 | P1 teleport graphics corrupt when P2 triggers Alia; U04/U05. | The retained P1 passenger painter omitted native X armor while Zero owned the world. | Restored armor only for the P2 world pass. `integrated-graphics-lab/alia-final` acknowledges six original pages, completes the native return and captures every materialization phase. Its beam, split sphere, cracked sphere, silhouette and forming armor match the co-op-disabled original renderer in `native-forming-reference`; the cracked sphere is authored art. | Passed, bounded native animation/return comparison. |
| V03 | P1 background changes when P2 reaches debris building; U03/U06. | The one shared EEBE0 atlas command was copied into both cameras. | `candidate-scene-bundle/report.json` loads the complete native-route pre-building snapshot, then uses real inputs through the area. Stationary P1 remains X1091/mode6 with its original city scenery; P2 reaches mode2/building. Source uses the authored camera threshold2568 and existing primary/secondary update boundaries. | Passed, bounded native transition and composed comparison. |
| V04 | Roof door introduction, companion pose/statue, and the newly forwarded boss-entry freeze; U10/U11. | Generic free traversal skipped native intro subtype1 camera carry; a retained companion also needs its native stand/animation. | `verified-roof` runs original54D9C once per world, mirrors only its actual displacement and preserves its native camera carry. Statue/reveal, four Sigma pages, held controls and both actual jumps pass. `verified-mattrex-boss` and `verified-mattrex-zero-p2/qualification.json` complete actual Both passage, intro, HP fill and release. In the second run Zero/P2 owns the scene and its native dialogue frames advance; the failed parent assertion wrongly expected P1 ownership and is retained as a harness error. | Passed, bounded intro/Mattrex scenes; no universal boss claim. |
| V05 | P2 lower background still offsets to P1; U03. | Class36 is screen-relative scenery. Native F123C precomputes X from the one world camera, so both panes reused that position. | `original-foreground-probe` versus `candidate-foreground-probe`: same original setup, position-only separated P2, both alive, native scenery. Each private draw now derives placement54 minus trunc(5*its camera/4) before native culling. The enclosing render transaction restores the shared actor; its controller/animation still runs once. Composed panes now scroll the lower scenery independently. | Passed, bounded source/native/composed comparison. |
| V06 | X graphics overlap the HUD with Zero as P1; owner clarification. | Private HUD conversion omitted P2 frame/badge; native sorting must keep all hero art behind the HUD. | `integrated-graphics-lab/hud-final`: Zero/P1 and X/P2, original raw6/0 loader, position-only overlaps, real charge/dash inputs. Native HUD sprites with view-local tile projection restore the frame, X badge and lives. Body, armor, charge glow, trail and both heroes remain behind intact native meters. | Passed, bounded composed/native packet check. |
| V07 | X/P2 hover energy at his feet is missing with Zero/P1. | Shared FX21 tests the wrong body's hover frames and immediately retires. | Native50F58 now updates under its allocating owner. `verified-hover` performs the actual jump/air hover and retains the live exhaust. Its blue oval/boot art matches `hover-primary-reference`; the rejected body-atlas experiment was reverted. | Passed, bounded native/composed check; owner retest pending. |
| V08 | Charged Grizzly/Crescent weapon graphics corrupt. | Class0A shield halves use compressed graphics directory34 at native tile0540, absent from the static NIM image mirror. | `verified-crescent-p1` and `verified-crescent-p2`: native Triangle charge/release allocates two actual shield halves, spends energy32?4 and expires naturally. Composed gold crescents are intact. Both seats sample the original decoded sheet with their own live palette; cache keys retain the sheet identity. Weapon1 access is a disclosed setup cheat, not an earned reward. | Passed, bounded charge/image/expiry for both X seats. |
| V09 | P2 cannot touch/rescue an injured stage Reploid. | Native category4/class4 DAT85 NPC hardcodes PLAYER in contact and healing; ordinary pickup admission does not cover it. | `native-rescue-producers.json` identifies four relocated original programs. Original raw6/0 NPC1776,248 is touched with both heroes alive: `rescue-candidate/both-alive-rescue` (P2 X) and `final-mechanics-lab/rescue-zero-qualified` (P2 Zero) grant exactly one team life, heal only the rescuer and retire naturally. Setup changes position/resources/contact protection, never rescue or reward flags. | Passed, actual native contact/reward/healing for both P2 heroes. |
| V10 | P2 is unaffected by Sigma viruses. | P2 body infection worked, but private-context restoration discarded native full-infection/progression counters at PLAY BE/A9. | `final-mechanics-lab/virus-qualified` and `virus-zero-qualified`: original raw6/1 class19 placements, four native contacts using one continuation to respawn them. No infection/actor-phase writes or protection. P2 X reaches31, loses HP30 to18, recovers to0, P1 HP stays26 and BE advances0 to1. P2 Zero completes its native invincibility/healing cycle, BE and Zero progression advance, and both remain alive. | Passed, actual native contacts/full infection/recovery/progression. |
| V11 | Suppressed Alia hints briefly interrupt controls. | Text-only suppression still starts native stand, freeze and co-op gather. | The guarded3D29C filter runs only after original56D54 commits the admitted hint's heard bit. It retires that silent actor and cancels only its provisional gather. `verified-silent` reaches actual hint5: both players keep movement, no shared lock/gather is acquired, and the native player/campaign knowledge bits advance. | Passed, bounded offline movement/completion check; downstream cannon, solo and LAN not requalified by this case. |

The owner marked three of four cards Fail; the buster card is explicitly Pass.
Saved notes and current-build baseline/input recordings are preserved for the
next investigation under `owner-022-followup-20261026`. Do not reuse the former
3/6 headline as proof that the complete roof scene passed.

`original-full/report.json` qualifies all30466 retained native reads, including
state CRC and co-op read order, on exact releaseF7408EFC. Its composed captures
show V03: P1 remains at X1091 while P2 enters the building; P1 layer1 changes
from mode6 to2 and the stationary pane switches scenery. Original EEBE0
reacts once to shared PLAY1D=1, then advances its controller. The existing
co-op EEC0C publication incorrectly copies this command to both cameras.

Native F11BC/F123C computes class36 screen-relative scenery X from its authored
placement at actor54 minus trunc(5*canonical_camera/4). The shared actor is
drawn in both panes without a private-origin correction. Inspect this producer
for V05 rather than assuming the ordinary secondary-camera mapping is wrong.

Fresh native P1 Zero and P2 Zero shot samples are retained in
`original-buster-hero1` and `original-buster-hero0`. They show a blue native
class3A projectile, so the earlier gray original-route image is history/path
dependent; those fresh samples do not close V01. These sessions enable render
verification; the ordinary owner/replay path does not. The one verification-off
comparison also shows a clean native projectile. That hypothesis is rejected;
do not add verification variants.

Historical2c39a4d/92866EAD passed24/24 CTests. Its initial hook-contract failure
was corrected by using the already instrumented primary-camera boundaries;
those scenery changes needed no new hook site. The later hover fix adds50F58 as recorded below. Native story checks
must acknowledge every ready page at91C44=FF/03. Earlier harness timeouts used
the menu readiness byte or acknowledged only one page; those are not control
regression findings. The final acknowledged run and real jump check succeed.

The earlier scene/foreground input probe is on765AC663. The92866EAD rebuild
changes a source comment/diagnostic identity and passes24/24 CTests. Keep the existing verified release archive
until a new release is requested and qualified. No new build delivery is
claimed by these checks.

The video fetch provided only its first roughly50seconds before HTTP403.
Those opening frames are decoded locally; later visual reports must not be
claimed reviewed from that partial file. The complete diagnostic route is
independently CRC/read-order qualified above. Stop retrying video formats.

Use one original route for scene/background/door evidence and its actual
shot intervals for graphics. Add a case only for a distinct observed producer
or ownership path. A loaded location or screenshot alone does not pass a
mechanic. The older234-comparison campaign and LAN remain parked.

The four previously open reports V02/V06/V09/V10 are now bounded passes.
The final-four evidence directory retains scripts, native frames, RAM samples,
setup snapshots and qualification reports. Initial failed setup attempts and
intermediate HUD captures remain historical evidence, not successful checks.
Raw stage IDs are used in the new checks: earlier reports guessed stage names
incorrectly; the original object programs/placements identify the interaction.

The Reploid dispatcher and contact leaf are matched against original code,
including DAT85 and the native life-grant instruction. The NPC advances once;
only its original contact leaf receives a second admission attempt. Its native
healing follows the successful collector through the existing serialized pickup
ownership map. No synthetic rescue, life increment or healing outcome is seeded.
Native full-virus events preserve the two progression counters alongside existing
shared damage/rank counts. Other scene/timer fields remain world-owned.

Private-view HUDs now keep original packet pools, sorting and anchors. Only the
sampled UI tiles/palette are projected inside the existing VRAM transaction.
The first expanded check found a missing P2 frame/badge; the final captures show
both badges, frames, lives, meters and virus block intact under hero overlap.
Shared-screen HUD conversion remains unchanged. Initial intro captures during a
shared scene did not qualify independent HUDs and were superseded by raw6/0.

The latest candidate passes24/24 CTests and `git diff --check`. Native50F58
was added to the configured/recompiled hook list; hook-contract validation
passes. The new archive `E:/Downloads/mmx5 metadata 1.zip` has SHA256
`7ad451d1e4e5ad15cfb23ec23093ff6ebdcd1fd2d3403e3fb0aeafac784a3dd7`.
Its179307-read route was timeboxed after a qualified original prefix through
sample18966; it is not a complete replay. Targeted checks use native loaders,
original owner snapshots and disclosed position/weapon-access setup instead.
No defeat, story-success, infection or scene-result flags were set to pass a check.

Release delivery remains the verified v0.2.2 archive until the requested
v0.2.3 package is independently verified. Retain the original
recordings, reports, images and setup snapshots. Repeated isolated test copies
of the EXE/assets/BIOS/mod catalog are disposable after their evidence is saved;
the shared build and current owner station remain available outside delivery.

Cleanup receipt: `owner-022-followup-20261026/cleanup-payloads.json` records
removal of108 duplicate EXE/asset/BIOS targets (1233.7MiB). No owner cards,
settings, recordings or diagnostic evidence were removed. Shared build36ED7770
and the existing owner station are retained; intermediate test payloads can be
recreated from the retained setup/harness rather than accumulating more builds.

Release candidate v0.2.3 executable SHA256:
`d30199794d371917712cbe0186a34a08115526f48dbde9f1fb2bf8e083ee358c`.
It changes version metadata after integrated mechanic candidate7122CA29;
code/feature contracts are unchanged. Fresh intro, Grizzly and duel QA states
are regenerated and cold-loaded on the exact release candidate before delivery.
Release qualification/manifests are retained under
`G:/Temp/Codex/psxrecomp/x5-parity/release-023-20261026`.

Release packaging also exposed a first-run launcher error: Test-Path rejected
an empty remembered disc path before the picker could open. Both portable
launchers now guard the empty value and the read-only VerifyOnly path. The
validated launcher sources are retained in packaging/release for later builds.

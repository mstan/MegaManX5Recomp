# X5 v0.2.6 owner follow-ups

Local branch: `feature/mmx5-master-native-waits-local`.
Owning local issue: `beads-eio.10.42`. Historical parity work remains parked.

The native character selector is accepted: "The character select is perfect
now." P2 can use Triangle to return when P1 made a mistake. Preserve that
behavior without another selector redesign or routine retest.

The new report supersedes the prior qualified acceptance for these specific
paths. The earlier bounded passes remain evidence for their actual setups;
they do not close the five failures below.

| ID | Report / link to earlier comparison | Bounded check | Status |
|---|---|---|---|
| R01 | Intro: distant P1's foreground disappears when leading P2 reaches the falling-debris building. Extends V02. | Advance the actual shared checkpoint with P1 still on the road; retain its six original foreground objects and inspect the composed screen, then let both cameras leave. | Passed bounded runtime check: six retained, then native retirement to zero. `native-boundary-final-report.json`. |
| R02 | Intro Both-player boss door: P2 still freezes instead of animating. Reopens V05 for the recorded entry path. | Actual native admission/open/carry/closure, with a disclosed 16-pixel backward position perturbation. Check displayed sprite frames, both collision suspensions, one crossing and closing clearance. | Passed: 16 displayed P2 frames, one native initialization/crossing, both alive and clear of the closing plane. `intro-door-drift-final-candidate-clearance-report.json`. Full reveal/control recovery was not repeated in this bounded check. |
| R03 | Dinorex: later background corruption with P2 leading. Extends V07 beyond the first atlas boundary. | Fresh checkpoint 0; P1 remains on the upper starting ledge while P2 triggers the actual lower-region command. Compare the original uploaded art separately from each camera. | First admission passed: exact original replacement-art SHA256 and retained upper P1 atlas. Later lower-route captures through wall3 also look correct. Continuous fresh-entry route remains unverified; manual validation pending. |
| R04 | Dinorex: the fire flood now appears, but its graphics are corrupt. Extends V13. | Lower-route original fire movement/retraction, composed captures and native atlas admission. Presence alone is not a pass. | All three recorded lower-route bursts/retractions passed, with composed fire art visually inspected. Continuous fresh-entry route remains unverified; manual validation pending. |
| R05 | Dinorex: P2 camera misses fire-breakable stone-wall stops; the third wall cannot be destroyed. Reopens V09. | Actual terrain controllers, camera stop, original fire destruction and camera release for all three walls. Position cheats may bypass shelter traversal; no destruction/progression flags. | Passed: stops2240/2560/2880, actual original fire destruction and restored bounds for all three walls. Both heroes alive. `dinorex-three-native-walls-verified-report.json`. Shelter traversal/survival bypassed for walls2/3. |

Original archive: `E:/Downloads/Opening stage and Dinorex stage bugs 0.2.6.zip`.
SHA256: `7355dd6611126ee1fe8eaee3f9ea0c0801920ab5cd99b8e60855b549ea74e648`.
Evidence root: `G:/Temp/Codex/psxrecomp/x5-parity/owner-026-reports-20261028`.
Session `1793231610013912` declares v0.2.6, execution `6702ced5`, co-op build
`264718ed`, local split, Both doors and side-by-side HUD. The archive supplies
starting cards, settings, mod choices and the input trace; it has no restored
save-state baseline or screenshots. Preserve all originals unchanged.

The supplied video is <https://youtu.be/lMAEEbrG_W0>. The web tool could not
fetch it, so the video has not been reviewed. The owner plans another intro
check with Alia messages; that future check is not yet evidence of a new pass
or failure. Work remains local and solo; the delivered v0.2.6 archive is intact.

## Causes and implementation

- R01: original intro `F11BC` retires foreground class36 when the singleton
  checkpoint advances. Its exact `F1200` retirement predicate now waits while
  a living/dying hero still views the road. Original objects, animation and
  culling remain in use; the shared checkpoint is never rolled back.
- R02: writing the companion's negative door gate skipped original `399D4`
  initialization. The companion now runs it once. A transient brake/landing
  animation can also end during the shared hold; its terminal program now
  changes to the original authored entry loop26. Only the suspended P2
  animation advances; the native door still owns movement and scene timing.
- R03/R04: the lower-region command is small-area class1/sub0, rather than a
  class0 camera command. It still tested only P1's body/camera. An eligible P2
  now executes that original map/atlas command once through the existing P2
  camera scope. Existing immutable old-region art preserves trailing P1.
- R05: the actual walls are terrain class0C/sub1 (`F3780`), not enemy24/sub1.
  Their original controller now uses the admitting fire owner's camera. It
  captures bounds, locks, waits for actual fire destruction and restores
  bounds itself; no wall or fire outcome is fabricated.

## Evidence limits

The supplied original executable replay verified all 31,583 native inputs and
hashes. Its observations established the shared-checkpoint foreground
retirement, timer-only false positive in the old door check, missing first P2
lower-atlas admission, and the walls' actual terrain type.

The current runtime candidate SHA256 is
`e5324d2ee255b076a4edcdaf21e3852526be953b70339aa616f60940fa0efea4`.
R01 and first-atlas evidence used `3d2938d0`; three-wall/fire evidence used
`ba0d8b3c`. Subsequent changes are confined to Both-door animation/entry
(immunity is preserved around the committed native initialization) and the
two overlay code-generation declarations. The final candidate passes the
door check and all 25 CTests. Original
archive, reports, composed images, native snapshots and capacity receipts are
retained at the evidence root. Original report failures and setup failures are
kept distinct from qualified passes.

No LAN compatibility or performance improvement is claimed. Native saves and
owner settings were copied into isolated test profiles; the delivered ZIP and
accepted character selector remain unchanged.

The combined continuous fresh-entry setup was deferred after bounded attempts:
a large vertical teleport crosses the native fatal camera bottom before its
authored camera admission, and stepped placement at x1200 stops on a floor
with camera2192. Neither bypass qualifies the missing continuous route. This
is a setup gap, not a claimed gameplay pass or an established new defect. The
prepared human check starts from the genuine first-P2-atlas state and follows
the actual route to the fire corridor.

The existing 25-check CTest suite initially caught missing code-generation
declarations for the two new overlay hooks. Both are now declared in
`game.toml`; no audit was weakened or compatibility validation bypassed.

Three bundled local owner setups are prepared at
`G:/Temp/Codex/psxrecomp/x5-parity/owner-026-reports-20261028/manual-qa`.
Run `Validation.ps1`: the continuous P2-led Dinorex route is first, with optional
intro door/reveal and foreground visual confirmations. P1 uses keyboard and
P2 targets DualSense. Each launch has normal audio/windowing, starts globally
paused, and reserves local capacity until its game closes. New notes are saved
separately from earlier accepted observations. Physical input ownership on this
new candidate still requires the owner; no device-file-only pass is claimed.

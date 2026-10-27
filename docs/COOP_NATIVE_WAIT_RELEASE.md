# Local v0.2.5 qualification

The owner accepted native startup, independent keyboard/DualSense controls,
and the complete initial split-screen HUD handoff. The only gameplay change
after the accepted v0.2.4 tree is the arrival HUD correction: both cameras are
available at enrollment, and private presentation continues until P1's native
action0 releases the stage-entry hold. P2 can finish resolving earlier. No
new persistent state or scene/progression flags implement this correction.

The local integration branch is `feature/mmx5-master-native-waits-local`.
It includes master c8fde08b, PR14 at1267f57e and accepted co-op at3ed23ec.
The original co-op branch and its full history remain intact. Shared SDK
cada1545d combines ffbd9d1c with the required local co-op changes; launcher
4940b09b retains separate-controller support on the current launcher base.
Nothing is pushed, merged remotely or published as a remote release.

The exact runtime and source identities, raw performance samples, original-disc
AOT receipt and prepared-state results are recorded in `QUALIFICATION.json`.
This package qualifies the combined SDK/UI/title integration; it does not
isolate a wait-only speedup or certify other games on this local SDK merge.

## Correctness evidence

- All25 title CTests pass, including44 actual generated-caller wait comparisons
  with12 accelerated cases, checking callbacks, continuations and observable
  state against the original polling implementation.
- All11 selected SDK tests and3 controller/launcher tests pass.
- Original-disc AOT:53 original images,54 recipes,72 published pairs and16,289
  guarded rows. The archive includes only the audited current cache.
- Fresh intro, Grizzly and pre-duel snapshots were created on the final binary.
  All three cold-load with intact positions/lifecycle and recover native
  gameplay through the original pause menu. A short failed menu-resume pulse
  was replaced by a released-input/longer-edge check; no production timing or
  watchdog was relaxed.
- Independent guest inputs and both attacks pass on the current integration.
  Owner physical input isolation and final HUD acceptance are separate evidence.
- Snapshot filenames now include the SDK's active-mod namespace; prepared
  launchers preserve that actual filename and require the exact binary hash.

The owner entered the original New Game flow. The automated New Game setup
stalled at its opening-sequence setup on both the old and new builds; native
owner controls succeeded. Failed direct-loader experiments were discarded.
Stage/checkpoint setup for Grizzly and the duel uses the original loaders and
bypasses prior routes/progression. It does not qualify bosses, rewards or exits.

## Stability and remaining scope

One earlier unattended title run hit the unchanged4-second starvation watchdog
before co-op enrollment. The retained ring shows a4.751s host-time gap across
only49,675 guest cycles, followed by117 SIO events in299 microseconds. That dump
does not establish a stuck SIO transaction; the source of the host stall remains
unverified. The watchdog stays enabled and unchanged. This remains a diagnostic
caveat, separate from the owner's deliberately closed playtest windows.

The older comparison campaign and physical LAN tests remain deferred. Solo
Grizzly pause reproduction, native cold-card persistence of the Black Zero
capsule unlock, full winner-selected stories/endings and native cold-save duel
winner persistence are not newly qualified. No full-parity or older-build
netplay compatibility claim is made.

Local split-screen, Either-player doors, diagnostics and Alia suppression stay
enabled by default. The package opens windowed with normal audio, P1 keyboard
and P2 DualSense. CD Speed and Fast Loading packages are absent. The delivery
folder retains one current X5 archive; native saves/settings are preserved.

## Matched runtime comparison

The A1/B1/B2/A2 check used one game at a time, the same two hero forms,
positions/camera,1080p nearest OpenGL, offline local split-screen, default
diagnostics/hints and dummy host audio. Both had dynamic resolution and extra
render-pass verification disabled. Each window warmed for5s and measured15s.

| Package | CPU ms/world tick (two samples) | Mean world ticks/s |
|---|---|---:|
| Delivered v0.2.4 | 15.087, 14.427 | 59.88 |
| Combined v0.2.5 | 14.253, 12.688 | 59.88 |

Mean CPU cost changed by-8.7%; frame throughput changed by-0.00%. The CPU ranges are favorable and disjoint in this short workload. This is bounded X5 evidence for the combined package, not a general wait-only or
whole-campaign speed claim. Raw samples retain process CPU times, RSS, player
state, renderer/performance counters, settings and source/binary identities.

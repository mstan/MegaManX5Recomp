# X5 Both-player door animation experiment

Owner-confirmed scope (2026-10-27): P2 retains the appropriate native walk/run
or dash animation through automatic Both-player door entry. Preserve current
admission, movement and timing. Local experiment only; owning issue
`beads-eio.10.34`. The delivered v0.2.3 ZIP is separate from this candidate.

Evidence: `G:/Temp/Codex/psxrecomp/x5-parity/door-animation-experiment-20261027`.
Baseline is exact release D3019979. Candidate executable SHA256:
`ae500255799a9502be67d27fea3dccb4e5f94affb5ad7c8c7b81560453ba51f4`.

The intro subtype1 door retains original resident54D9C because it also owns
the authored layer1 carry, camera arrival and closing event. The companion
receives its actual position/history displacement. The shared PLAY10/1C lock
suspends second_tick, however, so P2's last animation frame never advanced.
The ordinary P1 player pass continues its door animation.

The correction invokes only original17248 for the living, on-foot P2 companion
when the native carry runs and the shared lock has suspended its body pass.
That leaf updates the native animation cursor/timer (34/44..47); it does not
advance physics, contacts, HP, shared clocks, scripts or the door controller.
The selected native walk/dash animation remains intact. No new serialized
state, hook site, settings control or replacement animation is introduced.
Ordinary free-traversal doors keep their existing body updates.

| Check | Before | Candidate | Status |
|---|---|---|---|
| Walk into actual intro Both door | P2 remains on sprite9 through the observed104-update carry; P1 cycles. | P2 cycles native sprites7..20; both follow the original carry and reach the native closing/boss admission. | Passed, bounded native input/RAM/composed check. |
| Dash into that door | P2 remains on sprite47 through the observed107-update carry. | P2 advances native dash sprites46..48, retaining the dash pose until release. | Passed, bounded native input/RAM/composed check. |
| Native intro scene and control recovery | Previously qualified by the11-report follow-up. | Intact statue, four original Sigma pages, actual unlocked jumps by both heroes, both alive. | Passed, actual scene/control check. |
| Ordinary final Both door, raw6/1 | Existing free-traversal path. | Native checkpoint, actual admission/opening/passage/closing and boss intro/HP fill/control release. | Passed, actual entry/intro/fullHP/control recovery. |
| Automated regression | Release24/24. | Candidate24/24 CTests and diff whitespace check. | Passed. |

The original paused roof approach supplies setup only. Position/history-only
setup puts P2 just behind P1 to reach the aperture while actually moving;
Right and optional Circle inputs select the native walk/dash. No door phase,
animation, timer, camera-result or story-success values are seeded. Native
animation diagnostics record212 missing ticks across the two106-update carries.
The first observer missed fleeting phase4 and timed out in the legitimate boss
scene; it is retained as a harness error. The corrected observer reads the
actual door closing state or native scene admission instead.

These checks cover the reported intro carry and one ordinary-door regression;
they do not certify every door, mounted admission, survivor transfer or LAN.

Optional owner setup: `door-animation-experiment-20261027/candidate/Try Door Animation.cmd`.
It restores a fresh exact-candidate paused snapshot before the door, selects
P1 keyboard/P2 DualSense and uses a normal window/audio with disposable saves.
No owner-facing game is launched automatically. Retain the candidate/QA state
for this experiment; redundant baseline executable/assets are disposable.

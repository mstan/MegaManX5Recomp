# X5 follow-ups after the current Grizzly owner QA

Owner reports, 2026-10-25. Keep local. These follow the completed Grizzly
manual run and do not change its nine-check denominator. Local fixes:
f651130 (graphics) and c32de77 (nonblocking health pickups).

| ID | Report and desired behavior | Bounded check | Status / traceability |
|---|---|---|---|
| Q01 | Native health refill calls545DC to freeze all world categories. In co-op ordinary pickup healing must let the partner and world continue. | Original allocated item, exact delayed HP/auxiliary energy and ownership; partner jumps while healing is active; collector controls afterward. Both seat projections and immediate X ammo share one separated setup. | Passed bounded automation on82A36EAA. Red on FDFD7148 reproduced the freeze; pickup-qualified passes P2 large HP, P1 small HP and X ammo with1250 clean private-render checks. Local issue beads-eio.10.22. Native item placement and hurt resources are setup cheats; stage acquisition, incoming damage and tanks are not qualified. |
| Q02 | Zero/P2's Triangle projectile uses a private character assembly with no compressed graphics pointer; drawing sampled the other hero's shared static texture. | Original Triangle inputs, actual shot3A, retained native texture/palette and composed art in both panes. | Passed automated visual check and owner82A36EAA Grizzly/Gaea station visual retest. Clean blue Z-Buster captured on FDFD7148. Local issue beads-eio.10.23 retains the170B32A6 owner failure. |
| Q03 / R9 | The malformed airborne trail is native shared type1 dash exhaust; its private owner was retained for updates but missed during static drawing. This remains the existing R9 comparison. | Actual jump then air dash, native action0D and emitted effect packets; inspect exhaust and afterimages in both panes. | Passed automated visual check and owner82A36EAA Grizzly station visual retest. Clean green exhaust/red afterimages on FDFD7148. Existing local issue beads-eio.10.11 and historical failed status remain open until owner acceptance. |
| A05 follow-up | Gaea X/P2 shot2D uses static tile0520; its muzzle/shot atlas also needs the native compressed group uploads. This remains the armor list's existing A05 check. | Original native shots, correct static origin and original50D04/17348 uploads; inspect actual muzzle/shot art in both panes. | Passed automated visual check and owner82A36EAA Grizzly/Gaea station visual retest. Clean gold muzzle/green shot on FDFD7148 after restoring native uploads at320,32 and320,64. Local issue beads-eio.10.21 retains the170B32A6 screenshots F2178/F2420. No extra comparison added. |

Current owner recording, including complete restored-state baseline and input
reads: G:/Temp/Codex/psxrecomp/x5-parity/owner-grizzly-ps5-20261025-162432/saves/diagnostics/session-1792970678013800.
Q02/Q03/A05 share one graphics diagnosis and capture session, retaining
individual acceptance. Evidence root:
G:/Temp/Codex/psxrecomp/x5-parity/owner-effects-20261025.
fixed-final/probe-report.json records19 samples and676 clean sandbox checks;
the three *-active.png images show the actual effects. graphics-qualification.json
binds the native resources and captures. Earlier partial Gaea correction made
the shot invisible and is retained as incomplete evidence, not a pass.

The combined82A36EAA candidate repeats all three graphics checks in
combined/probe-report.json:19 samples,674 clean sandbox checks, and actual
composed-image review. All24 CTests and six matched local network checkpoints
pass (native roles, movement/attacks, independent menus and hint suppression).
Fresh-process Grizzly/Gaea/duel fixture restores pass with unchanged native
card bytes; the duel fixture also completes its actual introduction with both
heroes alive at32HP. These are bounded regressions, not physical LAN or duel
winner/story/save acceptance.

Current follow-up inventory: **4 passed / 0 open / 4**, bounded as recorded.
Only Q01/Q02 are new checks. Q03/R9 and A05 retain their original denominators.
Neither loading a fixture nor compilation alone establishes a graphics pass.

The owner withdrew the separate closing-walls report. It is not an open check.
Current Grizzly and armor acceptance remain in COOP_X5_GRIZZLY_BURNDOWN.md and
COOP_X5_ARMOR_SELECT.md; no new setup matrix is requested.

The owner reported the Large health aid only set Zero to half HP; do not treat that helper use as a pickup pass. Q01 retains its native allocated-item automation. Physical input crossover is tracked separately; no independent-device or LAN pass is inferred.

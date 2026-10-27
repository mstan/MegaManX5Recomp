# Mega Man X5 serial Windows review

Tracking: beads-znfx. Current scope and review checklist: [SERIAL_REVIEW.md](SERIAL_REVIEW.md).
Loading has owner approval. The owner authorized merging the replacement
renderer once the foreground barriers were verified. That focused check passed;
this supersedes the earlier interpolation and fixed-view candidates.

Default on: OpenGL/1080p with nearest textures, modern adaptive background,
ordinary enemy placement, room-edge framing and HUD, plus resident DAT/BIN
asset loading. The old fixed16:9 tile-ring widening path is retired, uncompiled
and unavailable. Interpolation is archived, uncompiled
and unavailable. Generic CD speed and host pacing packages are excluded.

The two original-disc archives contain231 members:165 typed assets,seven raw
DAT resources and59 raw BIN overlay/data members. Guarded loading-screen calls
retain the original receive callbacks, GPU/sound queues and completion protocol.
Background actors, movie/audio streaming and initial header reads stay original.
The fixed REFERENCE implementation leaves original loading unhooked.

Original-disc native recipes retained:53 images plus BIOS recipe,90 audited
native pairs and15,657 manifest rows. New overlay namespace:
cg18_d1867bb4_gce8c0cc39_f0. All native pairs and original-byte guards passed;
the six pre-existing explicit exclusions remain. Complete native coverage,
Linux release and full-game acceptance are not claimed.

A final 45-second hidden OpenGL check of compiled source 467de64 progressed
through the intro/attract sequence, served 29 resident requests/3,558 sectors
from both archives and shut down cleanly. It verified and reused the local
49,319,936-byte asset pack. Packed start preserves its original v0=0x800A0000
return value (raw start returns 1). The screenshot confirms a 2130x1200 internal
surface: the requested 1080p setting rounds up to 5x the 240-line reference.
The modern renderer emitted 2,159,459 tiles and encountered 434 ordinary enemy
eligibility checks during supplemental scans; no invalid-layer setup occurred.
Focused shared tests checked map coordinates beyond the native512px ring,
retail palette encoding, transparency/OT groups, double-buffer preservation,
capacity rejection and4:3 identity. That check exercised attract state9 only.
The first owner review exposed normal player gameplay state10 being omitted
from the renderer and extra-placement gates, leaving those stages at4:3.
Source ee4ac11 enables both retail states and preserves native menus/movies.
This is startup/renderer evidence; gameplay, music and renderer
acceptance remain pending. Exact executable-bound evidence lives in the central
parity-review-20261006/receipts/MMX5.serial-validation.json.

The corrected source ee4ac11 passed a targeted normal-gameplay check: real
player stage state10,106 revealed native columns,204,568 modern background
tiles and one supplemental ordinary enemy eligibility check; no invalid-layer
setup. It served17 resident requests/1,736 sectors and shut down cleanly.
The stage capture is2130x1200 and visibly shows the expanded scene. The earlier
attract evidence is historical. Audio/controllers were disabled for this check;
owner second-pass renderer approval remains pending.

Quick owner checks: resize, intro enemies at both revealed edges, scrolling
layers/HUD/dialogue, room ends and normal boss reach, then music/transition.
Two owner passes maximum.

The reported foreground barriers are retail category5/type36 objects, not
background tiles or HUD. Their overlay at800F11BC uses screen-relative X at
trunc(5*camera/4) and native96px draw bounds. The modern adapter now includes
these decorative placements in its supplemental scan, expands their original
draw bounds, tags their packets as scenery and preserves the5/4 projection
when the view anchors at room edges. Guest positions and placement records
remain original; no repeated or imported assets are fabricated. The skyline
also uses the existing shared half-speed parallax helper.

An isolated copy of the owner's checkpoint checked16:9 and32:9, cameras320,
384 and the asymmetric room start. A64px camera step moved the rendered
barrier texture80px: the compared opaque texture band matched100%. At the
room start all four authored barriers remained drawable at screen X400,560,
720 and832, including the scenery beyond the original416px draw limit.
No invalid background setup occurred and shutdown was clean. These focused
checks confirm the reported barriers; they do not claim every stage was tested.

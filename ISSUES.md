# MegaManX5Recomp — Issues

Current state (v0.0.1-alpha): the game boots from the PS1 BIOS and plays —
through the opening (including the intro cutscenes, which now decode and play),
into stages, with working controller input and memory-card save/load, and **no
known crashes**. It has not yet been verified all the way to the end.

---

## #1 — Full playthrough not yet verified end-to-end — OPEN

Stage gameplay works and there are no known crashes, but the game has not been
verified from start to finish. If you hit a hang, crash, or wrong behavior deep
in a stage or boss, that's the kind of thing worth reporting — capture where it
happened.

---

## #2 — OpenGL renderer flicker → software is the default — OPEN

The OpenGL (GPU) renderer shows intermittent black-frame flicker in this build,
so the clean software renderer ships as the default (`game.toml [video] renderer
= "software"`). OpenGL remains selectable in the launcher (Settings → Renderer)
for anyone who wants to try it. This is the same class of issue tracked and
root-caused on MMX6 (its ISSUES.md #7: `flush_cpu_upload()` merging disjoint
CPU→VRAM uploads into one union bounding box that repaints live frames from the
stale CPU VRAM mirror). Software stays the safe default for X5 until the GL path
is validated clean here.

---

## #3 — Widescreen (true 2D wide field of view) not yet wired — OPEN (enhancement)

X5 ships **4:3 only** this release. `[widescreen] full_2d = true` is present so
the wide present path *could* engage, but the piece that actually widens the
scene — the per-layer 2D background tile widen (`[widescreen.bg2d]`) — is **not
implemented** for X5. The hook sites have been located and documented in
`annotations/widescreen_bg2d_sites.md`, but they are not configured in
`game.toml` yet: X5's background-column count site is an inline `sltiu` (a
different instruction shape than MMX6's `li`), so the recompiler-side hook needs
a variant before the sites can be turned on. Until then, do **not** expect a
genuine wider field of view — the game presents at native 4:3.

---

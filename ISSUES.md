# MegaManX5Recomp — Issues

Current state (v0.0.2-alpha): the game boots through bundled OpenBIOS and plays —
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

## #2 — OpenGL renderer flicker — RESOLVED

The shared PSXRecomp upload-rectangle fixes resolved the intermittent
black-frame flicker. OpenGL is now the release default; software remains
selectable, and the updated framework also provides Vulkan.

---

## #3 — Widescreen (true 2D wide field of view) — EXPERIMENTAL MOD

The background tile-window, HUD, object-activation, and primitive-cull hooks are
implemented and exposed as the default-disabled **Widescreen** mod. Authentic
4:3 remains the default. The 16:9 path is experimental and should be reported
with the stage and scene when a background, HUD element, or object behaves
incorrectly.

---

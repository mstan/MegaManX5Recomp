# MMX5 [widescreen.bg2d] hook sites (Ghidra recon 2026-07-02)

Structural analogues of MMX6's 2D-background widescreen hook sites
(MegaManX6Recomp/game.toml lines ~117-148), verified by disassembly in the
`SLUS_013.34_no_header.bin` Ghidra program. Same Capcom engine,
instruction-for-instruction, shifted addresses + two constant deltas.

## BG per-layer tile renderer: FUN_80027f88  (X6: FUN_800270d0)
21 (0x15) columns x 16 rows; layer struct base 0x8009A1F8, stride 0x54, 3
layers; scroll lh at +0xA/+0xE; parallax parent lb at +0x52.

| field | addr | instruction | X6 equivalent |
|---|---|---|---|
| count_site | 0x80028218 | `sltiu v0,t7,0x15` | 0x800271d4 `li s6,0x15` |
| startcol_site | 0x80028040 | `andi v1,v1,0x1f` | 0x80027188 `andi v1,v1,0x3f` |
| startx_site | 0x80028058 | `sra s6,v0,0x10` (delay slot of `bgez a1` @ 0x80028054) | 0x800271a0 |

Deltas the recompiler hooks must absorb:
1. X5 has NO register-loaded count and NO hi-res `li 0x21` branch — the bound
   is the inline `sltiu` immediate at the loop compare. The X6 `li`-shaped
   count hook does not drop in; the widen must patch a `sltiu` immediate.
   Column wrap in the loop delay slot: 0x80028220 `andi t3,t3,0x1f`.
2. Tilemap ring is 32x32 (mask 0x1f, row stride 0x40, layer stride 0x800,
   ring base 0x800A51A8) vs X6's 64x32 (0x3f/0x80/0x1000/0x800A21B8). A 16:9
   widen (21 -> ~27 cols) fits, but the widened stream window (~464px incl.
   +/-16 lead) must stay under the ring's 512px world coverage — slack is 11
   columns, not X6's 43.

## Tile-ring streamer: 0x80028278  (X6: FUN_800273e4)
Same 3-layer loop (`sltiu s3,0x3`, stride 0x54); column-stream helper
0x80028328; row-stream helper 0x80028608 (scrollY and scrollY+0x100).

| field | addr | instruction | X6 equivalent |
|---|---|---|---|
| stream_left_site | 0x800282b8 | `addiu a1,s0,-0x10` | 0x80027424 |
| stream_right_site | 0x800282cc | `addiu a1,s0,0x150` | 0x80027444 `addiu +0x10` (+ width var lhu 0x8009B79C) |

Delta: X5 folds the screen width into the right-edge immediate (0x150 =
320+16) — the right-side widen is a pure immediate adjustment, no width
variable involved.

## Packet double-buffer + per-frame cap
- Buffer base 0x800BE9B0 (`lui v0,0x800c; addiu v0,v0,-0x1650`), stride 0x4000
  via `sll v1,parity,0xe` (1024 slots x 0x10-byte packets). Live ptr in
  scratchpad 0x1f800108; per-frame tile counter 0x1f80011c.
- bufbase_site = 0x80027c7c `addu v1,v1,v0` (driver, before `sw v1,0x108(at)`
  @ 0x80027c84) — exact match of X6's 0x80026dc4.
- cap_site = 0x8002810c `slti v0,v1,0x3e8` (1000-tile/frame cap; X6: 0x80027278).

Confidence: high — single BG renderer in the binary (unique `sltiu ..,0x15`
loop; unique caller `jal 0x80027f88` @ 0x80027ca0); driver/streamer/renderer
call-chain shape identical to X6 (slot table 0x80091D60 +0x198 parity).
Ghidra program has bodies unanalyzed — use disassemble_at, not get_code.

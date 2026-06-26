# Disc identity — Mega Man X5 (USA)

Redump-verified clean dump. Format: **bin/cue, single track, MODE2/2352, NTSC-U**.
Do **not** convert to ISO — a 2048-byte "cooked" ISO discards the Mode-2 Form-2
XA sectors PSX uses for streaming FMV/audio (X5 streams CAPLOGO.STR / X5OP.STR
movies + BGM.XA).

| Field | Value |
|-------|-------|
| Title | Mega Man X5 (USA) |
| Serial | SLUS-01334 |
| Revision | Original (only USA revision Redump lists) |
| Redump disc | #7437 |
| Track | 01, MODE2/2352, data |
| Size (.bin) | 582,954,960 bytes |
| CRC32 | `0D0CE609` |
| MD5 | `98C0D278DC4A795A0A7562D950D37CC9` |
| SHA-1 | `10709231F857636B5CCD3CD9ACEBC91458DCB5FD` |

Verified 2026-06-26: locally computed CRC32/MD5/SHA-1, .bin size, and serial all
match the Redump entry for Mega Man X5 (USA) (redump.info disc #7437).

Boot EXE: `SLUS_013.34` — load `0x80010000`, entry `0x8005894C`, text `0x82000`,
stack `0x801FFFF0` (PS-EXE header; SYSTEM.CNF STACK = 801FFF00). $gp is set at
runtime (header gp0 = 0), as in MMX6.

On-disc layout of note (mirrors MMX6's ROCK_X6 overlay model):
- `SLUS_013.34` — boot EXE (the static recomp target)
- `ROCK_X5.BIN` / `ROCK_X5.DAT` — streamed stage/engine overlay (dirty-RAM,
  recompiled via the overlay cache pipeline)
- `STR/CAPLOGO.STR`, `STR/X5OP.STR` — MDEC opening movies
- `XA/BGM.XA` — streamed audio

Disc image and extracted EXE are local-only (gitignored); recreate from the
source dump if missing.

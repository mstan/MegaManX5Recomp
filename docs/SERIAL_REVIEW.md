# Mega Man X5 serial review candidate

SLUS-01334 (USA). Loading has owner approval. The modern renderer awaits owner
review, with two owner passes maximum.

Default review features are OpenGL/1080p internal rendering, adaptive scene
reveal, ordinary enemy placement/visibility and packet-specific HUD anchoring,
plus the previously approved resident asset loading. The old fixed16:9
tile-ring widening path is retired: count, start, streamer and packet-cap
instructions execute unmodified. The modern renderer uses shared
`mod_capcom_bg2d.h` to resolve authored map tiles into a separate, checked,
double-buffered DMA arena. It keeps original palette, transparency and OT
priority groups; menus stay native. Fixed aspect choices use this same renderer.
Additional placement scans call the original scanner and eligibility routines
for ordinary enemies only; bosses, special encounters, NPCs and scene scripts
retain their original reach. The original guest camera is not changed.
This is a 2D sprite/tile renderer: nearest textures are preserved and 3D PGXP
perspective correction does not apply. Stage-wide coverage and extreme aspect
ratios remain subject to owner review.

The resident catalog contains ROCK_X5.DAT (172 members: 165 typed archives and
seven raw resources) and ROCK_X5.BIN (59 raw overlay/data members). The HLE
serves guarded blocking calls and loading-screen retries through the original
receive callbacks. They retain header parsing, member dispatch, RAM/code DMA
invalidation, progress state, final Pause and completion publication. Deferred
GPU/SPU uploads drain through the original queue and DrawSync before buffers
are reused. Original game-side decompressors remain in place.

Shared psxrecomp services own the resident cache, effective-disc file reads,
content/code hashing, RAM DMA effects and checked Mode 2 sector view. X5 owns
its descriptor tables, original ABI, callback addresses and blocking policy.
The adapter verifies archive hashes, the entire initialized descriptor table,
original loader/SDK code and positive callsite instructions before transfer.
Modified/relocated assets, unknown callers or changed code use the original
loader. The two asynchronous actor calls at 0x80056898 and 0x8005695C keep
original drive delivery. Movies and XA music are outside the catalog; generic
drive/host pacing is excluded. Startup archive-header reads remain original.

ENHANCED selects mmx5_resident.cpp. The fixed REFERENCE build selects a matching
feature registration without HLE hooks and uses original drive loading. There
is no live execution-profile selector. Mode 2 EDC/ECC are synthetic and are
ignored by the guarded callback; MSF and data reads follow its original ABI.

Interpolation source and package are preserved outside compiled sources and
the bundled catalog, with the earlier baseline also on
archive/mmx5-interpolation-20261007. It is unavailable in this candidate.
Original-disc AOT recipes and their six explicit exclusions are retained;
native modules must be freshly paired with the current runtime config. No
complete native coverage or Linux release is claimed by this Windows review.

Owner checks: resize while standing still and moving, then run/jump through
the intro and watch enemies enter both revealed edges. Check HUD/dialogue,
room ends and boss activation. Loading retains the approved implementation;
check music and a transition as a brief regression spot-check.

The preceding loader candidate (compiled source 1cf4e32) passed a 45-second startup
and intro/attract check: 29 resident requests, 3,558 sectors, both DAT/BIN
archives exercised, verified local pack reused and clean shutdown. The
requested 1080p setting produced a 2130x1200 internal surface (5x reference).
Original-disc native cache audit passed for all 90 published pairs and byte
guards. Exact binary identity and deferred owner validation are recorded in
the central review receipts. That result applies to the preceding fixed-view
binary; the replacement renderer's exact build and focused check are recorded
separately.

The replacement (compiled source467de64, shared frameworkec8c5c54) passed
its corrected45-second startup/intro-attract check: original-disc90native
pairs/guards valid, 2,159,459 modern tiles, 434 supplemental ordinary enemy
eligibility checks, zero invalid-layer setup, resident loading exercised and
clean shutdown. Its screenshot is2130x1200. This does not claim owner approval,
audio measurement, all-stage coverage or a full performance qualification.

The first owner renderer review found normal gameplay still pillarboxed.
The previous check covered attract state9; player-controlled stages use10.
Correction ee4ac11 enables both states for scene reveal and supplementary
ordinary enemy scans. The native overlay config identity remains e8c0cc39;
its original-disc native pairs remain compatible. Exact corrected-binary
evidence is recorded separately in the central review receipts.

The corrected-binary check reached real player stage state10 and showed106
revealed columns,204,568 modern tiles, one additional enemy eligibility check,
zero invalid-layer setup and a2130x1200 screenshot. Resident loading served17
requests/1,736 sectors; shutdown was clean. Audio/controllers were disabled.
The next owner review is the second renderer pass; no final acceptance is claimed.

That review found foreground barriers clipping at4:3 and being anchored as HUD.
The owner authorized integration once this specific problem was verified.
Category5/type36 scenery now participates in the supplemental placement scan
and original draw-bound expansion. Its negative camera selector is classified
as scenery, and sprite/quad packet positions retain the retail5/4 scroll mapping
through the modern room-edge view. Menus/dialogue retain their UI selectors.
The skyline uses the shared half-speed parallax helper.

The final focused checkpoint check passed16:9/32:9 and the room start: a64px
camera step moved the actual rendered barriers80px, with100% matching pixels
in the compared texture band. All four authored barriers at X400/560/720/832
remained drawable at the widened room start. Central receipts bind these
checks to the final executable. Integration is authorized by that result;
full-stage/audio/performance coverage is not claimed.

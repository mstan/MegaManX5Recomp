# Audited native frame wait

SLUS-01334's complete 152-byte SDK routine at `0x80062FC0` matches
the qualified Ape Escape routine after relocating the paired frame counter,
diagnostic string, callbacks and internal jump. It polls `0x8007FA70` and
decrements one private stack timeout. The audited edges are `0x80062FE8`
and `0x80063030`; timeout callbacks remain `0x8006C210`, `0x8006C438`
and `0x8006C448`.

The enhanced build selects the existing portable induction path. Reference
builds retain original polling, and netplay/precise modes retain their existing
exclusions. Selection and audited sites enter execution identity, without a
player-facing switch. The actual generated caller is checked by
`mmx5_vwait_contract`; game code is private regenerated input.

The original PR-only audit is recorded in `beads-eio.10.37`. On 2026-10-27 the
owner requested integrating PR 14 with the accepted local co-op work and cutting
a new Nextcloud release; the owning task is `beads-eio.10.39`, related to
`beads-2wy8.1` (optimization campaign). Source changes remain local.

The latest game master and PR heads are frozen at c8fde08b and 1267f57e.
The accepted co-op tree is transplanted unchanged from 3ed23ec onto that base;
its original branch/history remains intact. The framework combines PR pin
ffbd9d1c with the 22 retained co-op lineage commits through 59cae0a9. The launcher
also retains separate-controller, instance-zero capture and hidden diagnostic
support. Local-pair rendering keeps its native canvas, two-eye indexing,
presentation scope and exemption from VR frame shedding. Current-build host
snapshots and watchdog behavior remain part of qualification.

The subsequent owner check found a brief shared HUD during initial local
arrival. The game now seeds both private cameras at enrollment, keeps the
paired arrival draw active through the native landing phases, and prevents
the duplicated fallback from showing both meters during that handoff. This
is the only gameplay-source change after the accepted tree was transplanted.

Built performance is compared with the delivered v0.2.4 co-op executable on
matched X5 workloads. This is a combined framework/title package comparison;
do not attribute any measured change solely to wait induction. Keep neutral or
inconclusive results explicit, and do not adopt a reproducible X5 regression.
Controlled gameplay and current-build save/load must pass on the integration.
The older campaign, physical LAN and native-card unlock gap remain separately
tracked; this uptake is not a full campaign parity claim.

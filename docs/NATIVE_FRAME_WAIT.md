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

This repository update is a PR only. No X5 release or whole-game speed claim
is implied. Evidence and qualification limits are recorded in central Bead
`beads-eio.10.37`, related to campaign `beads-2wy8.1`.

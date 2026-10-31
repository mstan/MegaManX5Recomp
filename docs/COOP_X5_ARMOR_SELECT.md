# X5 co-op stage armor selection

Current owner design, 2026-10-27: use the original unit selector twice, first
for P1's campaign hero and then for P2's counterpart. Keep one X and one Zero;
each seat chooses only forms of its assigned hero. Black Zero remains a separate
native-looking choice, available only after the Sigma stage 3 capsule unlock.
The co-op Black Zero preference does not grant that unlock. See
[the current owner burndown](COOP_X5_023_OWNER_BURNDOWN.md) for the new capsule
checks. The X-only selector rows below retain their historical evidence from
before the owner requested consecutive choices for both players.

Owner design, 2026-10-25. Local issue `beads-eio.10.21`.
Continue `feature/mmx5-x4-parity-local`; preserve the existing Grizzly fixes.

P1 controls boss selection and the other frontend menus. At the stage
character/armor selector, remove Zero and list the native available X armor
variants. The player already assigned X owns left/right and confirmation,
including when X is P2. Selecting an armor must not swap hero seats or change
the canonical campaign hero. Owner clarification: Fourth Armor and Zero's
Z-Buster are always available in co-op, on either seat, preserving solo behavior.

| ID | Required behavior | Evidence needed | Status |
|---|---|---|---|
| A01 | Preserve the established X/Zero seats across stage selection and entry. | Actual native selector entry and resulting bodies in both seat assignments. | Passed on170B32A6: actual native X/Zero new games establish both assignments; original stage confirmation/entry preserves each. A genuine91-mask native card Load also retains its native default hero, clear bits, availability and card bytes. |
| A02 | P1 owns boss selection and other frontend menu input. | P2 cannot move/confirm the boss cursor; P1 can. Cancel from armor selection still returns to the native boss menu. | Passed, bounded: P2 cannot move/confirm boss selection; P1 selects and cancels/re-enters armor selection. Native Parts opens with Zero retained. Parts-tab return is outside this receipt. |
| A03 | Stage character selection lists X forms only: always Fourth, plus other natively available forms. | Inspect actual list/count and rendered selector; Zero absent, available X variants retained. | Passed, bounded: fresh native X-only list0/1, Fourth available even with original Zero-start availability0. Setup mask0E produces native order0/1/3/2/4; acquisition is explicitly bypassed. |
| A04 | The X player alone owns left/right and armor confirmation. | Actual inputs from each seat; wrong seat cannot move/confirm, X can select and enter. | Passed on170B32A6: wrong seat cannot move/confirm; X/P1 and X/P2 each select through original native input handling. |
| A05 | The chosen form has its native appearance and abilities on X in either seat. | Native form initialization, assets/parts, a distinctive movement/attack and resource ownership. | Passed owner82A36EAA Gaea station visual retest. Owner170B32A6 screenshots F2178/F2420 remain failed evidence. Local f651130 retains static shot origins and native Gaea compressed atlas uploads; actual FDFD7148 shots now show clean gold muzzle/green projectile in both panes. All five forms retain bounded entry/control evidence. The new visual receipt is owner-effects-20261025/fixed-final; no acquisition or exact-phase comparison claim. |
| A06 | Preserve current-build save/restore and compatible netplay ownership. | Actual save/load and two-peer matched simulation/render evidence on the combined candidate. | Unverified for final LAN/X-P2 armor-menu acceptance. Current-build warm snapshots pass both assignments; cold native snapshots and eight matched local delay-sync checkpoints pass (P2 Fourth, default silent hints, movement/attacks, independent menus, zero private-draw faults). Additional native-card network check passes X/P1 boss ownership, wrong-seat rejection, P1 Cancel, X-only roster, Fourth selection and actual stage entry at three matched checkpoints. X/P2 online selection and physical LAN remain manual. |
| A07 | Both heroes retain their bonuses in co-op: Fourth available to X and Z-Buster usable by Zero, on either seat and campaign start. | Real X/Zero new games, actual Zero weapon selection/shot and X form initialization; solo remains native. | Passed on170B32A6: both real new-game assignments have X FourthFA1 and native Zero Buster action3E, retaining genuine campaign1BA. Co-op-disabled Zero-start keeps native availability0 and original roster0/5 (Fourth absent), and its native Buster still fires; no co-op grant/enrollment runs. |

Use the original selector and availability rules with co-op Fourth always
available. Keep native persistent
save contents separate from disposable build-specific snapshots. Initial
campaign setup establishes the seats; this change targets the stage picker.
Keep diagnostics and default Alia suppression, and preserve the completed
Grizzly checks. Remaining stage checks stay in
[COOP_X5_GRIZZLY_BURNDOWN.md](COOP_X5_GRIZZLY_BURNDOWN.md).

Qualify both X seat assignments because they use different input and asset
paths. Add other cases only for distinct native form mechanics or observed
failures; do not multiply every stage, form, seat and camera combination.

Current inventory: **6 passed / 1 unverified / 7 total** (1 open). A05's Gaea projectile graphics fix has automated and owner visual
evidence; A06's LAN armor-menu interaction remains manual.
These are bounded checks, not additional character/stage matrices.
All other menu, bonus and current-build mechanics above have executable evidence.

Evidence root:
G:/Temp/Codex/psxrecomp/x5-parity/owner-armor-finish-20261025/

- candidate-x-final/session.json and candidate-zero-final/session.json:
  native new games, seat-specific input/Cancel and real stage entry, warm restore.
- candidate-forms-final/session.json: native base/Falcon/Gaea/Ultimate selections,
  actual native movement/attacks, private draw checks, restored fixture per form.
- ultimate-nova-current/session.json: real P2 Nova22 and independent P1 movement.
- native-card-current/session.json: genuine native card recognition/load and
  unchanged card SHA348b9e39..., stored clear mask91 and native availability.
- solo-native-current/session.json: actual co-op-disabled Zero new game,
  native Z-Buster and unmodified native roster without Fourth.
- paired-current/report.json: eight matched checkpoints on the same candidate;
  local CLI delay-sync peers, not physical LAN evidence.
- armor-netplay-audit/x-primary/report.json: three additional matched checkpoints
  from actual native-card Load through X/P1 armor selection and stage entry,
  unchanged card bytes, no RAM writes or snapshots. Reproduce with
  tools/test_mmx5_coop_armor_netplay.py. X/P2 is not covered by this receipt.
- grizzly-true-p1/session.json and grizzly-qualified-final/session.json:
  native door/capsule regression recorded separately in the stage burndown.

Candidate SHA256:
170b32a61982994453966cd8e1627b495c2e5a99da8e126c81bbb37e8dbd0589.
Frontend setup uses native mode3/80020CB4 and bypasses stage completion only.
Other-form setup may seed availability; it does not qualify armor acquisition.

Native13458 maps forms0/1/2/3/4 to DAT88/92/94/95/93. Original27370 supplies
ordered armor layers; P2 must project its own global assembly directory as well
as body pointers. Immutable UI sources remain valid for retained banks.
Snapshot provider12 stores form, frontend input history and filtered-roster
backup; older snapshots are disposable.

Gaea construction queues type1B effects before P2 enrollment completes. The
old allocation guard skipped their owner tags, so deferred50D04 interpreted
Zero's packed graphics as X's directory and182D8 overwrote guest text.
Existing watchdog captures and host stack identified this; ownership now tracks
projected construction. Fresh Gaea-only setup reproduced the original hang.
The fixed candidate runs both effects and normal gameplay without the hang;
watchdog behavior is unchanged.

Native1BA also records starting-campaign identity. Co-op Z-Buster overrides only
its three UI/ability consumers and never overwrites that native save field.
Fourth availability is a temporary stage-roster policy, not a native unlock
grant. The original table, including Zero, is restored for other frontend menus.

Continuation audit: creating a native Zero-campaign card for the P2 online case
by jumping directly to the Save prompt from the current fixture halted the
native dispatcher. This was an invalid setup, not a passed save or armor check,
and no production change follows from it. Its stopped-process log and receipt
are retained in armor-netplay-audit/zero-native-save.*. Do not repeat speculative
save-entry variants; use a normal Zero-campaign save or the owner LAN run.

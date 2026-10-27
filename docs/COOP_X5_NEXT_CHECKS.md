# Bounded X5 checks from the frozen X4 audit

Source audit: 2026-10-20. X4 remains frozen at Playtest42,
`50593bca772b600de6911cbfecd40b0f08721147`; X5 runtime source reviewed at
`e4c4f4b` (candidate `82084041`). This document selects seven reusable checks
from the existing 234-report inventory. It adds no comparative ledger rows and
qualifies no runtime passes. Main owns the live runner, fixes and ledger.

`F#` below means the individual numbered report in
`COOP_PLAYTEST_FEEDBACK_20261010.md`; `L#` means
`COOP_PLAYTEST_REGRESSIONS.md`. They retain their separate ledger dispositions
even when one mechanic check supplies evidence for several reports.

| Priority / existing reports | X4 failure and source contract | X5 finding and present limit | One bounded decisive check |
| --- | --- | --- | --- |
| High: F55, F71, F149, F154; P19/P22 | Capsule grant can use the campaign's character, story text or palette rather than X/P2. Frozen `mmx4_coop_lifecycle.c:1409` (`x_capsule_canonical`) routes capsule components to living X, including the child upgrade animation; `:1429` scopes counterpart Dr Light text. | `src/mods/mmx5_coop_plugin.c:356` (`enter_second`) always selects roster slot0 for counterpart X. `:1901` (`ai_update`) chooses the nearest living hero for shared AI and preserves the campaign identity; there is no capsule-specific X-owner filter among `:2812` onward registrations. Native armor availability/art need actual X5 qualification: a nearest-player projection alone is not the X4 capsule contract. This is an ownership risk, not a demonstrated capsule failure. | Teleport to one actual authored armor capsule in Zero/P1's campaign with X/P2 approaching and Zero nearby. Let native interaction, dialogue and grant run; inspect both return controls, original part flags, X art/palette and the granted part's actual ability. Use native save/reload to check persistence. If X5 grants individual parts without immediately enabling the complete armor, record that native rule and leave the ability component open until prerequisites are earned; do not force reward flags. |
| High: F15, F16, F60, F75, F93, F188; L16; P28 | Chaser entrance can lose the shared camera delta; private dash can accelerate the other rider or lose pose/effect priority. Frozen `mmx4_coop_plugin.c:541` keeps the canonical entrance camera until native mode0 ends. `mmx4_coop_combat.c:383` retains each vehicle effect's source. | X5 `mmx5_coop_plugin.c:807` seeds P2 from the initialized native car, calls `8003D784`, and restores rider-specific entrance fields. `:1046` keeps car cameras shared; `:1055` runs the private car update; `:235` rewrites exact `+50` car sources to owner mirrors. Prior fatal-order/control evidence does not qualify checkpoint0 READY, dash ownership or layering. | Enter native stage5/checkpoint0 once and retain car/rider phases, camera current/previous origins and READY/world count through control release. Then hold P2 dash with P1 neutral, comparing both native velocity/displacement, rider animation and exhaust layering over the same ticks. No whole-road traversal or roster/camera cross-product. F75 remains online-unverified until this setup is included in main's combined netplay candidate. |
| High: L3, L12, F4, F133; P10/P21 | Deferred death particles can inherit the surviving hero's origin/art; character sounds can steal one another's voices. Frozen `mmx4_coop_combat.c:475` records particle owner/origin and `:492` projects that owner during death/elemental initialization. Frozen `mmx4_coop_audio.c:142` and `:169` isolate character banks/private voices. | X5 `mmx5_coop_plugin.c:1204` records original death callers `8003DABC/8003DB3C`; `:1221` restores origin/layer/assembly for deferred init, but does not project the entire owner's palette/context. That difference warrants inspection, not an automatic patch. `mmx5_coop_audio.c:139` routes projected groups1/3; `:166` preserves native queued KEYON state. Existing evidence covers jump/buster overlap, expressly not hurt/death. | With heroes visibly separated and contact protection removed for the tested contact, let a native enemy hurt P2 while P1 jumps, then cause P2's native fatal sequence while P1 remains controllable. Retain accepted HP/reaction, sound group/tone/private-slot lifetimes, one death burst with correct origin/color, expiry and survivor movement. Keep output muted while preserving emulation. A damage-free fatal setup does not count as the hurt pass. |
| High: F19, with F12/F14 as observations; P07 | Ordinary dash exhaust follows canonical PLAYER after its allocating seat has been restored. Frozen `mmx4_coop_combat.c:413` explicitly projects owned deferred movement effects (types0/1); fixed-origin dust remains shared. | X5 already retains owner on effect allocation (`mmx5_coop_plugin.c:1255`), but its projected shared-effect updater at `:1266` explicitly handles only type2 buster release. `:1107` separately advances retained glow/trail pools. Successful trails therefore do not qualify ordinary dash exhaust. X4's type numbers are not presumed to map to X5. | With P1 stationary and both visible, dash P2 away, let exhaust expire, and dash back so a native effect slot is reused. Inspect the actual composed origin/facing/colors and continued P1 body visibility. If exhaust is wrong, identify its native initializer/update and allocating owner before changing code; avoid expanding to unrelated special weapons. |
| Normal, elective: F180; P18/P22 | Separate E-tank energy must not split acquired tank ownership or consume the other seat's supply. Frozen `mmx4_coop_lifecycle.c:168` stores private tank contents, `:180` merges campaign acquisition bits, and `:258` reads `separate_etanks`; frozen co-op manifest lines89-95 default it off. W-tanks stay shared. | X5 co-op manifest has no separate-tank option. Native capacity/content remain shared (`mmx5_coop_plugin.c:672`, QA setup: `PLAY+7E` acquisition, `PLAY+76` contents); `PlayerContext` at `:40` has no private tank store. Existing shared-tank/menu tests do not implement this elective X4 option. | After implementing the narrow option with default shared behavior preserved, initialize unequal HP/supplies and use each hero's native menu to heal once. Verify only the intended private contents are consumed, shared acquisition remains correct, and one actual native save/reload follows the documented persistence policy. Do not infer private persistence from a card checksum. |
| Normal, elective: F37; P19/P30 | Automatic Ultimate X and Black Zero choices must coexist without transferring the other hero's code/resources. Frozen `mmx4_coop_plugin.c:469` selects X's variant independently; `:1574` reads both options. Frozen `mmx4_cheats.c:44` applies the scoped character code, while Ultimate Armor still requires its native capsule (manifest129-141). | X5's co-op manifest declares neither option; `enter_second:369` forces counterpart X to base roster0. `keep_zero` only changes offline story availability and does not supply either enhancement. Native X5 enhancement contracts must be identified before adaptation; X4 Black Zero's palette-only contract must not be assumed for X5. | Establish the original X5 code/roster/ability behavior, then implement only the title-appropriate options. With both enabled in one boot, inspect each hero's native art/abilities, own-menu return and one section transition while retaining campaign/story flags. Any capsule requirement remains a native grant check, not an option that writes completion flags. |
| Normal, elective: F63, F181, with F207 recovery; P23 | Co-op scaling can lose fractional damage, change weakness identity or endlessly restart a stun. Frozen `mmx4_coop_balance.c:308` uses one tenths-of-HP carry per boss health bank (X weight5, Zero4); `:390` preserves native reaction strength; `:397` treats cooldown/recovery separately. Manifest107-125 makes scaling default-off with separate/shared cooldown choices. | X5 has no balance module or scaling/cooldown options. `mmx5_coop_plugin.c:1286` provides per-seat contact tokens, which are not independent accepted-hit cooldowns or fractional boss HP. Source review cannot transplant X4 boss tables or its Slash-specific stun branch. | First locate X5's actual accepted-damage and native reaction boundaries. On one live boss, compare a bounded mixed X/Zero accepted-hit sequence with one rejected contact; check the fractional total, original weakness reaction and eventual recovery. Validate the chosen default-off scaling policy before adding another cooldown mode; no boss-by-roster matrix. |

A deferred source observation: X4 `pickup_update_canonical:1318` cancels a
pending healer when its owner dies; X5 `pickup_tick:1381` reuses the recorded
seat without rechecking lifecycle. Normal large-heal world freeze may prevent
the overlap. Preserve F110's camera/normal-heal pass; do not add another test
variant unless an existing native fatal/script setup or observed failure makes
that overlap concrete. This is not a demonstrated regression.

The practical first batch is the Chaser entrance/dash, ordinary dash exhaust,
and separated hurt/death checks on the main candidate. Capsule grant/save/load
needs its authored setup and is a separate progression check. The three absent
elective options are implementation gaps, not runtime failures inferred from
missing UI; they remain individual unverified parity obligations.

Source evidence for this audit is read-only. Frozen lifecycle/plugin files
are under `build-tools/x4-playtest42-code`; combat/balance sources are also in
`build-tools/x4-reference-playtest42/src/mods`. Additional audio/cheat/manifest
files were read with local `git show 50593bca:PATH`, preserving the same freeze,
and saved under
`G:/Temp/Codex/psxrecomp/x5-parity/subagent-x4-qol`. No runner, build, source fix,
ledger mutation, tracker update, staging, commit or remote operation was done
by this audit.

Main follow-through: candidate4D0376A8 ran the bounded Chaser check in
`qa-victory-20261020/chaser-native-entry-dash`. Both native entrances and private
P2 acceleration are mechanically qualified; READY retirement and unobscured
rider/exhaust layering remain unverified. This evidence does not close those
full comparative rows. Native Grizzly victories now pass with each owner;
feedback90/128 close separately after composed-image review. See the main ledger.


Follow-up original-source/runtime evidence (2026-10-20): ordinary dash exhaust
is native type1/4F0B4, separately from retained trails. The reproduced allocating-
owner bug is fixed and bounded qualified on856484F5, including native expiry,
original art/facing/offsets and P2-to-P1 slot reuse; feedback19 is passed. See the
burndown for exact receipts and limits. This does not close post-door dash or
vehicle exhaust.

For the actual regular capsule check, Grizzly checkpoint3 placement35 is
subtype3 at authored(8810,145); observe the instantiated actor/floor before
position setup. Original55798 permits either hero, and55D24..55DC8 grants
shared PLAY+A1 bit08, serialized in slot59. Complete armor art/ability requires
its native prerequisites; neither an X-only filter nor an immediate ability
claim is justified by this one part. Runtime grant remains unverified.

Cold native card Load now passes on856484F5. Native disk-buffer1C904 uses
slot5F for its default hero; current-data1CC34 uses slot58 availability.
Loaded-game character input is04/04/05, with cursor PLAY30 and original roster
800F6DF0. Preserve this X5 behavior rather than enforcing X4 campaign identity.
Host snapshot/rewind parity is still missing. Cleared portraits use alternate
authored tile descriptors; do not judge their source contract as a palette-only
grayscale switch. Actual tile/art comparison remains open.


Content follow-up: the original CP3 capsule access proposal did not admit the
actor; native cameraY240 excludes authoredY145. Earlier-checkpoint far jumps
also skip route-camera prerequisites. Stop capsule setup variants until a
concrete native upper-route entry exists. The generic offline
`Runtime.teleport_pair` moves both positions and current camera coherently,
within existing current/target bounds, and is qualified only as setup.
It must not be used to claim camera/traversal/progression.

The source-qualified next exit recipes are in
`G:/Temp/Codex/psxrecomp/x5-parity/subagent-x4-qol/next-stage-transition-batch.json`.
Stage6 class11/subtype0 and stage4 class0C/subtype0 need actual native damage,
their distinct cleanup timers and original section exits; neither is a free
reserved controller or a source-proven miniboss. Both still need a valid
upstream route/camera setup. Native stage6 ladder ownership now has bounded
runtime evidence, independent of those exits. Raw14/0 is startup frontend;
raw11/1 has no checkpoint record and must not be forced through mode9.

# X5 playable X-versus-Zero story duel

Owner design, 2026-10-22. Local branch: `feature/mmx5-x4-parity-local`.
This replaces the undecided X5 encounter proposal attached to comparison F62.
It does not add consecutive fights or import Double/Iris into X5.

Owner steering, 2026-10-23: park this duel for final human validation while
finishing all other feasible X4 comparisons. Keep the implementation and
partial evidence below. F62 remains unverified; no complete duel/parity claim
is made and further duel investigation does not gate the non-duel sweep.

Current candidate11A66B52 advances disappearance to the native explosion's
white-flash ramp, per the owner's1864D0A4 retest. Native575D0 signals PLAY+2A
above16/31 brightness; both NPC result scripts consume that signal into
phase3. The losing body hides at that phase while its original class1A
controller remains active, before full white. No new timer or snapshot field
is needed. The earlier crouch, loser-owned effects, HP and story logic remain.
Evidence: `G:/Temp/Codex/psxrecomp/x5-parity/owner-duel-flash-20261026`.
Four actual-fatal checks pass: either winner after each real inner-door path,
with visible crouch before the flash and hidden/FALLEN body during the live
phase4 controller, before full white. Crouch save/load passes on both paths;
winner HP, retries, no-orb scans, native result dialogue and zero view leaks
pass. All24 rebuilt tests pass. The snapshot fixture was brought up to the
already-existing six armor sources/schema12; the runtime format is unchanged.
Owner visual acceptance remains pending.

Prior candidate1864D0A4 implements the owner's duel-only defeat sequence:
crouch, explosion, then vanish. The actual accepted fatal selects the native
player pose51 used by BIN28 stand48/EECC0. The loser remains DYING and visible;
its ordinary fatal dispatcher never runs. Native animation/settling advances
once per canonical world, using the same proven flat arena floor as arrival.
Only after the original class1A controller retires and its result handshake
advances does the body hide and become FALLEN. The private NPC can therefore
sequence the result while the actual loser is crouched, rather than waiting
for normal death orbs. Winner HP and result identity remain independent.
Existing RAM/body/lifecycle snapshot fields cover this sequence; no new
serializer fields or native-card changes are introduced. Evidence:
`G:/Temp/Codex/psxrecomp/x5-parity/owner-duel-crouch-20261026`.
Five grounded accepted-fatal checks pass: both winners for both native NPC
paths, plus a17HP winner. No death11 orb actors appear; each loser remains
visible in pose51 through the blast and becomes hidden/FALLEN only after its
controller retires. Two saves/loads during the crouch continue through the
same result. Winner HP/retries, result dialogue and zero view leaks pass.
A normal intro fatal still creates12 ordinary death-orb actors. A bounded
airborne setup failed to produce a hit; airborne final-hit settling remains
unverified, not a passing check. Full later story/card/LAN limits remain.

Prior candidate8EA4F4CA corrected the owner's9D02E26B defeat report.
Native story completion always defeats its hidden NPC shell, even when the
boss-side player won. HUD calls now temporarily display the actual duel
ledger and restore that private shell state immediately. A fallen P1 entrant
still supplies the left HUD's own body and art while P2 owns the world.
Both original NPC scripts allocate a class1A explosion controller during
defeat phase1->2 and retain it at shell+80; that specific new controller is
placed at the real loser's coordinates before updating. Timers, children,
story signaling and ordinary player fatal animation remain native. No new
serialized fields or native-card format changes are introduced.
Evidence: `G:/Temp/Codex/psxrecomp/x5-parity/owner-duel-defeat-20261026`.
Five actual accepted-fatal checks pass on8EA4F4CA: either winner for each
native NPC/entry path, plus a17HP boss-side winner. Captures show the losing
meter empty and the correct winner fill/badge; raw native controller XY equals
the fallen loser's position. All cases reach native result dialogue with
retries unchanged and zero local-view leaks. All24 CTests pass. Setup uses
positions and positive defenderHP1, never winner/death/result flags. These
checks do not qualify later story chapters, endings or native cold saves.

Prior local candidate9D02E26B corrected the owner's82A36EAA visual
failures. Zero's native intro hovers with a valid standing box before it lands;
the real participant now arrives during that conversation using the actual
entrant's floor. The hidden shell's authored movement/dialogue is unchanged.
Both32HP meters become visible together at control release; native96HP damage
trail residue is clamped to32. P2 entrants keep native HUD ordering/anchors
and project only their sampled UI tiles within the existing view transaction.
Actual P2 entry, native movement/menu, current-build active save/load and a
real2HP X hit pass, with zero local-view leaks. Owner visual/balance acceptance
and the existing story/native-save/LAN limits remain open.
Evidence: `G:/Temp/Codex/psxrecomp/x5-parity/owner-followups-20261026`.

Historical manual QA delivery084830CA retained the33E9009F duel polish and damage
profile. Its Sigma layer projection is guarded to raw0; normal partner painting also
precedes P1's base/armor so overlapping heroes no longer interleave pieces.
The replacement includes a fresh build-specific pre-door snapshot with native
save/restore and cold-launch checks; it does not repeat all prior duel combat
qualification or resolve the remaining F62 story/LAN/persistence gaps.

## Required behavior

1. Either player can trigger the existing fight at its second boss door.
2. The other hero performs the outgoing beam, then arrives in the original
   opposing NPC's area rather than returning beside the entering hero.
   The non-entering player becomes the boss, whichever physical seat enters.
   A partner who died earlier in the stage still returns for this duel; prior
   stage death must not remove that participant from the encounter.
3. The opposing participant looks exactly like the corresponding native NPC.
   Correct appearance includes its actual graphics, assembly, palette and
   animation; a similarly colored player sprite is insufficient.
4. The opposing participant's personal life meter disappears. The native
   right-side boss meter represents that participant's health, with one
   authoritative health bank and no duplicate visible or vulnerable avatar.
5. The two controllers fight each other. Damage must originate from actual
   eligible attacks and native collision; no autonomous third combatant remains.
6. Both participants retain their normal player movement, attacks, weapons
   and full native Start-menu move selection. The boss role does not substitute
   the NPC's boss attack set for normal player controls.
7. The winning character determines the subsequent story. A loss must not
   accidentally consume a co-op team retry or run the losing campaign's exit.
8. Owner manual-QA decision, 2026-10-23: start both participants at32HP,
   disable health recovery and damage-scaling bonuses during the duel, and
   tune damage for PvP while retaining normal moves and native menus.
   Keep earned campaign maxima, equipment and native card data intact.

Owner feedback also rejects a visible NPC stand-in followed by a second
arrival of the actual player. The non-entering player should arrive once;
the authored shell and its independently drawn aura must stay hidden. Both
health meters should have stable dimensions and matching vertical anchors,
including their frames and level text, from the first boss-meter draw.

Role assignment and normal controls are settled owner requirements. The meter
policy follows the non-entering boss participant, including when that is P1.

Suitable-scene staging (F151) remains a separate deferred elective request.
The duel decision does not authorize rewriting unrelated cinematics/endings.

## Duel owner feedback and replacement candidate (2026-10-23)

Owner reports an initially oversized/misaligned boss meter, lingering Zero
aura and a visible NPC followed by a second player arrival on 6987087D.
Owner approves equal starting health, no healing or damage-scaling bonuses,
normal moves and native menus, then requests replacement of the local QA build.

33E9009F hides only the retained native shell and its owned intro children.
The real participant arrives once at the original grounded anchor during
the native introduction; controls release when the original introduction
actually completes. The original actor remains available for story logic.
Both native NPC/entrant paths pass, including retirement of Zero's two
class24 aura actors. No real player's effects are cleared. Captures confirm
aligned bar/fill/frame/level text and no stationary aura after movement.

Actual entry normalizes current health to32 each. Display-only maxima are
scoped to the native HUD call; earned32/34 campaign maxima remain unchanged.
Actual eligible native collisions use the bounded PvP profile and retain
native recovery/token/disposition. Difficulty, armor and parts cannot scale
the accepted injury. Actual X basic, Zero opening slash and X full charge
deal2/3/5HP respectively without self-damage. A genuine large health pickup
at the injured participant remains unconsumed; E-tank confirms are blocked.
Both native Start menus and an active-duel current-build save/load pass.
A positive-HP1 final-hit setup followed by an actual X/P2 attack selects the
native X result text with retries unchanged. This is an initial tuning
profile for owner feedback, not a claim of competitive balance.

Snapshot schema11 includes intro/arrival/health state and rejects old states;
the deterministic digest includes the same new fields. All23 CTests pass.
The exact final executable also receives the existing seven-checkpoint
two-process Split movement/attack/personal-menu regression and a cold launch
of its fresh pre-inner-door fixture before packaging. These bounded checks
do not qualify physical LAN or every duel/result/ending interaction.

F62 remains unverified: complete appearance, remaining story/ending branches,
native cold-save winner persistence and current LAN duel remain open. Native
cold-save winner persistence is not implemented. No frozen comparison is
closed by this replacement, and general automated burndown stays stopped.

Evidence: `G:/Temp/Codex/psxrecomp/x5-parity/owner-duel-polish-20261023`,
including `handoff-native-duel/session.json`, `handoff-combat/report.json`,
`handoff-second-NPC/report.json`, `ctest-HUD-group.log`,
`paired-current/report.json` and `duel-cold-check/report.json`.
Earlier rejected HUD variants and their visual failures are retained.

## Native implementation findings

The earlier owner playtest candidate was D1B93AFC. Historical local
candidate 7EF32812 has bounded native entry, revival, reciprocal injury,
weapon selection, restore and genuine winner/result evidence below. The complete
playable duel is not qualified.
Source and retained runtime evidence establish these integration boundaries:

| Concern | Existing contract and implementation consequence |
|---|---|
| Entrance and passenger | `door_tick_canonical`, `scene_begin` and `scene_tick` in `src/mods/mmx5_coop_plugin.c` already select an entering seat and own the passenger beam. The duel needs a latched encounter-specific role before the ordinary return. It must honor the owner's either-player trigger even with Both-player doors selected. Identify the actual inner door; do not convert every raw18 door into a duel trigger. |
| NPC identity | Original BIN28, raw18/0, has class40 NPC X in Zero's campaign and class41 subtype0/1 NPC Zero in X's campaign. DAT140/141 and native assembly differ from the private DAT88/90/93 player sources. The F84 canonical-image fix must remain correct when the entering hero and campaign differ. |
| Normal player controls | Keep the non-entering seat's normal player body, movement, weapons and Start-menu move selection. NPC-identical presentation and the right boss meter must follow that body without replacing its controls with native NPC AI or leaving a third vulnerable actor. |
| NPC X dispatch | `800F4D7C` dispatches actor+4 through `800FE1B8`; live handler `800F2880` dispatches actor+5 through `800FE1C4`. The live wrapper retains native `800317B8` NPC-to-hero contact, `80031DD0` player attacks against the NPC and hurt/recovery. A negative accepted result enters the original defeat state. Do not replace the entire live wrapper with input handling. |
| NPC Zero dispatch | Class41 root `800F8B60` uses actor+4 table `800FE694`. Live handler `800F77A8` preserves damage, defeat and recovery before dispatching actor+5 through `800FE6A0` at `800F7A84..800F7AA0`. These original routines supply presentation/combat/story references; their authored attack commands are not the required player moveset. |
| Friendly fire | Ordinary co-op contact projects each eligible hero against shared enemies. The duel must restrict each attack to the opposing participant, avoid self-hits, preserve attack tokens/expiry and leave ordinary stages cooperative. Routing a controller to an NPC is not yet proof of reciprocal damage. |
| HUD ownership | Native `80025828` contains both left-player and right-boss sections. `PLAY+1F` gates both and cannot hide just one participant. The right meter uses `PLAY+24`, actor pointer `PLAY+20`, actor HP+5C/trail+5D and maximum `PLAY+A2`. Native `80026334..8002634C` substitutes48 for maxima below32; transferred player health needs a deliberate display mapping. Both appended shared-screen HUD and split-view native HUD paths need the same role policy. |
| Loss and story | Current survivor handling intentionally prevents a dead P1 from ending the world while P2 lives. A PvP loss requires encounter-specific winner resolution instead. The native NPC-death story request and ordinary player-fatal/retry path are different; winner-selected story must be proved through an actual resulting transition and native save/reload. |
| Native story request | NPC X's death table `800FE1F4` reaches `800F5110`; NPC Zero's `800FE72C` reaches `800FA38C`. Both ultimately request `PLAY+0F=10`. Resident `8002071C..80020744` selects mode13/checkpoint0D or0E from existing `PLAY+CB` bit80, with no winning-player field. Ordinary hero death goes to its fatal/retry machinery. Neither path alone implements winner-selected campaign context. |
| Native save identity | Winning story character needs independent state while each seat retains its normal body/menu identity. Native writers do not serialize selected `PLAY+44/49`; genuine `PLAY+1BA`, saved at slot5F, also affects native abilities and must not be repurposed. Runtime/snapshot identity alone does not establish cold native-save persistence. That storage contract remains unresolved and unverified; native card progress must remain intact. |
| Restore and netplay | Latched door/seat/character roles, encounter phase, input edges, health ownership and committed winner affect simulation. Include them in deterministic digest and current-build snapshots. Resolve one winner once; rendering and extra pane draws must not decide damage or story. Old save-state compatibility is not required. Native card progress remains protected. |

## Bounded implementation and verification

Integrate the settled role and normal-controls contract through the inner-door
handoff, NPC-identical opponent presentation and its health ownership. Keep this
work local and preserve the existing owner playtest candidate until a new
candidate is qualified.

The current hidden/muted qualification uses candidate F58BE959
(`f58be95969ea01651add885432a1a2ffd9bb52f9d02222eebd2df7c3ccab1189`).
One genuine native Load and two reused encounters cover the two distinct
entering-seat paths. P1 enters with P2 previously fallen; then P2 enters with
P1 previously fallen. In each encounter:

- The actual inner door and native introduction complete before the other
  player returns at the native NPC position. Previously fallen P2 returns
  with 32 HP; previously fallen P1 returns with 34 HP. Acquired moves, selected
  weapon and ammo survive revival; shared retries stay at two.
- Both players move independently, open/resume their own native menus, and
  inflict actual reciprocal nonlethal native injuries without self-damage.
  X selects earned C-Shot through the actual native menu (weapon0->1).
- A real slot7 save/restore through the native menu retains duel roles,
  health, ammo, selected weapon and menu ownership, followed by resumed play.
- The hidden NPC's owned charge/aura effects retire through their native
  paths. The actual effect-pool scan finds no surviving owned aura. Neither
  player's own effects are cleared by that retirement.
- Both encounters report zero local-view state leaks.

The source card and private cards are unchanged; the owned runner is stopped.
Its quit request returned busy/frozen, so this does not qualify graceful exit.
The raw observer's older scope/unverified strings are retained; the explicit
case probes and qualification receipt record the narrower current results.

The subsequent result milestone uses 7EF32812
(`7ef3281279dc6c707d33a709ae31e63caad51cdc91acc880d911993038f30a76`).
The same two encounters also exercise genuine boss-side victories: X/P2
defeats entrant Zero/P1, then Zero/P1 defeats entrant X/P2. Positive defender
HP1 and positions/ammo are disclosed setup; actual eligible attacks must
produce the accepted fatal. No hit token, winner, death-script, text-complete
or success flag is seeded by the observer.

Each actual losing player finishes the original fatal/death animation and
reaches FALLEN before the native result script takes control. The result
latches once in canonical accepted-contact order, stopping later attacks and
combat HP mirroring. This gives simultaneous fatal opportunities a
deterministic first-accepted-hit rule; an actual simultaneous-input case is
still unverified.

The original retained NPC root handles native grounding, timers and result
UI; its body stays hidden. Native text43 selects Zero's original result;
45/47 selects X's from genuine CB80. Source message decoding confirms the
authored counterattack/injury dialogue belongs to these original victory
paths. Both actual winner/text paths pass with retries unchanged. X's result
reaches initialized mode20/stage11/checkpoint0E. Zero's check observes native
story-loader mode19/raw18/checkpoint0E; later chapter initialization is not
claimed. A current-build slot7 restore from the initialized X story back to
the saved interactive duel also passes. Both cases retain the earlier
revival, reciprocal nonlethal contact, menus/equipment and restore checks.

Winner story identity is separate from either player's body/menu/parts and
is included in digest/current-build snapshots. Corrupt winner/story pairs
are rejected together. The final raw12 character predicate uses this
independent identity without changing body or native starting-character
privileges; actual downstream ending selection remains unverified. New-game
or native card-load frontend clears the runtime story identity. Independent
native cold-save winner persistence is not implemented.

23/23 CTests pass. Original/private native cards are unchanged, both owned
result runners are stopped, and zero local-view leaks are recorded.
The first result candidate's premature P1 lifecycle marking was caught by
review and superseded: native body ticks now admit pending fatal HP and finish
death before the NPC result proceeds. The earlier reports are retained, not
treated as qualification of that defect.

Remaining gaps are complete NPC pose/alternate-palette and result effects,
Zero chapter initialization and remaining chapter/destination/roster/ending
coverage, native cold-save winner persistence, interrupted admission and
actual peer qualification. F62 remains unverified.

Current result receipt:
`G:/Temp/Codex/psxrecomp/x5-parity/pvp-duel-normal-controls-20261022/winner-qualification.json`.
Earlier entry/revival receipt:
`G:/Temp/Codex/psxrecomp/x5-parity/pvp-duel-normal-controls-20261022/qualification.json`.

Use shared encounters rather than a configuration matrix:

- One real inner-door admission per distinct entering-seat path: outgoing
  beam, exact opposing appearance, no duplicate body, correct health meter,
  and independent normal controls after the native introduction. Exercise
  actual Start-menu equipment/move selection as well as open/resume.
- In those encounters, exercise actual reciprocal attack contact, rejection
  during native recovery and an accepted final hit for each winning character.
  Cover an additional attack family only for a distinct implementation path.
- Follow each actual winner into its correct native story continuation and
  verify gameplay/roster recovery and a private native save/cold load. Cheats
  can supply positions, resources and short fights; do not seed the winner or
  success flags being checked.
- On one combined current candidate, verify a current-build mid-duel restore
  and one actual peer session through input ownership, damage and the result.
  Reuse the ordinary co-op regression bundle to catch leaked hostility or HUD
  suppression after the encounter.

Absent participants and interrupted admission need finite safe behavior.
Previously dead participants now have both entry-path checks; the accepted-hit
ordering rule is described above. Add a case where its code differs; do not create a character/seat/camera/network Cartesian matrix.
If a native setup remains difficult, preserve its evidence for human setup.

F62 remains **unverified** until the playable duel and winner-selected native
story continuation are demonstrated. These partial checks do not change that status.

Source audit artifacts: `G:/Temp/Codex/psxrecomp/x5-parity/design-pvp-duel-20261022`.
Normal-controls story-context proposal:
`G:/Temp/Codex/psxrecomp/x5-parity/pvp-duel-normal-controls-20261022/story/story-context-proposal.json`.

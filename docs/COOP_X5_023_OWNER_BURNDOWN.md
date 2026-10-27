# X5 v0.2.3 opening-stage owner reports

Local campaign on `feature/mmx5-x4-parity-local`; owning issue `beads-eio.10.35`.
The accepted door-animation experiment remains included. This list addresses
the new owner reports, not the parked original 234-comparison campaign or LAN.

Evidence root:
`G:/Temp/Codex/psxrecomp/x5-parity/owner-023-intro-20261027`.
Original archive: `E:/Downloads/Opening stage bugs 0.2.3 (1).zip`, SHA256
`1a8eacdbc95cc9522cf0ffebb67fe5155e71e76c1c34cd8522fc1bfa4b9adf1d`.
The linked video could not be fetched (HTTP403); do not describe it as reviewed.
The supplied crop demonstrates X remaining colored during the white flash.
Owner revised Black Zero availability on2026-10-27: keep the separate native
choice, but unlock it only after the Sigma stage3 capsule. This supersedes the
previous always-available choice and is tracked separately from these eight bugs.

| ID | Individual report / failure mode | Bounded check and evidence | Status |
|---|---|---|---|
| O01 | P2's bottom floor follows P1's camera. Shared class36 scenery stores canonical screen X; the previous correction depended on optional widescreen. | Owner configuration has widescreen **off**. `candidate-spatial-draw/spatial-check`: move P2 alone, then P1 alone; composed bottom strips and native cameras show each floor follows its own view. Private draw recalculates origin and native visibility without advancing controllers. | Passed on current candidate, bounded to intro city floor. |
| O02 | P2 Zero's Z-Buster muzzle appears at P1. Native class3 body effects follow singleton PLAYER. | Baseline `baseline-spatial/spatial-probe.json` records class3/subE at P1; candidate `candidate-spatial-draw/spatial-check/report.json` records actual Triangle-created effect following P2. Native class3 update now projects its recorded allocator. | Passed, native P2 effect origin/art bounded check; class3 owner projection is included in integrated195C2B01. |
| O03 | P2 Zero's Alia trigger returns P1 X with corrupt art; reverse heroes okay. | Actual P2-owned Alia snapshot retained in `original-intro-prefix/saves/openbios/...slot01.pst`; compare the whole return animation against the original forming reference, with widescreen off. | Passed by owner on2026-10-27; conversation receipt `owner-acceptance-20261027.json`. |
| O04 | P2 Zero wall-jump dust appears at P1 in debris building. | Actual building approach saved as original slot03. Same class3 update is a concrete candidate; require an actual wall-jump and dust observation. First attempt entered a legitimate Alia scene and did not qualify wall dust. | Passed by owner on2026-10-27; conversation receipt `owner-acceptance-20261027.json`. |
| O05 | X campaign → quit → Zero campaign in one process corrupts P1 Z-Buster. | Exact original prefix verifies9000 CRC/read-order inputs, including the actual transition; its saved native Zero context reproduces blue X tiles in the Z-Buster. Candidate `buster` captures show the intact orange projectile and repaired Z HUD. `same-process-restart/report.json` also passes actual Zero -> Quit -> X -> Quit -> Zero in one candidate process, with genuine confirmations and roster recovery. | Passed, bounded native restart/shot checks. |
| O06 | Both heroes stay colored during the statue reveal flash. | `candidate-integrated/native-reveal`: actual Both door, intact statue/explosion/reveal, four native Sigma pages and fight admission. Native RGB offsets reach31/992/31744; peak captures show complete white with no colored body/armor fragments in either pane. | Passed, actual native reveal. |
| O07 | X only partly turns white during Sigma defeat. | `candidate-integrated/native-defeat-final`: living boss reduced to positive1HP as disclosed setup, then actual buster input reaches0HP through native damage and runs the defeat flash. Peak31/992/31744 composed captures have no colored X body/armor fragments. | Passed, actual native defeat/flash. |
| O08 | Zero stays colored during Sigma defeat. | Same native defeat as O07; Zero's retained body fades through the scene's actual offsets and disappears into full white in both panes, rather than remaining red. | Passed, bounded native defeat/flash. |

Original release prefix session1793085765657559 verifies6500 native input reads,
including cooperative read order and CRC. It supplies actual separated intro,
P2-owned Alia and building setup snapshots; it is not a complete route replay.
Snapshots, teleports and contact protection are setup only. No claimed scene,
boss-defeat or progression success flags may be seeded.

Intermediate EXEs/profiles, owner archive extraction and captures live under
the evidence root. Keep evidence and setup snapshots for ongoing checks; clean
duplicate runtime payloads once the integrated candidate is qualified. The
sole delivered v0.2.3 ZIP remains unchanged until a replacement is qualified.

Current eight-report inventory: **8 passed / 0 unverified / 8 total**. O03/O04
received owner acceptance on2026-10-27 ("x5 passes") after preparing one actual
paused building approach bundling both reports on86D8F625. This is human
validation; the earlier bounded automation did not qualify those mechanics.
`owner-acceptance-20261027.json` preserves the conversation receipt separately
from the panel's unchanged autosaved notes. Keep failed setup reports and the
original forming reference. This acceptance does not cover native cold-card
persistence, LAN or the parked original comparative campaign.

Integrated graphics candidate SHA256:
`195c2b013822c7a2175c2ffab5be62604f1d097f773f73d8f53882915da146fb`.
It passes24/24 CTests and whitespace checks. Actual Both-door walk admission
still cycles both original animations (16/16 and17/16 observed cursor variants),
closes, and reaches the original boss scene. Native palette source evidence is
resident2BD8C..2C058 and2C218, plus BIN0 F8498..F84C4.2C218 initializes
palette row masks/offsets; it is not a collision-reset routine. Retained colors
now mirror the native saturating offsets, row masks, transparent zero and STP.
The first defeat attempt lost its unprotected partner while the observer was
being prepared; the successful check restores the genuine released-fight
snapshot and enables protection before any attack. No defeat outcome is seeded.


## Black Zero capsule requirement (2026-10-27)

Owning local issue: `beads-eio.10.36`. This is a separate owner design change,
not a ninth opening-stage bug. Co-op offers Normal Zero before the genuine
unlock and both Normal/Black afterward, through the existing native selector.
The Black Zero preference cannot bypass native progress. Choosing Normal after
unlock must not make an already collected capsule reappear.

The actual Sigma stage 3 secret capsule is raw18/0, BIN28, primary placement72
at (4696,1024), class1A/subtype8. Its initialized center is (4696,1044). Raw11
is a frontend location, not this stage. Native camera-area placements4/5 admit
the lower room (maxY608 -> 848); the bounded setup crosses their real area at
x4320..4447/y780..795 before positioning above the capsule. Native checkpoint
loading, body/camera positioning and placement rescans bypass route access only.
They do not set unlock, dialogue, contact, capsule phase or completion flags.

The original initializer54FE4 now considers an eligible living counterpart when
P1 is ineligible: Fourth Armor on X must not hide Zero/P2's capsule. Original
eligibility and the actual original grant55D24 remain authoritative. Special
capsules use original message55 at55960, not table entry60; recipient dialogue
uses that hero's message bank. P2's actual Black grant reinitializes only its
private palette, preserving X's shared body/armor rows. Original capsule return
recognizes the subtype8 native progress fields instead of the ordinary armor
program mask. X's native Ultimate eligibility/grant branches remain in place;
this campaign does not claim an additional Ultimate acquisition test.

Final feature candidate SHA256:
`86d8f625418790ee44f1a6b4b2bf53441faaa6f54bd36ce6f952e87761b468b4`.
It includes the six graphics fixes above and passes24/24 CTests. Evidence:

- `black-zero-complete/native-capsule-p2/report.json`: actual X/P1, Zero/P2
  capsule admission,15 original Zero dialogue pages, native top contact,
  original phase11 grant (BLACK bit0 -> 1, preserving X's other progress bits),
  Black art, and38px of actual input movement afterward. X palette rows0/13/14/15
  are byte-identical before/after; recipient action/control locks recover.
- `black-zero-complete/native-capsule-p1/report.json`: actual Zero/P1 grant on
  the same final executable,15 original dialogue pages, an actual jump/top
  contact, Black appearance and38px of normal input movement after release.
  Entry setup first waited too briefly for current camera bounds; those failed
  setup receipts are retained, with no production workaround or forced bounds.
- The earlier `black-zero-candidate/native-capsule-complete` also retains a
  genuine P1 Zero collection. The first P2 attempt exposed the
  initializer ownership bug; the intermediate P2 pass exposed X's message-bank
  selection. Keep these observations as evidence, not additional pass counts.
- `black-zero-complete/selector-and-collected/report.json`: real earned progress
  offers two native choices; Left selects Normal and Right selects Black. Actual
  P1/P2 confirmations enter a stage with the correct Zero/X seats. Normal Zero
  keeps the earned bit, and native secret-capsule rescan does not respawn it.
  Reloading the genuine unearned campaign restores a single Normal choice;
  repeated Right cannot move off cursor0 or set the flag. The first entry
  observer stopped at an authentic Alia message before co-op enrollment; the
  completed check confirms its real pages and observes the proper stage state.
  Earlier locked/unlocked receipts and both failed observer receipts remain.

Native cold memory-card Save/Load of this new collection remains unverified.
Source inspection confirms original Save1D0C4 stores PLAY4A in the unchanged
native slot58; no card format or progress field was replaced. This is source
support, not an executed persistence pass. Do not repeat the invalid direct-Save
setup documented in COOP_X5_ARMOR_SELECT.md; use a normal campaign save for the
remaining check. Disposable build snapshots establish setup only.


Temporary-data cleanup: `cleanup-runtime-duplicates.json` records removal of
superseded private EXE/DLL copies. Keep the current feature and committed
six-fix graphics runtime, reports, images, logs, native setups and cards for
ongoing QA. The original delivered release remains unchanged. Scratch directories
under this evidence root are agent-owned and can be removed after this campaign.


The owner retest station is
`G:/Temp/Codex/psxrecomp/x5-parity/current-owner-candidate/Validation.ps1`.
Its one building card starts at a genuine native pause, with X/P1 far behind
Zero/P2, normal window/audio and the owner's DualSense assigned to P2. Cold
snapshot load, both living heroes, successful enrollment and a visible window
were verified. The card describes actual wall-kicks and the next P2-owned Alia
return, with separate O03/O04 notes and screenshots. Prior notes/runs remain in
place; previous panel metadata is under `history/pre-377c2e1-20261027`.

## v0.2.4 local release qualification

Owner requested the next Nextcloud build after acceptance. Release issue:
`beads-eio.10.38`. Candidate `v0.2.4-coop-alpha-debug` changes version metadata
and portable packaging after accepted mechanic candidate86D8F625. Runtime
source is ffa7cb1 on SDK59cae0a9; executable SHA256:
`309dced11919e51abfe9f37a6589ee8f1dddf7afcf91fa9d5b506eff7c792601`.

All24 CTests pass. The intro, Grizzly and pre-duel QA snapshots were regenerated
from actual native new games on this executable, then cold-loaded individually
and resumed through actual world ticks without failure. These snapshots bypass
route access and do not qualify campaign progression or memory-card persistence.
All four portable launcher VerifyOnly checks pass, including blank first-run
disc selection and the updated exact-executable guards. Imports are Windows
system DLLs only. The stage has11 packages; diagnostics/hint suppression default
on, with local split views, Either doors, keyboard/P1 and DualSense/P2 selected.
CD Speed and Fast Loading packages/controls are absent.

Qualification, archive verification and delivery receipts are retained under
`G:/Temp/Codex/psxrecomp/x5-parity/release-024-20261027`. The requested delivery
target is `G:/Nextcloud/Games/PSX`, with one verified X5 ZIP and no staging
directories or checksum sidecars there. Native cold-card Black Zero persistence
and previously parked campaign/LAN limitations remain disclosed in the package.

Delivered on2026-10-27 as
`MegaManX5Recomp-v0.2.4-coop-alpha-debug-windows-x64.zip`, ZIP SHA256
`53fcfb698bb391484834d719ef3d3cc5ee946a7cb25b5ed7d2f790aa540ca261`.
The copied archive passed integrity, executable/version and checksum checks
before removing the exact hash-qualified v0.2.3 archive. It is the sole X5 ZIP
in the PSX delivery folder. No owner saves/settings or other games changed.
Disposable test-state/cache directories were cleaned; qualification logs,
captures, exact-build QA fixtures and delivery receipts remain outside delivery.

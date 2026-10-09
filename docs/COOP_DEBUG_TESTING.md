# X5 co-op alpha debug testing

## Resumed release assessment (2026-10-09)

The owner's final validation in session
`01a11f09-b4bd-79f2-a768-45d31a2d06f5` accepts the Grizzly result:
the game returned to stage selection, C-Slash persisted, and Grizzly appeared
defeated after entering another stage, dying and returning. The owner explicitly
waived repeating this boss check. The initial briefing/portrait observation was
confounded by the synthetic stage-access fixture; it is not an outstanding
confirmed co-op progression defect. Automated return reports remain failed or
pending records; this human acceptance supplements them.

The owner also accepted the improved independent-display performance. Existing
evidence covers checkpoint survivor/team retry, P1-only Armor mount/dismount,
matching peer state, jitter/restoration checks, two controller pages and lobby
UI. The last recorded ordinary cheat-OFF build passed all eight CTests.

The owner subsequently authorized skipping the car replay and cutting an alpha
if prior coverage exists. It does: `qa-chaser-pose-fixed-x/sequence/report.json`
and `qa-independent-offline-zero/sequence/report.json` both pass the two first-
death orders, survivor movement/jump and shared retries 2 -> 1 and 1 -> 0.
The X run also checks the 180-update Select lock. Those controlled offline
fixtures qualify the car rules; they do not claim complete road traversal.
Release work is proceeding under that authorization.

The recovery checklist below records the state before release preparation:

1. Retire the generated stage-access card from active test/staging defaults.
   Future save fixtures must originate from an online download. Preserve the
   downloaded original and provenance; modify an existing save only sparingly.
   Active targeted/staging tools now require an explicit downloaded card and
   source URL, reject the retired synthetic card, and preserve the original.
   The historical generator refuses execution. Historical reports/cards remain
   evidence. Unused cards are initialized by the native runtime, not fabricated
   by test scripts.
2. Finish one bounded car check on the final candidate: a grounded surviving
   rider can jump after the other dies, Select cannot voluntarily despawn either
   rider, and the last death spends one shared retry. Earlier controlled car
   checks passed both death orders; the latest cheat-enabled paired run stopped
   on a jump assertion made while already airborne at a road gap. That failure
   establishes neither a gameplay regression nor a final-candidate pass. The
   owner now accepts the prior passing coverage and waived this replay.
3. Choose and build the actual package. `-CoopDebugTest` is the implemented
   debug-alpha route, with cheats separately opt-in through `-TestCheats`.
   An ordinary debug-tools-OFF release currently omits the co-op catalog; a
   non-debug co-op alpha needs an independent catalog inclusion option.
4. Smoke-test a fresh extraction of the final ZIP: co-op off, Unified and Split,
   both controller pages and lobby UI, one native save/load, and a brief audible
   separated-player session. A retained synthetic-fixture run stalled during
   memory-card handling; its cause remains unestablished. No exhaustive stage
   matrix or repeat of the accepted Grizzly check is required.
5. Commit the pending debug controls, targeted harness, packaging and handoff
   changes; record exact source/build identities and update release notes with
   the opt-in behavior and coverage limits. The branch is eight commits beyond
   the locally recorded `origin/master`, with six tracked edits and two new
   files at this assessment. No alpha ZIP, tag or publication was made.

Real two-machine/Internet netplay is explicitly deferred to user testing.
Full campaign, late-game scripts and broad stage/actor coverage are alpha
limitations, rather than newly imposed prerequisites. The optional playable
X-versus-Zero story encounter remains later work.

The co-op branch offers two controller seats in recomp-ui and the normal
LAN/Direct IP and Online Netplay lobby pages. Enable **X + Zero Co-op** under
Mods. For local play, bind separate devices to Player 1 and Player 2 on the
dashboard. For netplay, each peer binds its local device as Player 1 and the
room assigns the logical seat. The host selects Unified or Split in the mod's
Netplay cameras option. Both peers must use the same build and original USA
SLUS-01334 disc. Unified remains the default, and offline play uses Unified.

Private staging preserves the built launcher fonts/artwork, co-op catalog,
OpenBIOS, two-seat game configuration and lobby version stamp. It validates
debug, recomp-ui and netplay build flags with `--debug-test-build`. Downloaded
save cards can be copied into a new private playtest directory; previous cards
and playtest directories are preserved. The synthetic stage-access card used
in earlier evidence is retired from future use.

Build a private qualification candidate from this worktree:

```powershell
& 'C:/Program Files/CMake/bin/cmake.exe' -S . -B build-coop -DPSX_DEBUG_TOOLS=ON -DPSX_RECOMP_UI=ON -DPSX_NETPLAY=ON -DPSX_SDL_BACKEND=SDL2 -DMMX5_COOP_TEST_CHEATS=ON
& 'C:/Program Files/CMake/bin/cmake.exe' --build build-coop --target psx-runtime -j 2
py -3 tools/test_mmx5_coop_targeted.py --exe build-coop/mmx5-runtime.exe --out build-coop/qa-targeted-new --save-card 'C:/path/to/downloaded-mmx5-card.mcd' --save-source-url 'https://gamefaqs.gamespot.com/ps/258931-mega-man-x5/saves'
py -3 tools/prepare_mmx5_coop_playtest.py --build build-coop --out build-coop/playtest-debug-new --debug-test-build --save-card 'C:/path/to/downloaded-mmx5-card.mcd' --save-source-url 'https://gamefaqs.gamespot.com/ps/258931-mega-man-x5/saves'
```

The card argument above is a placeholder for a downloaded save. Record its
actual download URL and preserve that downloaded original unchanged. A completed
campaign save may not suit an unbeaten-boss test; choose the appropriate native
slot rather than manufacturing progression.

The targeted harness owns its two hidden loopback peers and copied cards. It
uses synchronized controller commands for mutations and debug TCP for reads.
It checks one Grizzly encounter/victory/return, one checkpoint/team retry,
one P1 Armor mount/dismount with P2 denial, and one car death order plus the
sequence's Select lock. It compares peer core/mod digests and enables local
render-restoration checks. `--only boss` and `--only vehicles` isolate those cases.
`--human-return` leaves a visible pair after the boss rewards for the owner;
its report marks the return pending rather than claiming a full pass.
This is debug-assisted scenario coverage, not full stage traversal, a complete
campaign, audible playtesting or real Internet/lobby transport validation.
The owner performs real netplay testing.

To produce a local release ZIP with the same launcher/controller/lobby flow:

```powershell
./tools/package_release.ps1 -Version 0.0.1 -CoopDebugTest
```

This uses a separate debug build/stage directory, SDL2, explicit recomp-ui and
netplay flags, and the development co-op catalog. The packaged game declares
two players. The version gains `-coop-alpha-debug` for lobby matching.
Add `-TestCheats` to include qualification commands and a `-cheats` version
suffix. The ordinary build defaults `MMX5_COOP_TEST_CHEATS` to OFF; this option
requires debug tools. Its value is part of the trusted compatibility contract,
so a cheat candidate cannot negotiate with a normal candidate. The packager
builds a local ZIP; publishing is a separate step.

## Qualification controls

In a cheat build, logical P1 holds **L3 + R3**, adds the command's shoulder
buttons, then releases the chord. With default keyboard bindings the arming
keys are **T + Y**. Commands execute once at a canonical world boundary, using
the replicated P1 SIO packet, and are excluded from local render replay.
They work during active stage simulation. Stage-entry commands also work from
the loaded stage selector and Game Over, through the synchronized VBlank
boundary. Unsupported stage/actor commands return a rejected receipt.

| Command | Shoulder buttons | Extra keyboard keys | Action |
| --- | --- | --- | --- |
| 1 | L2 | E | Original Grizzly checkpoint 1 reload |
| 2 | R2 | R | Original Grizzly checkpoint 4 reload near the boss approach |
| 3 | L2 + R2 | E + R | Native fatal transition on an existing Grizzly actor |
| 4 | L1 | Q | Refill living bodies/cars each world update |
| 5 | L2 + L1 | E + Q | Native P1 fatal HP/car death |
| 6 | R2 + L1 | R + Q | Native P2 fatal HP/car death |
| 7 | L2 + R2 + L1 | E + R + Q | Native stage 5/section 0 entry for Ride Chasers |
| 8 | R1 | W | Native stage 4/section 1 entry for Ride Armor |
| 9 | L2 + R1 | E + W | Place P1 beside existing native Armor |
| 10 | R2 + R1 | R + W | Place P2 beside existing native Armor |
| 11 | L2 + R2 + R1 | E + R + W | Move living P2 beside living P1 |
| 12 | L1 + R1 | Q + W | Native Grizzly stage entry |
| 13 | L2 + L1 + R1 | E + Q + W | Set three shared retries for a controlled death case |

The fatal commands disable refill for the victim before applying death.
Refill preserves original contact immunity and mount/door behavior. Boss death,
rewards and campaign return remain native; the commands do not set victory or
defeated-boss flags. Checkpoint commands invoke the original continuation
controller/loader; mission commands invoke original title/loader modes.
The older arbitrary RAM fixture mailbox remains offline-only.

The diagnostic receipt at the co-op diagnostic address +0x80/+0x84/+0x88 contains
the command count, last command and result (zero means rejected). Cheat edge,
refill-mask and command-count state participate in the mod digest.

## Targeted findings (2026-10-09)

The retained private paired Split reports are under `build-coop/`. The historical
stage-access card is preserved but retired. These checks cover selected native
paths and use debug commands to shorten traversal.

- Grizzly entry, native boss gate/dialogue/health fill, fatal/death controller,
  original defeated bit and both character reward sequences were observed.
  `qa-targeted-final4` compares peer core/mod samples through the fight.
  The automated post-boss return driver did not pass: incorrect save-menu
  choices reopened Save; one retained run also hit memory-card IRQ starvation.
  The latter has no established root cause and does not establish a netplay bug.
  Subsequent owner validation accepted return and durable rewards/defeat state,
  as recorded above, and explicitly waived a repeat.
- `qa-targeted-final4` validates one CP1 single-death survivor, then a team wipe
  consuming one shared retry and reloading the original checkpoint.
- `qa-targeted-vehicles-direct` validates P2 Armor denial, P1 native mount and
  dismount, with matching samples through2944 and zero restoration faults.
  Its car Select window stayed out of voluntary departure states and exercised
  the departure-lock counter. Native road-gap deaths consumed one retry.
  A later survivor jump assertion ran after the car reached the gap at X1509,
  airborne, so it does not qualify jump control or the final team reload.
- Launcher captures in `qa-launcher-debug-assets` show two dashboard/controller
  seats and LAN/Online lobby pages with the staged fonts and artwork.
  No public lobby or Internet session was created.
- All8 existing CTests pass on the ordinary cheat-OFF build. The compiled normal
  binary excludes the cheat-enabled marker, and its trusted compatibility ID
  differs from the separately staged cheat-enabled candidate.

`playtest-alpha-cheats` is a private debug-assisted candidate with the eight-stage
save, two-seat recomp-ui assets and version stamp. `build-coop/mmx5-runtime.exe`
is restored to cheats OFF. ZIP packaging and publication have not been executed.
Boss return and prior car-rule coverage are accepted by the owner. Final package
qualification remains; real netplay is deferred to user testing. The private
reports do not establish full campaign qualification.

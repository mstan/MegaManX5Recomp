# X5 private character audio investigation

P21 now has an X5 adapter with bounded native/runtime evidence, including the
deferred-buster ownership retest. Full audio parity remains unverified. Work
resumed after owner-requested local checkpoint `3d5abaa` on 2026-10-19.
Reference: X4 `cf6ce95`, `src/mods/mmx4_coop_audio.c` and its audio evidence.
Original input: SLUS-01334 EXE SHA256
`f61827043c9a4f4100154cf0489a11a1cd1aa056284551fd8b1c24a613a6769b`.
Disassemblies and extracted original archives stay under ignored build folders.
No derived audio assets are committed or shipped.

## Verified native entry points

| Role | Original X5 evidence | Limits |
| --- | --- | --- |
| Character sound routing | `80016490`; group uses `a0 & 7F`; end/record arrays `8009A730` / `8009A770` | `800164A8` is an interior instruction, not an entry point. |
| Spatial direct sound | `8001657C`; actor supplied in `a2`; priority array `800E8028` | Need native ownership/call-site coverage for both seats. |
| SDK voice/tone key-on | `8005D458`; direct driver calls at `80016768` / `800167C0` | More stack arguments than the first four registers; preserve original call stack. |
| Voice status | `80059988`, called with `1 << voice` at `8001673C` | Private status/key-off routing has boundary coverage; broader runtime qualification remains open. |
| VAB parser | `800626F8`; sticky wrappers `80062688` and `800626BC` pass original `a2` to parser `a3` and callback `800626F0` to `a2` | Callback returns its sample-base argument. Do not register `80062694`, an interior candidate, as a function. |
| VAB status and transfer gate | Status array `800EA7A0`, parser stores2 at `80062AB8`; `8005FCC8(0)` releases the original gate at `8007DCF4` | Original body transfer normally makes status1. Private residency must reproduce readiness without issuing a native DMA. |
| Tone metadata | `800E9F50 + voice * 38h`; arithmetic at `8005D674..8005D748` | X4 uses a34h record. Blindly copying X4's save/restore extent would miss state. |
| Deferred register/dirty writes | `8005E4D8` writes register-relative `800EA4B6` (register base `800EA4B0`), dirty bytes `800EA490`, and16-word mask array `800EA638` | Retained work/history/key-mask extents and pending-key coverage are described below. |

## Original resource layout

`ROCK_X5.DAT` record88 is X, record90 is Zero. The existing checked DAT parser
resolves their sector extents and padded members. Character-specific pairs:

| Members | Header type | Body type | X header/body bytes | Zero header/body bytes |
| --- | --- | --- | --- | --- |
| 10/11 | 6 | `20101h` | 4760 / 172192 | 3708 / 108288 |
| 12/13 | 6 | `20303h` | 3164 / 35280 | 3164 / 40368 |

The first header's `pBAV` offset is120 for X and92 for Zero; the second pair's
offset is60. Music members4/5 are shared and excluded from private character
banks. The second body type differs from X4's `20301h` and must be checked as
X5 data. Members14/15 are not yet identified.

X5 currently frees the DAT buffer after copying character assets. Private
audio preparation must own or retain its header/encoded ADPCM bytes before
that free; borrowed views into the discarded archive would be invalid.

## Native calculation and adapter

`tools/qualify_mmx5_audio_native.py` runs the unmodified original VAB parser
and key-on routines in an isolated MIPS CPU. `qa-audio-native/sdk-caller-note.json` passes
all85 declared character tones: X groups1/3 contain34/13, Zero24/14. No tone
uses VAG255/noise, and all calculations complete without MMIO writes. RAM
snapshot differences stay within the documented SDK data regions. Snapshot
differences are not a complete store trace; original instruction inspection
establishes the retained field extents. Unicorn's matching RAM-write hook
misexecutes the original jump-delay store in5FCC8, so RAM runs uninstrumented
and only the MMIO range has a write hook. No game instruction is replaced.
The calculation uses the original game's note argument (tone attribute6 with
fine attribute5, via16734/38 and5D350); the earlier center-note4 probe is retained
but is not the caller qualification.

The adapter owns copies of all four headers and encoded sample banks before
the DAT is freed. Handles12/13 are X,14/15 Zero; SPU residency binds once per
boot epoch. The original parser installs sample-base-zero metadata and the
adapter reproduces ready status/transfer completion after host residency.
P2 group1/3 routing uses scoped record/end/priority swaps. Original key-on
calculates the native pitch, stereo gain and ADSR; P2 plays on independent
private slots, with canonical register/meta/dirty/history/key-mask state
restored before the shared driver can flush it. P1 keeps native hardware
voices. Private stop/status/volume callbacks follow original16D54,59988,
5D7C4 and5E448; global silence still affects both sets.

X5's retained regions are register mirror800EA4B0+voice*10h, metadata
800E9F50+voice*38h, dirty byte800EA490+voice, work block800EA760..800EA7A0,
16 history words plus pending-off halves800EA638..800EA67C and key/reverb
masks800E9F40..800E9F4C. These differ from X4's extents. The boundary test
covers both characters/groups, freeing the input DAT, once-per-epoch binding,
pointer/priority restoration, pending native voice state, metadata reopening,
private stop and global silence. Its SDK calls are models; the separate
original-code test qualifies real metadata calculations.

## Bounded runtime evidence and stopping point

CandidateFC6F8C15 passes eight cases in `qa-private-audio-native-idle-x`:
individual and simultaneous jumps, individual and simultaneous attacks, and
jumps after each seat's owned menu. Jump checks require actual native liftoff.
P2/Zero's jump sample address, pitch and ADSR match the original SDK/record;
native and private matching-slot playback overlap. Muted PCM captures are
nonzero, and adapter error counters stay clear. This is machine-observed
playback evidence, without an ear review or complete DSP comparison.

The reverse roster in `qa-private-audio-native-idle-zero` passes the first
three jump cases and P1's attack, then fails P2/X's buster sound. The retained
trace `qa-p2-buster-sound-context-zero` finds SOUND group1/index8 from8004F350
on native voice23 with the campaign Zero bank. The original type2 shared effect
at8004F3F4 defers the sound until after P2's projected update has ended.
`qa-p2-buster-effect-context-zero` identifies the effect in the96-slot70h pool
at80094398..80096D98 and its original init/update path.

The latest source records allocation-seat ownership for that pool, clears it
on reuse, and projects the owned type2 buster update onto its allocating hero.
Candidate7C3E87D3 passes all eight reverse-roster cases in
`qa-buster-owner-exact-tone-zero`: P2/X's jump record1 and buster record8 match
the original X sample address, pitch and ADSR on private slots46/45, with
simultaneous overlap and both owned menu returns. The retained original SOUND
trace in `qa-buster-owner-sound-trace-zero` observes4F350 and private voice45;
the older voice23/campaign-bank failure remains retained as pre-fix evidence.
All17 debug and production CTests pass with this fix.
CandidateAA1105D6 also passes all eight cases in each roster after delayed
emitter cleanup (`qa-20261020-audio-cleanup-native-zero`,
`qa-20261020-audio-cleanup-native-x`). These explicitly use
resident archive loading disabled; it does not qualify the default-loading
watchdog issue. Private X/P2 jump/buster tone identity, overlap and both owned
menu returns remain intact on that candidate. Zero/P2's original jump-tone
identity and simultaneous native/private playback pass in the reverse pairing.
Other charged/shared effects, music, hurt/death, stage reload and sustained
playback remain unverified. Runtime PCM is retained with speaker output muted.

## Required qualification

Retain original encoded samples and native pitch/gain/ADSR calculation. Prove
the VAB transfer gate, ready status, voice register/meta/queue/key-mask extents
and original pending-key preservation before enabling private channels. Bind
derived sample residency to SPU boot epochs. Exercise both X/Zero seat orders,
simultaneous jump/hurt/attack voices, native music and shared effects, death,
pause, stage reload and sustained playback. Capture emulated audio while output
is muted for automated checks. Compilation and identifying SDK functions do
not establish audible parity.


## Native hurt and death observation (2026-10-20)

Candidate856484F5 adds one actual Zero/P2 native intro-enemy hurt and its native
death audio. Exact source/SDK/DAT identities and receipts:
`G:/Temp/Codex/psxrecomp/x5-parity/qa-save-effects-20261020/native-hurt-death-approach/qualification.json`.
Native38D84 chooses group3/record0 or1; actual record0 uses private voice44,
pitch0400/sample address0/ADSR80FF,1FC0, exactly matching the original Zero
bank15 tone. Address0 is this original sample's valid base-zero address.
P2 HP32->27 and accepted contact are observed. Its voice overlaps native P1
jump voice22 for11 frames and continues until natural END_STOP despite a
subsequent P1 hurt voice. P1 jump slot/pitch/ADSR match the original record;
its resident hardware sample-base address is not separately qualified here.
P1 has HP protection only and can still enter a native hurt reaction.

Native death3D974 requests group3/record0C. Both original Zero stereo tones
match private voices44/45, pitches0390/03C7, address07640 and ADSR80FF/1FC0.
Fatal HP is explicit setup after actual hurt; no fatal-contact pass is claimed.
The retained muted PCM is nonzero stereo44.1kHz,69090 frames. Full DSP/ear
review, other hurt/death character paths, music and sustained playback remain
unverified. No audio implementation changes were needed for these observations.

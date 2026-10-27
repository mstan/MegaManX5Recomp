"""P1 input-only native rewards, real private-card Save and selector return.

Start after an actual Maverick defeat. Never creates a card, seeds an award,
changes UI/progression RAM or treats the mission-report load as a return pass.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time
import traceback

import psutil

from coop_debug import Runtime, PLAY
from test_mmx5_coop_grizzly_clear_runtime import require


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--expected-clears', type=lambda x: int(x, 0), required=True)
    parser.add_argument('--continue-from', type=Path,
                        help='Continue this owned native flow after a retained report already observed real Save completion')
    args = parser.parse_args()
    parent = json.loads(args.session.read_text(encoding='utf-8'))
    exe = Path(parent['command'][0]).resolve()
    require(Path(psutil.Process(parent['pid']).exe()).resolve() == exe, 'Not owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest() == parent['executable_sha256'], 'Binary changed')
    r = Runtime(parent['port'])
    require(not r.request('netplay_status')['active'], 'Private offline native-card check only')
    command = parent['command']
    card_dir = Path(command[command.index('--memcard-dir')+1])
    cards = [card_dir/f'card{i}.mcd' for i in (1, 2)]
    require(all(p.is_file() for p in cards), 'Use the existing owned private cards')
    args.out.mkdir(parents=True, exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    report = dict(success=False, human_validation=False, executable_sha256=parent['executable_sha256'],
                  session=str(args.session.resolve()),
                  expected_clears=args.expected_clears, steps=[], input=[], text_pages=[],
                  card_sha_before=[hashlib.sha256(p.read_bytes()).hexdigest() for p in cards],
                  limits='Actual native reward/save/selector only; no cold reload, all-boss portrait or campaign-ending pass.')

    def save():
        (args.out/'report.json').write_bytes((json.dumps(report, indent=2)+'\n').encode('utf-8'))

    def frames(n):
        start = r.request('frame')['frame']
        deadline = time.monotonic()+20
        while r.request('frame')['frame'] < start+n:
            require(time.monotonic() < deadline, 'Native frontend stopped advancing')
            time.sleep(.02)

    def pulse(button):
        report['input'].append(dict(seat=0, button=hex(button), before=r.read(PLAY, 8).hex()))
        r.input(button, 0)
        frames(6)
        r.input(0, 0)
        frames(8)

    try:
        r.release()
        before = r.read(PLAY, 0x1C0)
        report['before_play'] = before.hex()
        if args.continue_from:
            previous = json.loads(args.continue_from.read_text(encoding='utf-8'))
            require(previous['executable_sha256'] == parent['executable_sha256'] and
                    previous['expected_clears'] == args.expected_clears and
                    'native_save_completed' in previous,
                    'Continuation requires the actual prior native Save on this candidate')
            require(before[0] in (3, 4, 20) or (before[0] == 18 and before[1] in (7, 9)),
                    'Continuation must already be in native Save-complete/return frontend')
            report['continued_from'] = dict(path=str(args.continue_from.resolve()),
                sha256=hashlib.sha256(args.continue_from.read_bytes()).hexdigest())
            report['card_sha_before'] = previous['card_sha_before']
            report['native_save_completed'] = previous['native_save_completed']
        else:
            require(before[0] == 12 and before[1], 'Need an actual initialized mission report')
        require(before[0x4C] == args.expected_clears, 'Unexpected earned clears')
        campaign = before[0x44]
        deadline = time.monotonic()+180
        signature = None
        save_confirmed = bool(args.continue_from)
        overwrite_confirmed = False
        while True:
            p = r.read(PLAY, 0x1C0)
            require(p[0x44] == campaign and p[0x4C] == args.expected_clears,
                    'Native frontend changed campaign or earned clears')
            current = p[:4].hex()
            if current != signature:
                label = f'step-{len(report["steps"]):02d}-{current}'
                report['steps'].append(dict(label=label, play=p.hex(), text=r.read(0x80091C44, 2).hex()))
                print('Native reward/return', current, flush=True)
                r.screenshot(args.out/f'{label}.png')
                signature = current
                save()
            if (p[:3] == b'\x04\x04\x00' and p[0x0C:0x0E] == b'\x0D\x00' and
                    not r.read(0x8009A424, 1)[0] and not r.read(0x80091C44, 1)[0]):
                report['selector_play'] = p.hex()
                r.screenshot(args.out/'native-selector.png')
                break
            require(time.monotonic() < deadline, 'Native reward/save/selector deadline expired')
            if p[0] == 12:
                pulse(0x4000)
            elif p[0] == 14:
                if p[1] == 1 and not r.read(0x8009A424, 1)[0]:
                    frames(15)
                    pulse(0x4000)  # Original upper Save row.
                else:
                    time.sleep(.06)
            elif p[0] == 18:
                if p[1] == 3:
                    pulse(0x4000)  # Native first slot.
                elif p[1] == 4 and not save_confirmed:
                    pulse(0x80)    # Original save confirmation defaults right/No.
                    pulse(0x4000)
                    save_confirmed = True
                elif p[1] == 5 and not overwrite_confirmed:
                    # An occupied slot has its own native overwrite prompt.
                    pulse(0x80)
                    pulse(0x4000)
                    overwrite_confirmed = True
                    report['overwrite_confirmed'] = True
                elif p[1] == 7:
                    report['native_save_completed'] = p.hex()
                    pulse(0x4000)
                elif p[1] == 9:
                    pulse(0x4000)  # Native Continue after completed write.
                else:
                    time.sleep(.06)  # Original card I/O owns its completion.
            elif p[0] == 20:
                if r.read(0x80091C44, 2) == b'\xFF\x03':
                    report['text_pages'].append(r.confirm_native_text(timeout=4))
                else:
                    time.sleep(.06)
            elif p[0] == 4 and p[1] != 4 and r.read(0x80091C44, 2) == b'\xFF\x03':
                report['text_pages'].append(r.confirm_native_text(timeout=4))
            elif p[0] in (3, 4):
                # Native mode3 constructs the selector before mode4 fades in.
                time.sleep(.06)
            else:
                raise AssertionError(f'Unexpected native frontend {current}')

        report['card_sha_after'] = [hashlib.sha256(p.read_bytes()).hexdigest() for p in cards]
        require(save_confirmed and 'native_save_completed' in report, 'Native Save completion not observed')
        require(report['card_sha_after'][0] != report['card_sha_before'][0], 'Native Save did not change card1')
        require(report['card_sha_after'][1] == report['card_sha_before'][1], 'Unselected card2 changed')
        slot = cards[0].read_bytes()[0x2200:0x2358]
        checksum = struct.unpack_from('<H', slot, 0x154)[0]
        require(checksum == (sum(slot[:0x154])+0x55) & 0xFFFF, 'Actual saved slot checksum mismatch')
        require(slot[0x4A] == args.expected_clears, 'Native saved slot lost earned clears')
        report['native_saved_slot'] = dict(bytes=slot.hex(), defeated_mask=slot[0x4A], checksum=checksum,
                                           phase=slot[1], path=str(cards[0]))
        (args.out/'after-native-save-card1.mcd').write_bytes(cards[0].read_bytes())
        report['success'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        print(report['error'], flush=True)
    finally:
        r.release()
        report['after_play'] = r.read(PLAY, 0x1C0).hex()
        save()
    print(json.dumps({k: report.get(k) for k in ('success', 'error')}), flush=True)
    return 0 if report['success'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

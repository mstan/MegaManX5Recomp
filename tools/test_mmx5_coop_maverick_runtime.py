"""One native on-foot Maverick entry/final-hit check in an owned QA session.

Original checkpoints bypass routes. Controller input must open doors, admit
the boss, finish its positive HP and reach the original report. Vehicle combat
has a separate contract. A configured checkpoint is never a gameplay pass.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import subprocess
import sys
import time
import traceback

import psutil

from coop_debug import Runtime, PLAY
from audit_mmx5_coop_locations import counter
from test_mmx5_coop_grizzly_clear_runtime import MAVERICKS, require

# SLUS_013.34 checkpoint tables and ROCK_X5.BIN boss placements. Stage7/CP7
# is an earlier alternate route; CP6 is its actual near-boss approach.
CHECKPOINTS = {1: 4, 2: 2, 3: 6, 4: 1, 6: 4, 7: 6, 8: 4}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--stage', type=int, choices=tuple(CHECKPOINTS), required=True)
    parser.add_argument('--entry-only', action='store_true',
                        help='Park after actual native admission/full HP; do not lower boss HP or run the finisher')
    parser.add_argument('--post-intro-prerequisite', action='store_true',
                        help='Disclosed phase1/old-story-queue bypass; never credits intro completion')
    args = parser.parse_args()
    parent = json.loads(args.session.read_text(encoding='utf-8'))
    exe = Path(parent['command'][0]).resolve()
    require(Path(psutil.Process(parent['pid']).exe()).resolve() == exe, 'Not the owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest() == parent['executable_sha256'], 'Binary changed')
    r = Runtime(parent['port'])
    require(not r.request('netplay_status')['active'], 'Native checkpoint setup is offline only')
    args.out.mkdir(parents=True, exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    section, boss_class = MAVERICKS[args.stage]
    report = dict(success=False, human_validation=False, stage=args.stage, section=section,
                  checkpoint=CHECKPOINTS[args.stage], boss_class=hex(boss_class),
                  executable_sha256=parent['executable_sha256'], session=str(args.session.resolve()),
                  samples=[], writes=[], input=[], entry_status='unverified', clear_status='unverified',
                  bypassed_prerequisites=['Original checkpoint loader bypasses route traversal',
                                         'HP-only refill; original contact and hurt recovery remain enabled'],
                  limits='This native entry/final-hit/report only. No full fight, route, later rewards/save/selector or netplay pass.')

    def save():
        (args.out/'report.json').write_bytes((json.dumps(report, indent=2)+'\n').encode('utf-8'))

    def sample(label):
        s = r.personal_effects()
        p = r.read(PLAY, 0x1C0)
        pool = r.read(0x80092090, 48*0x9C)
        bosses = [(0x80092090+i, pool[i:i+0x9C]) for i in range(0, len(pool), 0x9C)
                  if pool[i] and pool[i+1] == boss_class and not pool[i+2]]
        ptr, boss = bosses[0] if bosses else (0, b'')
        row = dict(label=label, world=s['state']['world_ticks'], play=p.hex(), life=s['life'],
                   scene=s['scene'], bodies=[b.hex() for b in s['bodies']], boss_address=hex(ptr),
                   boss=boss.hex(), door=r.read(r.diagnostic()+0xA4, 16).hex())
        report['samples'].append(row)
        return row, p, s, boss

    try:
        r.release()
        r.resume_native_pause()
        r.fixture(11, hp=3)
        r.fixture(12, hp=0)
        report['before'] = sample('before')[0]
        before_play = bytes.fromhex(report['before']['play'])
        require(not before_play[0x4C] & (1 << (args.stage-1)), 'Requested boss is already defeated')
        if args.post_intro_prerequisite:
            require(before_play[0x79] in (0, 1), 'Prerequisite bypass requires an early campaign')
            report['bypassed_prerequisites'].append('Post-intro phase1 and empty old-story queue; intro not tested')
            for address, value in [(PLAY+0x79, 1)]+[(PLAY+0x194+i, 255) for i in range(16)]:
                old = r.read(address, 1)[0]
                r.request('write_ram', addr=hex(address), val=hex(value))
                report['writes'].append(dict(address=hex(address), before=old, value=value,
                                            purpose='chapter/old-story prerequisite only'))
        previous = counter(r, 'mmx5.coop.enrolled')
        r.load_location(args.stage, section, CHECKPOINTS[args.stage])
        deadline = time.monotonic()+100
        moving = False
        signature = None
        while True:
            row, p, s, boss = sample('native-entry')
            current = (p[:2].hex(), row['door'][:4], tuple(s['scene']), tuple(s['life'][:2]),
                       boss[:7].hex(), boss[0x5C] if boss else None)
            if current != signature:
                print('Native entry', current, flush=True)
                signature = current
                save()
            if p[0] == 10 and counter(r, 'mmx5.coop.enrolled') > previous:
                require(not any(x in (4, 5) for x in s['life'][:2]), 'Native entry lost a hero')
                unlocked = (s['life'][:2] == [0, 0] and not p[1] and not p[0x1C] and
                            not s['scene'][0] and all(b[4] == 1 and not b[0xD0] for b in s['bodies']))
                if (boss and boss[4] == 1 and 0 < boss[0x5C] < 128 and
                        boss[0x5C] == p[0xA2] and unlocked):
                    r.release()
                    report['fight_ready'] = sample('fight-ready')[0]
                    report['entry_status'] = 'passed bounded native admission/HP fill/control return'
                    break
                if s['scene'][0] or (boss and boss[4]):
                    if moving:
                        r.release()
                        moving = False
                    if p[0x1C]:
                        seat = max(0, s['scene'][0]-1)
                        report['input'].append(dict(seat=seat, button='Cross', purpose='original scene text'))
                        r.tap(0x4000, .10, seat)
                elif unlocked and not moving and all(b[4:6] == b'\x01\x02' for b in s['bodies']):
                    r.input(0x20, 0)
                    r.input(0x20, 1)
                    moving = True
                    report['input'].append(dict(seats=[0, 1], button='Right', purpose='native door approach/crossing'))
            require(time.monotonic() < deadline, 'Bounded native boss admission did not finish')
            time.sleep(.10)

        bodies = r.players()
        finisher = next(i for i, b in enumerate(bodies) if b[2] == 0)
        companion = finisher ^ 1
        # Preserve the observed native standing floor instead of leaving the
        # neutral companion beside the entrance during unattended combat.
        if all(b[0x70] & 8 for b in bodies):
            shapes = [r.read(struct.unpack_from('<I', b, 0x68)[0], 4) for b in bodies]
            bottoms = [struct.unpack_from('<b', s, 1)[0]+s[3] for s in shapes]
            x = struct.unpack_from('<i', bodies[finisher], 8)[0]/65536
            foot = struct.unpack_from('<i', bodies[finisher], 12)[0]/65536+bottoms[finisher]
            y = foot-bottoms[companion]
            r.teleport(companion, x, y)
            report['companion_position_setup'] = dict(seat=companion, x=x, y=y,
                purpose='Position/history only on the finisher actual grounded floor; no contact/control flags')
        r.screenshot(args.out/'fight-ready.png')
        save()
        if args.entry_only:
            report['limits'] = 'Actual native entry/full HP/control return only. Boss HP/state untouched; defeat/reward and attack mechanics unverified.'
            report['clear_status'] = 'unverified; entry-only requested'
            report['success'] = True
            return 0
        command = [sys.executable, str(Path(__file__).with_name('test_mmx5_coop_grizzly_clear_runtime.py')),
                   '--session', str(args.session), '--out', str(args.out/'clear'),
                   '--stage', str(args.stage), '--finisher', str(finisher), '--paired-victory']
        result = subprocess.run(command, check=False)
        clear = json.loads((args.out/'clear/report.json').read_text(encoding='utf-8'))
        report['clear_status'] = 'passed bounded final hit/death/report' if clear['success'] else 'unverified; retained failed check'
        require(result.returncode == 0 and clear['success'], 'Native final-hit/report check did not pass')
        report['success'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        print(report['error'], flush=True)
        try:
            report['failure'] = sample('failure')[0]
            r.screenshot(args.out/'failure.png')
        except Exception:
            report['capture_error'] = traceback.format_exc()
    finally:
        try:
            r.release()
            p = r.read(PLAY, 0x20)
            if p[0] == 10 and not p[1] and not p[0x1C] and not r.personal_effects()['scene'][0]:
                r.tap(8, .15, 0)
            report['parked_in_native_pause'] = r.read(PLAY+1, 1) == b'\x02'
        except Exception:
            report['cleanup_error'] = traceback.format_exc()
        save()
    print(json.dumps({k: report.get(k) for k in ('success', 'entry_status', 'clear_status', 'error')}), flush=True)
    return 0 if report['success'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

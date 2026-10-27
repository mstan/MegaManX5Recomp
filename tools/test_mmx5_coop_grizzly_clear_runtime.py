"""Native X Maverick final hit and death/return from a prepared encounter.

Enemy HP may be lowered once to one positive HP. Only native buster input
can finish it. This does not qualify the route, intro prerequisite, damage
balance, later rewards/save screens or every frame of the victory animation.
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
from audit_mmx5_coop_locations import counter

# Original placement/dispatch mapping, independently read from SLUS_013.34
# and ROCK_X5.BIN. Stage5's class32 is the factory's on-foot boss; its road
# entrance/departure remains a separate vehicle contract and is not qualified
# by this final-hit check.
MAVERICKS = {1: (0, 0x36), 2: (1, 0x39), 3: (0, 0x38),
             4: (1, 0x34), 5: (0, 0x32), 6: (1, 0x35), 7: (0, 0x33), 8: (0, 0x37)}


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--stage', type=int, choices=tuple(MAVERICKS), default=1,
                        help='Original on-foot Maverick stage; prepare its actual native encounter first')
    parser.add_argument('--finisher', type=int, choices=(0,1), default=1,
                        help='Seat holding native X; covers each actual companion projection path')
    parser.add_argument('--paired-victory', action='store_true',
                        help='Require both living native victory poses without a hidden passenger')
    args = parser.parse_args()
    section, boss_class = MAVERICKS[args.stage]
    defeated_bit = 1 << (args.stage-1)
    parent = json.loads(args.session.read_text(encoding='utf-8'))
    exe = Path(parent['command'][0]).resolve()
    require(Path(psutil.Process(parent['pid']).exe()).resolve() == exe, 'Not owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest() == parent['executable_sha256'], 'Binary changed')
    r = Runtime(parent['port'])
    require(not r.request('netplay_status')['active'], 'Offline setup required')
    args.out.mkdir(parents=True, exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    report = dict(success=False, human_validation=False, paired_victory_requested=args.paired_victory,finisher=args.finisher, session=str(args.session.resolve()),
                  executable_sha256=parent['executable_sha256'],
                  source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  stage=args.stage, section=section, boss_class=hex(boss_class),
                  samples=[], transition_samples=[], input=[],
                  bypassed_prerequisites=['Prepared native encounter; qualify its route/door separately',
                                          'Both heroes receive HP-only refill during combat; original contacts/recovery remain enabled',
                                          'X finisher position/history follows an original normal-buster approach',
                                          'Original boss HP lowered once to one positive HP'],
                  limits='Native X final hit/death and first real frontend only. Record external chapter/story setup separately; do not credit intro/story, acquisition, save/reload, exact paired victory or network play from this result.')

    def save():
        (args.out/'report.json').write_bytes((json.dumps(report, indent=2)+'\n').encode('utf-8'))

    boss_address = None

    def sample():
        nonlocal boss_address
        personal = r.personal_effects()
        play = r.read(PLAY, 0x1C0)
        if boss_address is None:
            boss_address = struct.unpack_from('<I', play, 0x20)[0]
            require(0x80092090 <= boss_address < 0x80092090+48*0x9C and
                    (boss_address-0x80092090) % 0x9C == 0,
                    'Native health-bank actor is outside the original enemy pool')
        boss = r.read(boss_address, 0x9C)
        return dict(frame=r.request('frame')['frame'], world=personal['state']['world_ticks'],
                    boss_address=hex(boss_address), boss=boss.hex(), boss_hp=boss[0x5C],
                    boss_xy=[v/65536 for v in struct.unpack_from('<ii', boss, 8)],
                    play=play.hex(), scene=personal['scene'], life=personal['life'],
                    bodies=[b.hex() for b in personal['bodies']])

    def ticks(count):
        start = r.state()['world_ticks']
        deadline = time.monotonic()+20
        while r.state()['world_ticks'] < start+count:
            require(time.monotonic() < deadline, 'Native world stopped during shot')
            time.sleep(.02)

    def input_for(button, count):
        report['input'].append(dict(seat=args.finisher, button=hex(button), world_before=r.state()['world_ticks'],
                                    requested_ticks=count))
        r.input(button, args.finisher)
        ticks(count)

    try:
        r.release()
        paired_before={name: counter(r,'mmx5.coop.'+name) for name in
                       ('paired-victory-scenes','paired-victory-poses')}
        r.resume_native_pause()
        # Holding contact fixture12 pins native immunity timer61. If a boss
        # has already hurt a hero, that would hold action17 and suppress its
        # normal attacks. HP-only refill leaves native recovery untouched.
        r.fixture(11, hp=3)
        r.fixture(12, hp=0)
        before = report['before'] = sample()
        play = bytes.fromhex(before['play'])
        bodies = [bytes.fromhex(b) for b in before['bodies']]
        require(play[0] == 10 and play[0x0C:0x0E] == bytes((args.stage, section)),
                'Not the requested native Maverick gameplay')
        require(not play[1] and not play[0x1C] and before['scene'] == [0, 0], 'Scene still locked')
        require(before['life'][:2] == [0, 0], 'Both heroes must be alive')
        if args.stage == 5:
            require(all(b[0xD5] != 0xFF for b in bodies),
                    'Stage5 final-hit check requires the original on-foot factory encounter')
        campaign=play[0x44]
        require(bodies[args.finisher][2] == 0, 'Finisher must be native X')
        require(bodies[args.finisher][0x93] == 0, 'Finisher must select the native normal buster')
        require(not play[0x4C] & defeated_bit, 'Requested Maverick is already defeated')
        boss = bytes.fromhex(before['boss'])
        require(boss[0] and boss[1] == boss_class and boss[4] == 1 and boss[0x5C] & 127,
                'Original live Maverick actor not ready')
        floor = struct.unpack_from('<i', bodies[args.finisher], 12)[0]/65536
        camera = r.read(0x8009A1F8, 0xFC)
        left = struct.unpack_from('<h', camera, 0x1E)[0]+16
        right = struct.unpack_from('<h', camera, 0x1C)[0]+304
        require(right > left, 'Invalid native arena interval')
        report['enemy_hp_setup'] = dict(address=hex(boss_address+0x5C), before=boss[0x5C], value=1,
                                        written=boss[0x5C] != 1,
                                        purpose='positive-health setup only; never write death/completion')
        if boss[0x5C] != 1:
            r.request('write_ram', addr=hex(boss_address+0x5C), val='0x01')
        report['after_hp_setup'] = sample()
        require(report['after_hp_setup']['boss_hp'] == 1, 'HP setup did not hold')
        save()
        deadline = time.monotonic()+60
        for attempt in range(24):
            current = sample()
            report['samples'].append(current)
            save()
            b = bytes.fromhex(current['boss'])
            if not b[0] or b[1] != boss_class or not b[0x5C] & 127 or b[4] != 1:
                report['terminal_boss'] = current
                break
            require(current['life'][:2] == [0, 0], 'Fight lost a hero')
            require(time.monotonic() < deadline, 'Bounded final-hit input expired')
            if bytes.fromhex(current['bodies'][args.finisher])[5] == 0x11:
                # A real damage reaction must finish natively before firing.
                input_for(0, 12)
                continue
            if args.stage == 6 and not struct.unpack_from('<I', b, 0x54)[0]:
                input_for(0, 6)
                continue
            on_left = current['boss_xy'][0] > (left+right)/2
            distance = 120 if args.stage == 6 else 80
            target = max(left, min(right, current['boss_xy'][0]+(-distance if on_left else distance)))
            # Stage6's original boss flies above floor-level buster fire and
            # disables its target box during parts of flight. Position-only
            # setup aligns a real native shot with its current vulnerable Y.
            aim_y = min(floor, current['boss_xy'][1]+16) if args.stage == 6 else floor
            try:
                r.teleport(args.finisher, target, aim_y)
            except RuntimeError:
                # An already-fired native shot can kill the boss between
                # sample() and the setup boundary. A new victory scene then
                # correctly refuses teleport; retain that real completion.
                terminal = sample()
                report['samples'].append(terminal)
                terminal_body = bytes.fromhex(terminal['boss'])
                if not terminal_body[0x5C] & 127 and terminal_body[4] == 2:
                    report['terminal_boss'] = terminal
                    break
                raise
            input_for(0x20 if on_left else 0x80, 2)
            input_for(0x8000, 4)
            input_for(0, 8)
        else:
            raise AssertionError('Native normal buster never finished the one-HP Maverick')
        r.release()
        require(any(not bytes.fromhex(row['boss'])[0x5C] & 127 for row in report['samples']),
                'Native zero boss HP was not observed')
        r.screenshot(args.out/'native-death.png')
        deadline = time.monotonic()+100
        signature = None
        paired_seen=False
        held_pose_captured=False
        hidden_during_victory=[]
        while True:
            row = sample()
            report['transition_samples'].append(row)
            p = bytes.fromhex(row['play'])
            bs = [bytes.fromhex(b) for b in row['bodies']]
            if p[0]==10 and any(b[4]==1 and b[5]==0x19 for b in bs):
                if any(not b[3] or b[4]==3 for b in bs):
                    hidden_during_victory.append(row['world'])
                if all(b[3] and b[4]==1 and b[5]==0x19 for b in bs) and not paired_seen:
                    paired_seen=True
                    report['paired_native_victory']=row
                    r.screenshot(args.out/'paired-native-victory.png')
                if all(b[3] and b[4]==1 and b[5]==0x19 and b[6]>=1 for b in bs) and not held_pose_captured:
                    held_pose_captured=True
                    report['paired_held_victory']=row
                    r.screenshot(args.out/'paired-held-victory.png')
            current = (p[:2].hex(), p[0x1C], tuple(row['scene']),
                       tuple((b[3], b[4], b[5]) for b in bs), p[0x4C])
            if current != signature:
                print('Native clear transition', row['world'], current, flush=True)
                signature = current
                save()
            if p[0] != 10 and 'first_boundary' not in report:
                report['first_boundary'] = row
            # Native mode11 performs the stage-end cleanup before mode12
            # commits rewards and presents the mission report. Observe that
            # intermediate transition and mode12 initialization without
            # treating either as a retry/failure.
            if p[0] not in (10, 11, 12) or (p[0]==12 and p[1] and p[0x4C]&defeated_bit):
                report['frontend'] = row
                break
            require(time.monotonic() < deadline, 'Native clear did not reach a frontend')
            if p[0] == 10 and p[0x1C]:
                owner = max(0, row['scene'][0]-1)
                r.tap(0x4000, .08, owner)
            time.sleep(.03)
        p = bytes.fromhex(report['frontend']['play'])
        require(p[0] == 12, 'Clear did not reach the original mission report')
        require(p[0x4C] & defeated_bit, 'Native clear did not commit the defeated bit')
        require((p[0x4C] & ~defeated_bit) == (play[0x4C] & ~defeated_bit),
                'Native clear changed another Maverick defeated bit')
        require(p[0x44] == campaign, 'Final hit changed the campaign')
        require(not bytes.fromhex(report['frontend']['boss'])[0], 'Original boss actor did not retire')
        r.screenshot(args.out/'native-mission-report.png')
        report['victory_review']=dict(both_native_poses_observed=paired_seen,
            hidden_while_other_celebrated=hidden_during_victory,
            counters={name:counter(r,'mmx5.coop.'+name)-before for name,before in paired_before.items()})
        if args.paired_victory:
            require(paired_seen,'Both living heroes never entered native victory action25 together')
            require(not hidden_during_victory,'A living hero hid while the other celebrated')
            require(report['victory_review']['counters']==
                    {'paired-victory-scenes':1,'paired-victory-poses':1},
                    'Paired victory did not start once with one companion pose')
        report['render_stats'] = r.request('render_pass_stats')
        require(all(not report['render_stats'].get(k, 0) for k in
                    ('verify_mismatch', 'watchdog', 'device_reads', 'vram_leaks')), 'Private redraw fault')
        report['success'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        print(report['error'], flush=True)
        try:
            report['failure'] = sample()
            r.screenshot(args.out/'failure.png')
        except Exception:
            report['capture_error'] = traceback.format_exc()
    finally:
        try:
            r.release()
        except Exception:
            report['cleanup_error'] = traceback.format_exc()
        save()
    print(json.dumps({k: report.get(k) for k in ('success', 'error', 'cleanup_error')}), flush=True)
    return 0 if report['success'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

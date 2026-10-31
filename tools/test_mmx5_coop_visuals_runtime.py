"""Native personal charge/dash effects in an owned offline X5 session.

Checkpoint, HP/contact protection and positions are explicit setup bypasses.
Controller input and original updates must create the effects. Native actor
state and composed captures qualify bounded cases, not the full campaign.
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
from audit_mmx5_coop_locations import counter, wait_ready


def require(value, message):
    if not value:
        raise AssertionError(message)


def serial(value):
    if isinstance(value, bytes):
        return value.hex()
    if isinstance(value, list):
        return [serial(item) for item in value]
    if isinstance(value, dict):
        return {key: serial(item) for key, item in value.items()}
    return value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--dat', type=Path, required=True,
                        help='Original ROCK_X5.DAT for native assembly-table identity')
    parser.add_argument('--survivor-only', action='store_true',
                        help='Check X/P2 glow after P1 is fully fallen and hidden')
    parser.add_argument('--after-scene',type=Path,help='Use the current fallen-P1 survivor from this passed scene report')
    args = parser.parse_args()
    require(not args.after_scene or args.survivor_only,'--after-scene requires --survivor-only')
    session = json.loads(args.session.read_text(encoding='utf-8'))
    exe = Path(session['command'][0]).resolve()
    require(Path(psutil.Process(session['pid']).exe()).resolve() == exe, 'Not the owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest() == session['executable_sha256'], 'Owned binary changed')
    r = Runtime(session['port'])
    require(not r.request('netplay_status')['active'], 'Offline fixture required')
    args.out.mkdir(parents=True, exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    report = dict(success=False, human_validation=False, cases=[],
                  executable_sha256=session['executable_sha256'], session=str(args.session.resolve()),
                  resident_loading=session.get('resident_loading', True),
                  bypassed_prerequisites=['Grizzly checkpoint1', 'HP/contact protection', 'position/history fixture'],
                  limits='Native personal effect state/drawing and retained captures; visual comparison and broader mechanics separate.')
    if args.after_scene:
        prior=json.loads(args.after_scene.read_text(encoding='utf-8'))
        require(prior['success'] and prior['executable_sha256'] == session['executable_sha256'] and
                Path(prior['session']).resolve() == args.session.resolve(),'Prior scene is not a passed case on this owned session')
        report['prior_scene']=dict(path=str(args.after_scene.resolve()),sha256=hashlib.sha256(args.after_scene.read_bytes()).hexdigest())
        report['bypassed_prerequisites']=['Current state from the linked prior scene test; its original setup bypasses apply',
                                         'Contact protection during charge setup; expires before capture']
    dat = args.dat.read_bytes()
    report['dat_sha256'] = hashlib.sha256(dat).hexdigest()
    assemblies = []
    for record in (88, 90):
        sector, size = struct.unpack_from('<II', dat, record*8)
        entry = dat[sector*2048:sector*2048+size]
        require(len(entry) == size and struct.unpack_from('<I', entry, 4)[0] == size,
                'Invalid original character record')
        cursor = 2048
        for asset in range(10):
            kind, size = struct.unpack_from('<II', entry, 8+asset*8)
            if asset == 9:
                require(kind == 3 and cursor+size <= len(entry), 'Missing native assembly asset')
                assemblies.append(entry[cursor:cursor+size])
            cursor += (size+2047) & ~2047

    def wait(predicate, message, seconds=15):
        deadline = time.monotonic()+seconds
        while not predicate():
            require(time.monotonic() < deadline, message)
            time.sleep(.01)

    def ticks(count):
        start = r.state()['world_ticks']
        wait(lambda: r.state()['world_ticks'] >= start+count, 'World stopped during input')

    def idle():
        state = r.personal_effects()
        if args.after_scene:return state['life'][:2] == [5,0] and state['bodies'][1][4:6] == bytes((1,2))
        return state['life'][:2] == [0, 0] and all(body[4:6] == b'\x01\x02' for body in state['bodies'])

    def prepare():
        r.release()
        wait(idle, 'Heroes did not return to native idle')
        if args.after_scene:return
        x, y = struct.unpack_from('<ii', r.personal_effects()['bodies'][0], 8)
        r.teleport(1, x/65536-48, y/65536)
        ticks(6)
        wait(idle, 'Position fixture did not settle')

    def timing(before, after, private=True):
        worlds = after['state']['world_ticks']-before['state']['world_ticks']
        native = after['state']['native_timer']-before['state']['native_timer']
        require(worlds > 0 and worlds == native, 'Personal effects changed the once-per-world clock')
        if private:
            # A diagnostic publication may precede either late native pass by
            # one world update. This allows that phase, not a second tick.
            for start, end in zip(before['native_effect_ticks'], after['native_effect_ticks']):
                require(abs((end-start)-worlds) <= 1, 'Personal native update did not run once per world')

    def owned(actors, body, glow=False):
        assembly = struct.unpack_from('<I', body, 0x3c)[0]
        require(0x9f000000 <= assembly < 0x9f100000, 'P2 has no private character assembly')
        table = assemblies[body[2]]
        base = assembly-struct.unpack_from('<I', table)[0]
        require(r.read(base, 0x74) == table[:0x74], 'Private assembly header differs from original character')
        # Original 3BD64 uses header+0 for the body; 3BE30/3BE50 uses
        # header+70 for the two charge glow actors. Trails copy the body.
        expected = base+struct.unpack_from('<I', table, 0x70)[0] if glow else assembly
        require(all(struct.unpack_from('<I', actor, 0x3c)[0] == expected for actor in actors),
                'Personal effect inherited the other character assembly')

    def dash(label):
        prepare()
        case = dict(name=label+'-dash', samples=[])
        report['cases'].append(case)
        before = r.personal_effects()
        case['before'] = serial(before)
        drawn = counter(r, 'mmx5.coop.p2-dash-trails-drawn')
        r.input(0x2020, 1)  # Native Circle + Right; no dash-state writes.
        deadline = time.monotonic()+3
        active = []
        while time.monotonic() < deadline:
            state = r.personal_effects()
            case['samples'].append(serial(state))
            active = [actor for actor in state['trails'][1] if actor[3]]
            if active and 0 < state['bodies'][1][0x8c] < 128:
                break
            time.sleep(.01)
        require(active and 0 < state['bodies'][1][0x8c] < 128, 'Original input did not create native dash trails')
        owned(active, state['bodies'][1])
        require(not any(actor[3] for actor in state['trails'][0]), 'P2 dash changed P1 trails')
        require(state['bodies'][0][8:16] == before['bodies'][0][8:16], 'P2 dash moved P1')
        wait(lambda: counter(r, 'mmx5.coop.p2-dash-trails-drawn') > drawn, 'Native P2 trails were never drawn')
        r.screenshot(args.out/(label+'-dash.png'))
        r.release()
        wait(lambda: not any(actor[3] for actor in r.personal_effects()['trails'][1]), 'Native trails did not expire')
        after = r.personal_effects()
        timing(before, after)
        require(not after['state']['failed'], 'Trail rendering failed')
        case['after'] = serial(after)
        print('PASS', case['name'], 'native private trails and expiry', flush=True)

    def charge(label, survivor=False):
        prepare()
        case = dict(name=label+'-charge', survivor=survivor)
        report['cases'].append(case)
        before = r.personal_effects()
        case['before'] = serial(before)
        drawn = counter(r, 'mmx5.coop.p2-charge-glow-drawn')
        shot_count = before['bodies'][1][0x98]
        if args.after_scene:require(shot_count == 0,'Prior scene left an active native buster shot')
        r.input(0x8000, 1)
        ticks(120)
        if survivor:
            if not args.after_scene:
                r.fixture(1, seat=0)
                report['bypassed_prerequisites'].append('P1 fatal HP fixture for survivor rendering')
            wait(lambda: r.lifecycle()[0] == 5, 'P1 native death did not finish')
            hidden_drawn = counter(r, 'mmx5.coop.p2-charge-glow-drawn')
            r.fixture(12, hp=0)
            wait(lambda: not r.players()[1][0xA4], 'Native survivor immunity did not expire')
            ticks(12)
            case['glow_draws_after_primary_hidden'] = counter(r, 'mmx5.coop.p2-charge-glow-drawn')-hidden_drawn
            require(case['glow_draws_after_primary_hidden'] > 0, 'Glow did not draw after P1 became hidden')
        state = r.personal_effects()
        case['charged'] = serial(state)
        require(state['bodies'][1][0x95] == 0x12, 'Base X/P2 did not reach native full charge')
        active = [actor for actor in state['glow'][1] if actor[3]]
        require(active, 'Full native charge has no private glow actor')
        owned(active, state['bodies'][1], glow=True)
        x, y = struct.unpack_from('<ii', state['bodies'][1], 8)
        require(all(abs(struct.unpack_from('<i', actor, 8)[0]-x) < 65536 and
                    abs(struct.unpack_from('<i', actor, 12)[0]-y) < 65536 for actor in active),
                'Charge glow did not follow its own hero')
        require(not any(actor[3] for actor in state['glow'][0]), 'P2 charge changed P1 glow')
        require(counter(r, 'mmx5.coop.p2-charge-glow-drawn') > drawn, 'Native P2 charge glow was never drawn')
        if survivor:
            require(state['life'][:2] == [5,0] and not state['life'][2] and not state['bodies'][0][3],
                    'Charge capture did not establish a living P2 and fully hidden fallen P1')
        r.screenshot(args.out/(label+'-charged.png'))
        r.release()
        wait(lambda: not any(actor[3] for actor in r.personal_effects()['glow'][1]), 'Charge glow did not stop on release')
        if args.after_scene:
            wait(lambda: r.personal_effects()['bodies'][1][0x98] > shot_count,'Post-scene charge release produced no active native shot')
            shot = r.personal_effects()
            case['released_with_native_shot'] = serial(shot)
            case['native_active_buster_count'] = shot['bodies'][1][0x98]
            require(case['native_active_buster_count'] == 1,'Post-scene full charge did not produce one active native shot')
        after = r.personal_effects()
        if args.after_scene:
            require(after['bodies'][1][0x5C] == before['bodies'][1][0x5C] and
                    after['bodies'][1][0xA8:0xC8] == before['bodies'][1][0xA8:0xC8],
                    'Post-scene charge changed the survivor inventory')
        timing(before, after, private=not survivor)
        require(not after['state']['failed'], 'Charge effect rendering failed')
        case['after'] = serial(after)
        print('PASS', case['name'], 'native private glow and release', flush=True)

    try:
        r.resume_native_pause()
        r.release()
        if args.after_scene:
            require(r.lifecycle()[:2] == (5,0),'Current scene has not left a fallen P1 and living P2')
            r.fixture(12,hp=2)
        else:
            r.fixture(11, hp=3)
            r.fixture(12, hp=3)
            previous = counter(r, 'mmx5.coop.enrolled')
            r.load_location(1, 0, 1)
            wait_ready(r, previous)
        state = r.personal_effects()
        report['p2_character'] = state['bodies'][1][2]
        require(not args.survivor_only or report['p2_character'] == 0, 'Survivor-only case requires X/P2')
        phases = () if args.survivor_only else ('before-menu', 'after-p2-menu', 'after-p1-menu', 'after-rejoin')
        for phase in phases:
            if phase in ('after-p2-menu', 'after-p1-menu'):
                owner = 1 if phase == 'after-p2-menu' else 0
                r.tap(8, .15, owner)
                wait(lambda: r.read(PLAY+1, 1) == b'\x02', 'Owned menu did not open')
                r.resume_native_pause()
            elif phase == 'after-rejoin':
                prepare()
                withdrawing = dict(name='active-charge-withdrawal', applies=report['p2_character'] == 0)
                report['cases'].append(withdrawing)
                if withdrawing['applies']:
                    r.input(0x8000, 1)
                    ticks(120)
                    state = r.personal_effects()
                    active = [actor for actor in state['glow'][1] if actor[3]]
                    require(active, 'Withdrawal setup has no native charged glow')
                    owned(active, state['bodies'][1], glow=True)
                    withdrawing['charged'] = serial(state)
                r.input(0x8001 if withdrawing['applies'] else 1, 1)
                wait(lambda: r.lifecycle()[1] == 2, 'Native withdrawal did not finish')
                state = r.personal_effects()
                withdrawing['absent'] = serial(state)
                require(not any(actor[3] for pool in ('glow', 'trails') for actor in state[pool][1]),
                        'Withdrawn hero left visible personal effects')
                r.input(0, 1)
                ticks(4)
                r.tap(1, .15, 1)
                wait(lambda: r.lifecycle()[1] == 0, 'Native rejoin did not finish')
                print('PASS native withdrawal clears personal effects and rejoins', flush=True)
            dash(phase)
            if report['p2_character'] == 0:
                charge(phase)
        if report['p2_character'] == 0:
            charge('p2-survivor', survivor=True)
        report['success'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        try:
            report['failure'] = serial(r.personal_effects())
            r.screenshot(args.out/'failure.png')
        except Exception:
            report['capture_error'] = traceback.format_exc()
        raise
    finally:
        try:
            r.release()
            r.fixture(12, hp=0)
            owner = 0 if r.lifecycle()[0] == 0 else 1
            if r.read(PLAY+1, 1) == b'\x00':
                r.tap(8, .15, owner)
        except Exception:
            report['cleanup_error'] = traceback.format_exc()
        finally:
            (args.out/'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')


if __name__ == '__main__':
    main()

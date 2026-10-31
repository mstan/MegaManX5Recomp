"""Controlled original stand-lock/fatal/return check; not an actual door pass."""
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
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--session', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--fatal', action='store_true')
    p.add_argument('--hold-after-fatal', action='store_true', help='Retain the original lock until the owner finishes dying')
    a = p.parse_args()
    require(not a.hold_after_fatal or a.fatal, '--hold-after-fatal requires --fatal')
    session = json.loads(a.session.read_text(encoding='utf-8'))
    exe = Path(session['command'][0]).resolve()
    require(Path(psutil.Process(session['pid']).exe()).resolve() == exe, 'Not owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest() == session['executable_sha256'], 'Binary changed')
    r = Runtime(session['port'])
    require(not r.request('netplay_status')['active'], 'Offline fixture only')
    a.out.mkdir(parents=True, exist_ok=False)
    (a.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    report = dict(success=False, fatal_requested=a.fatal, hold_after_fatal=a.hold_after_fatal, human_validation=False, samples=[],
                  harness_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  executable_sha256=session['executable_sha256'], session=str(a.session.resolve()),
                  resident_loading=session.get('resident_loading', True),
                  bypassed_prerequisites=['Grizzly checkpoint1 original loader', 'HP/contact protection during setup',
                                         'P2 HP17 and ammo73 inventory fixture',
                                         'controlled original3D29C stand-lock and3D2C4 release calls',
                                         'P1 fatal HP fixture when requested'],
                  limits='Controlled native script lock only; no door collision, door controller or committed destination is qualified.')

    def save():
        (a.out/'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')

    def observe(label):
        s = r.personal_effects()
        report['samples'].append(dict(label=label, personal=serial(s), play=r.read(PLAY, 0x80).hex()))
        save()
        return s

    def wait(predicate, label, timeout=20):
        deadline = time.monotonic()+timeout
        while True:
            s = r.personal_effects()
            if predicate(s):
                return s
            require(time.monotonic() < deadline, label)
            time.sleep(.025)

    try:
        r.release()
        r.resume_native_pause()
        r.fixture(11, hp=3)
        r.fixture(12, hp=3)
        previous = counter(r, 'mmx5.coop.enrolled')
        r.load_location(1, 0, 1)
        wait_ready(r, previous)
        r.release()
        wait(lambda s: all(b[4] == 1 and b[5] == 2 for b in s['bodies']) and s['bodies'][0][0x70]&8,
             'Native grounded idle did not follow arrival', 30)
        r.fixture(11, hp=0)
        r.fixture(2, seat=1, hp=17, ammo=73)
        before = observe('native-grounded-control')
        handoffs_before = counter(r, 'mmx5.coop.dead-scene-owner-handoffs')
        inventory = before['bodies'][1][0xA8:0xC8]
        retries = r.read(PLAY+0x45, 1)
        flags = r.read(PLAY+0x4C, 8)
        r.fixture(5)
        locked = wait(lambda s: s['scene'] == [1, 3], 'Passenger did not finish native departure')
        require(locked['life'][:2] == [0, 0] and locked['bodies'][1][4] == 3 and not locked['bodies'][1][3],
                'Stand-lock did not establish a living hidden passenger')
        observe('locked-hidden-passenger')
        if a.fatal:
            r.fixture(1, seat=0)
            dead = wait(lambda s: s['life'][0] in (4, 5), 'Original fatal entry did not run')
            require(dead['life'][1] == 0, 'Fatal owner killed its living passenger')
            observe('native-owner-fatal')
            if a.hold_after_fatal:
                held = wait(lambda s: s['life'][0] == 5, 'Native owner death did not finish while the script remained locked')
                require(held['scene'] == [2,0] and held['life'][1] == 0 and r.read(PLAY+0x1C,1) == bytes((1,)),
                        'Finishing the old owner death lost the inherited native script lock')
                observe('original-lock-held-after-owner-fell')
        r.fixture(6)
        observe('original-lock-release')
        def restored(s):
            b = s['bodies'][1]
            return s['scene'] == [0, 0] and s['life'][1] == 0 and b[3] and b[4] == 1 and \
                b[5] == 2 and not any(b[o] for o in (0x7A, 0xD0, 0xD4)) and \
                (not a.fatal or s['life'][0] == 5)
        after = wait(restored, 'Living passenger did not regain grounded control after original lock release', 30)
        require(not after['state']['failed'], 'Co-op plugin failed')
        require(after['bodies'][1][0x5C] == 17 and after['bodies'][1][0xA8:0xC8] == inventory,
                'Script recovery changed private passenger resources')
        require(r.read(PLAY+0x45, 1) == retries and r.read(PLAY+0x4C, 8) == flags,
                'One fatal owner changed retries or boss outcomes')
        private_body_before = counter(r, 'mmx5.coop.local-view-p2-body-quads')
        before_x = struct.unpack_from('<i', after['bodies'][1], 8)[0]
        start = after['state']['world_ticks']
        r.input(0x80, 1)
        moved = wait(lambda s: s['state']['world_ticks'] >= start+12, 'Survivor world stopped')
        r.release()
        require(struct.unpack_from('<i', moved['bodies'][1], 8)[0] < before_x-65536,
                'Living passenger did not regain native movement')
        settled = wait(lambda s: s['bodies'][1][4:6] == bytes((1,2)), 'Native movement did not settle')
        floor_y = struct.unpack_from('<i', settled['bodies'][1], 12)[0]
        jump_start = settled['state']['world_ticks']
        r.input(0x4000, 1)
        jumped = wait(lambda s: s['state']['world_ticks'] >= jump_start+5, 'Native jump stopped')
        r.release()
        require(struct.unpack_from('<i', jumped['bodies'][1], 12)[0] < floor_y-4*65536,
                'Returned passenger could not jump through original input/physics')
        observe('native-jump-after-recovery')
        wait(lambda s: s['bodies'][1][4:6] == bytes((1,2)) and
             abs(struct.unpack_from('<i',s['bodies'][1],12)[0]-floor_y) <= 65536,
             'Native terrain did not return the passenger to its platform')
        observe('native-movement-and-landing-after-recovery')
        r.fixture(12, hp=0)
        wait(lambda s: not s['bodies'][1][0xA4], 'Native arrival immunity did not expire')
        expired = observe('native-immunity-expired')
        wait(lambda s: s['state']['world_ticks'] >= expired['state']['world_ticks']+12, 'Present capture did not advance')
        r.screenshot(a.out/'recovered.png')
        report['private_body_quads_after_recovery'] = counter(r, 'mmx5.coop.local-view-p2-body-quads')-private_body_before
        if a.fatal and session.get('local_cameras') == 'split':
            require(report['private_body_quads_after_recovery'] > 0, 'Private camera emitted no living P2 body after P1 became hidden')
        report['render_stats'] = stats = r.request('render_pass_stats')
        require(all(stats.get(k, 0) == 0 for k in ('verify_mismatch', 'watchdog', 'device_reads', 'vram_leaks')),
                'Private rendering fault during controlled scene')
        report['handoffs'] = counter(r, 'mmx5.coop.dead-scene-owner-handoffs')-handoffs_before
        require(report['handoffs'] == int(a.fatal), 'Scene recovery did not hand ownership over exactly once')
        report['success'] = True
        print('PASS controlled original scene return; fatal owner:', a.fatal, flush=True)
    except Exception as error:
        report.update(error=f'{type(error).__name__}: {error}', traceback=traceback.format_exc())
        print(report['traceback'], flush=True)
    finally:
        try:
            r.release()
            observe('final')
        except Exception as error:
            report['final_error'] = str(error)
        save()
    return 0 if report['success'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

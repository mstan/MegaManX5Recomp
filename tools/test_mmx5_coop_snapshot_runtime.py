"""One owned session: private resources, attacks, guest menu and native scene restore.

Setup teleports/protection/resource grants bypass traversal and acquisition.
Actual native menu use, script activation and completion are not seeded.
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
from test_mmx5_coop_scene_fatal_runtime import serial


def require(value, message):
    if not value:
        raise AssertionError(message)


def state_operation(r, op, slot):
    before = r.request('savestate_status')['generation']
    r.request('savestate', op=op, slot=slot)
    deadline = time.monotonic()+25
    while True:
        status = r.request('savestate_status')
        if status['generation'] > before and not status['pending']:
            require(status['last_ok'] == 1 and status['last_op'] == op and
                    status['last_slot'] == slot, f'{op} slot{slot} rejected: {status}')
            return status
        require(time.monotonic() < deadline, f'{op} did not complete')
        time.sleep(.03)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--session', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--scene-only', action='store_true', help='Continue the actual scene check independently of the completed resource/menu batch')
    p.add_argument('--scene-owner', type=int, choices=(0,1), default=0)
    p.add_argument('--scene-kind', choices=('controlled','authored'), default='controlled',
                   help='Controlled uses original stand/release calls; authored requires a native boss approach')
    a = p.parse_args()
    parent = json.loads(a.session.read_text())
    exe = Path(parent['command'][0]).resolve()
    require(Path(psutil.Process(parent['pid']).exe()).resolve() == exe, 'Not owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest() == parent['executable_sha256'], 'Binary changed')
    require(parent.get('separate_etanks'), 'This combined check requires private E on')
    r = Runtime(parent['port'])
    require(not r.request('netplay_status')['active'], 'Offline only')
    a.out.mkdir(parents=True, exist_ok=False)
    (a.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    report = dict(success=False, session=str(a.session), executable_sha256=parent['executable_sha256'],
                  samples=[], operations=[], checks=[], scene_only=a.scene_only, scene_owner=a.scene_owner, scene_kind=a.scene_kind,
                  bypasses=['native checkpoint setup bypasses route/door', 'HP/contact protection during setup',
                            'distinct HP/ammo and initial E supplies granted; acquisition unqualified'],
                  limits='Offline same-process saves/loads. Cold load, rewind, network and audio listening require separate evidence.')
    if a.scene_kind=='controlled':
        report['bypasses'].append('Original stand/release calls establish a controlled script hold; authored trigger/door/boss completion unqualified')

    def save():
        (a.out/'report.json').write_text(json.dumps(report, indent=2)+'\n')

    def observe(label):
        s = r.personal_effects()
        report['samples'].append(dict(label=label, personal=serial(s),
                                     play=r.read(PLAY, 0x80).hex(), menu=r.read(0x800D4F40, 0x60).hex()))
        save()
        return s

    def wait(test, label, seconds=35):
        deadline = time.monotonic()+seconds
        while True:
            s = r.personal_effects()
            if test(s):
                return s
            require(time.monotonic() < deadline, label)
            time.sleep(.03)

    def frames(n):
        start = r.request('frame')['frame']
        wait(lambda s: r.request('frame')['frame'] >= start+n, 'Frontend stopped', 15)

    def pulse(button, seat=0):
        r.input(button, seat)
        frames(6)
        r.input(0, seat)
        frames(8)

    def load(stage, cp):
        r.release();r.resume_native_pause();r.fixture(11,hp=3);r.fixture(12,hp=0)
        enrolled = counter(r, 'mmx5.coop.enrolled')
        r.load_location(stage, 0, cp)
        deadline = time.monotonic()+90
        while True:
            play = r.read(PLAY, 0x20);s = r.personal_effects()
            if play[0] == 10 and not play[1] and not play[0x1C] and \
               counter(r,'mmx5.coop.enrolled') > enrolled and s['life'][:2] == [0,0]:
                break
            require(time.monotonic() < deadline, 'Native checkpoint setup timeout')
            if play[0] in (4,5,6,20):r.tap(0x4000,.04)
            time.sleep(.04)
        wait(lambda s: all(b[4:6] == b'\x01\x02' and b[0x70]&8 for b in s['bodies']), 'Heroes did not land')

    def menu(seat):
        pulse(8, seat)
        wait(lambda s: r.read(PLAY+1,1) == b'\x02' and r.read(0x800D4F44,1) == b'\x01' and
             not r.read(0x8009A424,1)[0], 'Native menu did not open')
        require(r.read(r.diagnostic()+0x48,1)[0] == seat+1, 'Wrong menu owner')

    def operation(op, slot):
        report['operations'].append(state_operation(r,op,slot));save()

    def scene_batch():
        # Same candidate/session: an original stand hold or authored approach.
        load(1,0 if a.scene_kind=='controlled' else 4);r.fixture(11,hp=0);r.fixture(12,hp=3)
        # The native corridor is longer than Unified's two-player tether.
        # Move the pair; a stationary passenger would cap either approach at
        #9717.5 before the boss placement exists. No admission flag is seeded.
        if a.scene_kind=='controlled':r.fixture(5,seat=a.scene_owner)
        else:r.input(0x20,0);r.input(0x20,1)
        wait(lambda s: s['scene'][0] == a.scene_owner+1, 'Native scene did not acquire requested owner')
        r.release();locked=wait(lambda s: s['scene'] == [a.scene_owner+1,3], 'Passenger did not finish departure')
        observe('native-scene-C-before');r.screenshot(a.out/'C-before.png');operation('save',6)

        def finish_scene(label):
            if a.scene_kind=='controlled':r.fixture(6,seat=a.scene_owner)
            deadline=time.monotonic()+90
            while True:
                s=r.personal_effects();play=r.read(PLAY,0x20)
                if not s['scene'][0] and not play[1] and not play[0x1C] and all(b[4:6] == b'\x01\x02' for b in s['bodies']):break
                require(time.monotonic()<deadline, 'Native scene did not release control')
                if play[0x1C]:r.tap(0x4000,.08,a.scene_owner)
                time.sleep(.03)
            return observe(label)

        finish_scene('C-first-native-finish')
        operation('load',6)
        restored=observe('C-restored-owned-scene')
        require(restored['scene'] == [a.scene_owner+1,3] and restored['life'] == locked['life'], 'Native scene owner/passenger did not restore')
        finish_scene('C-second-native-finish')
        before_x=struct.unpack_from('<i',r.players()[a.scene_owner],8)[0]
        pulse(0x20,a.scene_owner)
        require(struct.unpack_from('<i',r.players()[a.scene_owner],8)[0] > before_x+65536, 'Native control did not recover after second finish')
        r.screenshot(a.out/'C-finished.png')
        report['checks'].append(f'{a.scene_kind} owned native scene save/release/load/release and passenger/control recovery')

    try:
        if not a.scene_only:
            load(1,0)
            r.fixture(11,hp=0);r.fixture(12,hp=3)
            r.fixture(2,seat=0,hp=13,ammo=51);r.fixture(2,seat=1,hp=11,ammo=73)
            r.fixture(4,seat=0,hp=16);r.fixture(4,seat=1,hp=8)
            # Native attacks immediately before pausing retain their lifetime in B.
            r.input(0x8000,0);r.input(0x8000,1);frames(4);r.release();menu(1)
            before = observe('guest-menu-B-before')
            r.screenshot(a.out/'B-before.png')
            operation('save',7)
            world=before['state']['world_ticks'];pulse(8,0)
            require(r.read(PLAY+1,1) == b'\x02' and r.state()['world_ticks'] == world, 'Other seat resumed owned menu')
            pulse(0x10,1)
            require(r.read(0x800D4F54,1)[0] == 0x0A, 'Native E1 cursor not selected')
            pulse(0x4000,1)
            wait(lambda s: s['bodies'][1][0x5C] == 19 and r.read(0x800D4F45,1) == b'\x01', 'Native E heal did not consume8')
            observe('B-spent-once');r.resume_native_pause()
            r.input(0x20,0);r.input(0x80,1);frames(12);r.release()
            r.fixture(2,seat=0,hp=7,ammo=3);r.fixture(2,seat=1,hp=5,ammo=9)
            menu(0);observe('different-live-menu')
            operation('load',7)
            restored = observe('guest-menu-B-restored')
            require(r.read(PLAY+1,1) == b'\x02' and r.read(r.diagnostic()+0x48,1)[0] == 2, 'P2 menu ownership lost')
            require(restored['state']['world_ticks'] == before['state']['world_ticks'], 'Menu world time did not restore')
            for seat in range(2):
                require(restored['bodies'][seat][8:16] == before['bodies'][seat][8:16] and
                        restored['bodies'][seat][0x5C] == before['bodies'][seat][0x5C] and
                        restored['bodies'][seat][0xA8:0xC8] == before['bodies'][seat][0xA8:0xC8],
                        'Actor position/HP/ammo did not restore')
            r.screenshot(a.out/'B-restored.png')
            pulse(0x10,1);require(r.read(0x800D4F54,1)[0] == 10, 'Restored menu tank cursor')
            pulse(0x4000,1)
            wait(lambda s: s['bodies'][1][0x5C] == 19 and r.read(0x800D4F45,1) == b'\x01', 'Restored private E not usable exactly once')
            r.resume_native_pause();observe('B-restored-exited')
            report['checks'].append('P2 owned menu/time/both actor HP+ammo+position/private E native spending restored')
            # A live-world snapshot must also recover after a genuine native reload.
            r.input(0x8000,0);r.input(0x8000,1);frames(3);r.release()
            live=observe('world-A-before');operation('save',8)
            load(1,4)
            operation('load',8)
            after=observe('world-A-after-native-reload')
            require(r.read(PLAY+12,2) == b'\x01\0' and after['life'][:2] == [0,0] and not after['state']['failed'], 'Live world restore failed')
            require([b[0x5C] for b in after['bodies']] == [b[0x5C] for b in live['bodies']], 'Live private HP lost')
            r.screenshot(a.out/'A-restored.png')
            report['checks'].append('Live-world restore after original checkpoint reload; both lives/resources retained')
        scene_batch()
        report['success']=True
        print('PASS combined offline snapshot checks',flush=True)
    except Exception:
        report['error']=traceback.format_exc();print(report['error'],flush=True)
    finally:
        try:r.release();observe('final')
        except Exception:pass
        save()
    return 0 if report['success'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

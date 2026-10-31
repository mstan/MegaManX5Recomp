"""One owned native menu/pickup/reload sequence for shared or private E.

Resources and checkpoint position are disclosed setup. Original controllers
must heal, refill, compact and carry the contents; no heal/exit flag is seeded.
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


def require(value, message):
    if not value:
        raise AssertionError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--private', action='store_true')
    args = parser.parse_args()
    parent = json.loads(args.session.read_bytes())
    exe = Path(parent['command'][0]).resolve()
    require(Path(psutil.Process(parent['pid']).exe()).resolve() == exe, 'Not owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest() == parent['executable_sha256'], 'Binary changed')
    require(bool(parent.get('separate_etanks', False)) == args.private, 'Wrong boot policy')
    r = Runtime(parent['port'])
    require(not r.request('netplay_status')['active'], 'Offline resource setup only')
    args.out.mkdir(parents=True, exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    report = dict(success=False, human_validation=False, private=args.private,
                  executable_sha256=parent['executable_sha256'], session=str(args.session.resolve()),
                  samples=[], input=[], bypasses=[
                      'Original Grizzly1/0/CP0 checkpoint bypasses the route',
                      'HP/ammo, first E capacity/contents and W capacity/contents are resource setup',
                      'Contact protection; HP refill disabled before native healing',
                      'Original items allocated at separated P2, not authored-placement admission',
                      'Original checkpoint continuation bypasses retry/exit trigger, not retention'],
                  limits='Native E menus/refill/compaction/checkpoint retention and shared W only. '
                         'No snapshot/rewind, card-energy persistence, netplay or full-route pass.')

    def save():
        (args.out/'report.json').write_bytes((json.dumps(report, indent=2)+'\n').encode())

    def sample(label):
        s = r.personal_effects()
        row = dict(label=label, state=s['state'], life=s['life'], scene=s['scene'],
                   bodies=[b.hex() for b in s['bodies']], play=r.read(PLAY, 0x1C0).hex())
        report['samples'].append(row)
        save()
        return s

    def wait(predicate, label, timeout=35):
        deadline = time.monotonic()+timeout
        while True:
            s = r.personal_effects()
            if predicate(s):
                return s
            require(time.monotonic() < deadline, label)
            time.sleep(.03)

    def frames(count):
        start = r.request('frame')['frame']
        deadline = time.monotonic()+12
        while r.request('frame')['frame'] < start+count:
            require(time.monotonic() < deadline, 'Native frontend stopped')
            time.sleep(.02)

    def pulse(button, seat):
        report['input'].append(dict(seat=seat, button=hex(button), before=r.read(PLAY, 4).hex()))
        r.input(button, seat)
        try:
            frames(6)
        finally:
            r.input(0, seat)
        frames(8)

    def grounded(s):
        p = r.read(PLAY, 0x20)
        return (p[:2] == b'\x0A\0' and p[12:14] == b'\x01\0' and not p[0x1C]
                and s['life'][:2] == [0, 0] and s['scene'] == [0, 0]
                and all(b[4:6] == b'\x01\x02' and b[0x70] & 8 for b in s['bodies']))

    def open_tank(seat, cursor=0x0A):
        pulse(8, seat)
        wait(lambda s: r.read(PLAY+1, 1) == b'\x02' and
             r.read(0x800D4F44, 1) == b'\x01' and not r.read(0x8009A424, 1)[0], 'Menu not ready')
        require(r.read(r.diagnostic()+0x48, 1)[0] == seat+1, 'Wrong menu owner')
        world = r.state()['world_ticks']
        pulse(8, seat ^ 1)
        require(r.read(PLAY+1, 1) == b'\x02' and r.state()['world_ticks'] == world,
                'Other seat resumed menu or world ran')
        if r.read(0x800D4F54, 1)[0] < 10:
            pulse(0x10, seat)  # Original Up selects E1 directly.
        for _ in range(3):
            if r.read(0x800D4F54, 1)[0] == cursor:
                break
            pulse(0x80 if cursor == 0x0A else 0x20, seat)
        require(r.read(0x800D4F54, 1)[0] == cursor and
                r.read(0x800D4F45, 1) == b'\x01', 'Wrong original tank focus')

    def use_e(seat, hp, label):
        open_tank(seat)
        r.screenshot(args.out/(label+'-menu.png'))
        pulse(0x4000, seat)
        wait(lambda s: s['bodies'][seat][0x5C] == hp and
             r.read(0x800D4F45, 1) == b'\x01' and not r.read(0x8009A424, 1)[0],
             'Native healing/compaction did not finish at expected HP')
        require(r.read(PLAY+0x78, 1) == b'\x85', 'E use changed shared W')
        sample(label)
        r.screenshot(args.out/(label+'-healed.png'))
        r.resume_native_pause()

    def pickup(subtype, label):
        r.fixture(3, seat=1, hp=subtype)
        address = int.from_bytes(r.read(r.diagnostic()+0x78, 4), 'little')
        require(address, 'Original pickup allocation failed')
        wait(lambda s: r.read(address, 1) == b'\0', 'Original item did not retire', 20)
        require(r.read(r.diagnostic()+0x74, 1) == b'\x02', 'P2 did not own pickup')
        sample(label)

    try:
        r.release()
        r.resume_native_pause()
        r.fixture(11, hp=3)
        r.fixture(12, hp=3)
        r.load_location(1, 0, 0)
        s = wait(grounded, 'Native checkpoint not grounded', 85)
        require([b[2] for b in s['bodies']] == [1, 0], 'Use native Zero/P1 and X/P2')
        require(not s['bodies'][1][0xFC] & 0x40, 'Ordinary pickup conversion requires native FC40 off')
        r.fixture(11, hp=0)
        x = struct.unpack_from('<i', s['bodies'][0], 8)[0]/65536
        y = struct.unpack_from('<i', s['bodies'][1], 12)[0]/65536
        r.teleport(1, x+80, y)
        r.fixture(2, seat=0, hp=13, ammo=0x90)
        r.fixture(2, seat=1, hp=11, ammo=0x90)
        r.fixture(4, seat=0, hp=16)
        if args.private:
            r.fixture(4, seat=1, hp=8)
        flags = int.from_bytes(r.read(PLAY+0x7E, 2), 'little') | 0x4000
        for i, byte in enumerate(struct.pack('<H', flags)):
            r.request('write_ram', addr=hex(PLAY+0x7E+i), val=hex(byte))
        r.request('write_ram', addr=hex(PLAY+0x78), val='0x85')
        sample('resource-setup')
        use_e(1, 19 if args.private else 27, 'P2-uses-own-eight' if args.private else 'P2-uses-shared-sixteen')
        require(r.players()[0][0x5C] == 13, 'P2 healed P1')
        use_e(0, 29 if args.private else 13, 'P1-private-sixteen' if args.private else 'P1-shared-already-empty')
        r.fixture(2, seat=1, hp=32, ammo=0x90)
        pickup(0x21, 'actual-fullHP-P2-refill-four')
        if args.private:
            use_e(0, 29, 'P1-empty-after-P2-refill')
            r.fixture(2, seat=1, hp=11, ammo=0x90)
            use_e(1, 15, 'P2-uses-native-four')
            pickup(0x28, 'P2-native-second-E-acquisition')
            require(int.from_bytes(r.read(PLAY+0x7E, 2), 'little') & 0x3000 == 0x3000,
                    'Native second E acquisition missing')
            r.fixture(2, seat=0, hp=13, ammo=0x90)
            r.fixture(2, seat=1, hp=32, ammo=0x90)
            r.fixture(4, seat=0, hp=16)
            r.fixture(4, seat=1, hp=32)
            pickup(0x21, 'P2-native-second-slot-refill-four')
            use_e(0, 29, 'P1-private-sixteen-with-two-slots')
            r.fixture(2, seat=1, hp=1, ammo=0x90)
            use_e(1, 32, 'P2-native-full-heal-compacts-second-four')
            # Native34240..34258 copies slot2 over used slot1 at completion,
            # including full HP. Preserve that original rule, not a guessed
            # unused remainder in the first tank.
            sample('before-native-checkpoint-continuation')
            r.load_location(1, 0, 0, continuation=True)
            wait(grounded, 'Native continuation not grounded', 85)
            sample('after-native-checkpoint-continuation')
            r.fixture(2, seat=1, hp=11, ammo=0x90)
            use_e(1, 15, 'P2-compacted-four-retained-after-reload')
            use_e(0, 29, 'P1-remains-empty')
        else:
            use_e(0, 17, 'P1-uses-P2-shared-refill-four')
            require(r.players()[1][0x5C] == 32, 'P1 tank healed P2')
        before = [b[0x5C] for b in r.players()]
        open_tank(1, 0x0C)
        pulse(0x4000, 1)
        wait(lambda s: r.read(PLAY+0x78, 1)[0] & 127 == 0 and
             r.read(0x800D4F45, 1) == b'\x01', 'Native W use did not complete')
        require([b[0x5C] for b in r.players()] == before, 'W use changed HP')
        sample('native-shared-W-spent')
        r.screenshot(args.out/'native-shared-W.png')
        r.resume_native_pause()
        report['success'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        print(report['error'], flush=True)
        try:
            sample('failure')
            r.screenshot(args.out/'failure.png')
        except Exception:
            report['capture_error'] = traceback.format_exc()
    finally:
        r.release()
        p = r.read(PLAY, 0x20)
        if p[:2] == b'\x0A\0' and not p[0x1C] and r.personal_effects()['scene'] == [0, 0]:
            r.tap(8, .15, 0)
        sample('parked')
        save()
    print(json.dumps({k: report.get(k) for k in ('success', 'private', 'error')}), flush=True)
    return 0 if report['success'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

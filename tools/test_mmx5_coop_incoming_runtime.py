"""Native P2 incoming damage before/after each owned pause menu.

Use an owned offline session already positioned near original enemies. Position
and P1 HP protection are setup only; P2 damage/immunity and enemy interaction
must run natively. No damage, contact, enemy state or success flag is injected.
"""
import argparse
import hashlib
import json
from pathlib import Path
import time
import traceback

import psutil
from coop_debug import Runtime, PLAY
from audit_mmx5_coop_locations import counter, snapshot


def require(value, message):
    if not value:
        raise AssertionError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    session = json.loads(args.session.read_text(encoding='utf-8'))
    executable = Path(session['command'][0]).resolve()
    require(Path(psutil.Process(session['pid']).exe()).resolve() == executable, 'Not owned process')
    require(hashlib.sha256(executable.read_bytes()).hexdigest() == session['executable_sha256'], 'Owned binary changed')
    runtime = Runtime(session['port'])
    require(not runtime.request('netplay_status')['active'], 'Offline fixtures only')
    args.out.mkdir(parents=True, exist_ok=False)
    source = Path(__file__).read_bytes()
    (args.out/'harness.py').write_bytes(source)
    report = dict(success=False, session=str(args.session.resolve()),
                  executable_sha256=session['executable_sha256'],
                  harness_sha256=hashlib.sha256(source).hexdigest(), cases=[],
                  bypassed_prerequisites=['native intro boot and P2 controller approach in parent evidence',
                                         'P1 HP protection only; P2 HP/contact protection disabled'],
                  limits='One roster/intro enemy encounter; other damage classes, stages and audio fidelity remain unverified.')

    def save():
        (args.out/'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')

    def wait(predicate, message, seconds=12):
        deadline = time.monotonic()+seconds
        while not predicate():
            require(time.monotonic()<deadline, message)
            time.sleep(.03)

    try:
        runtime.release()
        runtime.resume_native_pause()
        runtime.fixture(11, hp=1)
        runtime.fixture(12, hp=0)
        require(runtime.lifecycle()[:2] == (0, 0), 'Both heroes must start alive')
        report['initial'] = snapshot(runtime)
        for phase, owner in (('before-menu', None), ('after-p2-menu', 1), ('after-p1-menu', 0)):
            case = dict(name=phase, menu_owner=owner, samples=[])
            report['cases'].append(case)
            if owner is not None:
                runtime.tap(8, .10, owner)
                wait(lambda:runtime.read(PLAY+1, 1) == b'\x02', 'Owner menu did not open')
                wait(lambda:runtime.read(0x800D4F44, 1) == b'\x01', 'Menu did not become interactive')
                require(runtime.read(runtime.diagnostic()+0x48, 1)[0] == owner+1, 'Wrong pause owner')
                world = runtime.state()['world_ticks']
                runtime.tap(8, .10, owner^1)
                require(runtime.read(PLAY+1, 1) == b'\x02', 'Other seat closed owner menu')
                require(runtime.state()['world_ticks'] == world, 'Native world ran during menu')
                runtime.resume_native_pause()
                case['resumed'] = snapshot(runtime)
            before = snapshot(runtime)
            hp = before['players'][1]['hp'] & 127
            require(hp > 8, 'Insufficient P2 health for another native contact')
            before_contacts = counter(runtime, 'mmx5.coop.p2-contact')
            case['before'] = before
            runtime.request('fn_clear')
            runtime.request('fn_filter', lo='0x800317B8', hi='0x80031C28')
            runtime.input(0x20, 1)
            deadline = time.monotonic()+15
            while True:
                sample = snapshot(runtime)
                case['samples'].append(sample)
                now = sample['players'][1]['hp'] & 127
                require(sample['lifecycle'][:2] == [0, 0], 'Native contact killed a hero')
                require(sample['players'][0]['hp'] == before['players'][0]['hp'], 'P2 contact changed P1 HP')
                if now < hp:
                    break
                require(time.monotonic()<deadline, 'No original enemy damage observed')
                # Stay within this bounded authored encounter. If native hurt
                # knocks the hero back, Right can bring it toward the enemy.
                if sample['players'][1]['x'] >= 620:
                    runtime.input(0, 1)
                time.sleep(.04)
            runtime.release()
            case['after'] = snapshot(runtime)
            case['contact_delta'] = counter(runtime, 'mmx5.coop.p2-contact')-before_contacts
            case['native_contact_entries'] = runtime.request('fn_entry_dump', count=128,
                                                            addr_lo='0x800317B8', addr_hi='0x800317BC')
            case['native_contact_returns'] = runtime.request('fn_exit_dump', count=128,
                                                            addr_lo='0x800317B8', addr_hi='0x800317BC')
            case['last_contact'] = runtime.read(runtime.diagnostic()+0x68, 8).hex()
            runtime.request('fn_disable')
            require(case['contact_delta'] > 0, 'HP change lacks accepted P2 enemy-contact evidence')
            require(case['native_contact_entries']['entries'], 'Original contact function was not traced')
            require(not case['after']['state']['failed'], 'Co-op state failed')
            runtime.screenshot(args.out/(phase+'.png'))
            case['passed'] = True
            save()
            print('PASS', phase, 'P2 HP', hp, '->', case['after']['players'][1]['hp'] & 127,
                  'accepted contacts', case['contact_delta'], flush=True)
        report['success'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        print(report['error'], flush=True)
    finally:
        try:
            runtime.request('fn_disable')
            runtime.release()
            if runtime.read(PLAY+1, 1) == b'\x00':
                runtime.tap(8, .10)
        except Exception:
            report['cleanup_error'] = traceback.format_exc()
        save()
    return 0 if report['success'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

"""Controlled native continuation reloads preserve per-seat state.

Attach to an owned offline fresh-boot process. The fixture invokes the original
negative-exit controller and original mode9/init; it does not select a different
room or establish traversal/owner human acceptance. Test alive, absent and
fallen P2 separately: continuation must not refill inventory or resurrect P2.
"""
import argparse
import json
from pathlib import Path
import time

from coop_debug import Runtime, PLAY


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def wait_for(predicate, seconds=30):
    deadline = time.monotonic()+seconds
    while not predicate():
        if time.monotonic()>deadline:
            raise TimeoutError('Native continuation did not reach its expected state')
        time.sleep(.025)


def enrollments(r):
    return next(c['count'] for c in r.request('mod_counters')['counters']
                if c['name']=='mmx5.coop.enrolled')


def reload(r):
    before = enrollments(r)
    r.fixture(8)
    wait_for(lambda: r.state()['enrolled'] and enrollments(r)>before)
    require(not r.state()['failed'], 'Co-op failed during original init')
    require(r.read(PLAY, 1) == b'\x0a', 'Original continuation did not return to gameplay')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    r = Runtime(args.port)
    require(not r.request('netplay_status')['active'], 'Continuation fixtures are offline only')
    report = dict(controlled_continuation_fixture=True, human_validation=False, success=False)
    try:
        r.resume_native_pause()
        require(r.lifecycle()[:2] == (0, 0), 'Start with two living actors')
        r.fixture(2, seat=0, hp=13, ammo=121)
        r.fixture(2, seat=1, hp=17, ammo=73)
        first, second = r.players()
        report['before'] = [first.hex(), second.hex()]
        reload(r)
        a, b = r.players()
        require(a[0x5c]==13 and b[0x5c]==17, 'Native continuation refilled/copied per-seat HP')
        require(a[0xa8:0xc8]==first[0xa8:0xc8] and b[0xa8:0xc8]==second[0xa8:0xc8],
                'Native continuation refilled/copied per-seat ammo')
        report['alive'] = [a.hex(), b.hex()]
        r.screenshot(args.out/'alive.png')

        r.input(1, 1)
        wait_for(lambda: r.lifecycle()[1] == 2)
        r.input(0, 1)
        inventory = r.players()[1]
        reload(r)
        require(r.lifecycle()[1] == 2 and not r.players()[1][3], 'Continuation rejoined absent P2')
        require(r.players()[1][0x5c]==inventory[0x5c] and
                r.players()[1][0xa8:0xc8]==inventory[0xa8:0xc8], 'Absent continuation lost inventory')
        report['absent'] = r.players()[1].hex()
        r.screenshot(args.out/'absent.png')
        r.tap(1, .06, 1)
        wait_for(lambda: r.lifecycle()[1] == 0)

        r.fixture(1, seat=1)
        wait_for(lambda: r.lifecycle()[1] == 5)
        retries = r.read(PLAY+0x45, 1)[0]
        inventory = r.players()[1]
        reload(r)
        require(r.lifecycle()[1] == 5 and not r.players()[1][3], 'Continuation resurrected fallen P2')
        require(r.read(PLAY+0x45, 1)[0] == retries, 'Survivor continuation spent a team retry')
        require(r.players()[1][0xa8:0xc8]==inventory[0xa8:0xc8], 'Fallen continuation refilled ammo')
        report['fallen'] = r.players()[1].hex()
        r.screenshot(args.out/'fallen.png')
        report['success'] = True
        print('PASS native continuation: independent inventory, absent/fallen retained, no retry spent', flush=True)
    except Exception as error:
        report['error'] = str(error)
        raise
    finally:
        r.release()
        report['state'] = r.state()
        report['life'] = r.lifecycle()
        if r.lifecycle()[0] == 0 and r.read(PLAY+1, 1) == b'\x00':
            r.tap(8, .05)
        (args.out/'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')


if __name__ == '__main__':
    main()

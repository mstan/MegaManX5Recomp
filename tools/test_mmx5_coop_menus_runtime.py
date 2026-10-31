"""Native personal menus, guest Quit authority and campaign Quit/re-entry.

One owned offline session. The initial Grizzly load and protection bypass
traversal/damage; menu navigation, actual Quit, new-game selection and control
recovery use native input. No menu, campaign-success or completion flag is set.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time
import traceback

import psutil

from coop_debug import Runtime, PLAY, PLAYER
from audit_mmx5_coop_locations import counter, snapshot, wait_ready

MENU = 0x800D4F40
FADE = 0x8009A424


def require(value, message):
    if not value:
        raise AssertionError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    session = json.loads(args.session.read_text(encoding='utf-8'))
    exe = Path(session['command'][0]).resolve()
    require(Path(psutil.Process(session['pid']).exe()).resolve() == exe, 'Not owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest() == session['executable_sha256'], 'Binary changed')
    r = Runtime(session['port'])
    require(not r.request('netplay_status')['active'], 'Offline fixture required')
    args.out.mkdir(parents=True, exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    report = dict(success=False, human_validation=False, steps=[], checks=[],
                  session=str(args.session.resolve()), executable_sha256=session['executable_sha256'],
                  source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  source_evidence='34A84 native menu Select;21B2C confirmation/fade gate;211C4 cursor;21CC4 native Quit reset',
                  bypassed_prerequisites=['native Grizzly checkpoint1 setup',
                                          'HP refill/contact protection during setup and menu/control checks',
                                          'nonfull per-seat resources seeded before initial menus'],
                  limits='Native menu authority/art and Quit/new-game lifecycle; unlocked special weapons, saves, damage and network play remain unverified. Art/suppression requires composed-image review.')

    def save():
        (args.out/'report.json').write_bytes((json.dumps(report, indent=2)+'\n').encode('utf-8'))

    def wait(predicate, message, seconds=40):
        deadline = time.monotonic()+seconds
        while not predicate():
            require(time.monotonic() < deadline, message)
            time.sleep(.03)

    def frames(count):
        start = r.request('frame')['frame']
        wait(lambda: r.request('frame')['frame'] >= start+count, 'Native frames stopped', 20)

    def pulse(button, seat=0):
        r.input(button, seat)
        frames(6)
        r.input(0, seat)
        frames(8)

    def observe(label):
        report['steps'].append(dict(label=label, snapshot=snapshot(r),
                                   play=r.read(PLAY, 0x80).hex(), menu=r.read(MENU, 0x60).hex(),
                                   fade=r.read(FADE-4, 16).hex()))
        save()
        r.screenshot(args.out/(label+'.png'))
        print(label, flush=True)

    def grounded():
        return r.lifecycle()[:2] == (0, 0) and all(
            b[4:6] == b'\x01\x02' and b[0x70] & 8 for b in r.players())

    def open_menu(owner, label):
        r.release()
        wait(grounded, 'Heroes did not settle before menu')
        r.input(8, owner)
        wait(lambda: r.read(PLAY+1, 1) == b'\x02', 'Native menu did not open')
        r.input(0, owner)
        wait(lambda: r.read(MENU+4, 1) == b'\x01' and r.read(FADE, 1) == b'\x00',
             'Native menu did not become interactive')
        require(r.read(r.diagnostic()+0x48, 1)[0] == owner+1, 'Wrong native menu owner')
        before = r.state()['world_ticks']
        pulse(8, owner ^ 1)
        require(r.read(PLAY+1, 1) == b'\x02', 'Other seat resumed menu')
        require(r.state()['world_ticks'] == before, 'World ran during menu')
        observe(label)

    def guest_quit_blocked(label):
        pulse(1, 1)
        require(r.read(PLAY+1, 1) == b'\x02' and r.read(MENU+5, 1)[0] < 3,
                'Guest Select reached the campaign Quit page')
        require(r.read(r.diagnostic()+0x48, 1) == b'\x02', 'Guest Select stole menu ownership')
        observe(label)
        report['checks'].append(label)

    def close_menu(owner):
        r.input(8, owner)
        wait(lambda: r.read(PLAY+1, 1) == b'\x00', 'Native menu did not close')
        r.input(0, owner)
        frames(8)

    def move(owner, label):
        wait(grounded, 'Heroes did not settle after menu')
        before = r.players()
        x = struct.unpack_from('<i', before[owner], 8)[0]
        other = struct.unpack_from('<i', before[owner ^ 1], 8)[0]
        r.input(0x20, owner)
        wait(lambda: struct.unpack_from('<i', r.players()[owner], 8)[0] > x+4*65536,
             'Menu owner did not recover native Right')
        r.input(0, owner)
        frames(8)
        require(struct.unpack_from('<i', r.players()[owner ^ 1], 8)[0] == other,
                'Menu owner movement displaced the other seat')
        observe(label)

    try:
        r.resume_native_pause()
        r.release()
        r.fixture(11, hp=3)
        r.fixture(12, hp=3)
        old = counter(r, 'mmx5.coop.enrolled')
        r.load_location(1, 0, 1)
        wait_ready(r, old)
        wait(grounded, 'Native Grizzly setup did not settle')
        initial_roster = [b[2] for b in r.players()]
        require(sorted(initial_roster) == [0, 1], 'Missing original character')
        report['initial_roster'] = initial_roster
        r.fixture(11, hp=0)
        r.fixture(2, seat=0, hp=13, ammo=73)
        r.fixture(2, seat=1, hp=11, ammo=91)
        resources = [(b[0x5C], b[0xA8:0xC8]) for b in r.players()]
        for owner in (0, 1):
            open_menu(owner, f'initial-p{owner+1}-menu')
            if owner == 1:
                guest_quit_blocked('initial-guest-quit-blocked')
            close_menu(owner)
            require([b[2] for b in r.players()] == initial_roster, 'Menu changed roster')
            require([(b[0x5C], b[0xA8:0xC8]) for b in r.players()] == resources,
                    'Menu changed either seat resources')
            move(owner, f'initial-p{owner+1}-control-recovered')
        report['checks'].append('both personal menus reject wrong-seat resume and preserve roster/resources')

        open_menu(0, 'p1-menu-before-quit')
        r.input(1, 0)
        wait(lambda: r.read(MENU+5, 1) == b'\x06', 'Campaign owner cannot enter native Quit')
        r.input(0, 0)
        wait(lambda: r.read(MENU+6, 2) == b'\x02\x01' and r.read(FADE, 1) == b'\x00',
             'First native Quit prompt did not become interactive')
        require(r.read(MENU+0x34, 4) == b'\x00'*4, 'Wrong initial native confirmation')
        observe('p1-continue-prompt')
        if r.read(FADE+3, 1) == b'\x00':
            pulse(0x40)
        require(r.read(FADE+3, 1) == b'\x01', 'Continue No cursor did not move natively')
        pulse(0x4000)
        wait(lambda: r.read(MENU+0x34, 4) == b'\x01\x00\x00\x00' and
             r.read(FADE, 1) == b'\x00', 'Second native Quit prompt did not become interactive')
        observe('p1-quit-prompt')
        if r.read(FADE+3, 1) == b'\x01':
            pulse(0x10)
        require(r.read(FADE+3, 1) == b'\x00', 'Quit Yes cursor did not move natively')
        pulse(0x4000)
        wait(lambda: r.read(PLAY, 1)[0] != 10, 'Campaign owner native Quit did not leave gameplay', 60)
        observe('native-title-after-p1-quit')
        report['checks'].append('P1 retains actual native Quit through both confirmation pages')

        deadline = time.monotonic()+90
        while r.read(PLAY, 2) != b'\x01\x03':
            require(time.monotonic() < deadline, 'Native new-game selector did not open')
            mode = r.read(PLAY, 1)[0]
            if mode != 1:
                pulse(0x4000 if mode == 20 else 8)
            else:
                time.sleep(.03)
        observe('native-new-game-select')
        # Choose the other campaign after native reset. This changes real
        # character/menu resources and exercises a meaningful lifecycle path.
        if initial_roster[0] == 0:
            pulse(0x20)
        pulse(0x4000)
        deadline = time.monotonic()+180
        while True:
            play = r.read(PLAY, 0x20)
            body = r.read(PLAYER, 0x158)
            if play[0] == 10 and body[0] and body[4:6] == b'\x01\x02' and not play[0x1C]:
                break
            require(time.monotonic() < deadline, 'Native intro after Quit/re-entry did not recover')
            if play[0] != 10 or play[0x1C]:
                pulse(0x4000)
            else:
                r.release()
                time.sleep(.04)
        r.fixture(11, hp=3)
        r.fixture(12, hp=3)
        wait(grounded, 'Partner did not settle after native re-entry', 60)
        expected = [initial_roster[0] ^ 1, initial_roster[1] ^ 1]
        require([b[2] for b in r.players()] == expected, 'Quit/re-entry lost campaign or counterpart identity')
        report['reentry_roster'] = expected
        observe('native-intro-reentered')
        open_menu(1, 'p2-menu-after-reentry')
        guest_quit_blocked('reentry-guest-quit-blocked')
        close_menu(1)
        move(1, 'p2-control-after-reentry')
        require([b[2] for b in r.players()] == expected, 'Reentered guest menu promoted its character')
        require(not r.state()['failed'], 'Co-op failed after re-entry')
        report['checks'].append('native new game changes campaign; P2 owns/resumes its new character menu and moves independently')
        report['render_stats'] = r.request('render_pass_stats')
        require(report['render_stats']['verify_checks'] > 0, 'Private redraw verifier did not run')
        require(all(not report['render_stats'].get(k, 0) for k in
                    ('verify_mismatch', 'watchdog', 'device_reads', 'vram_leaks')), 'Private redraw fault')
        require(counter(r, 'mmx5.coop.local-view-state-leak') == 0, 'Private draw changed co-op state')
        report['success'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        print(report['error'], flush=True)
        try:
            observe('failure')
        except Exception:
            report['capture_error'] = traceback.format_exc()
    finally:
        try:
            r.release()
            if r.read(PLAY, 2) == b'\x0a\x00':
                r.fixture(12, hp=0)
                r.input(8, 0)
                wait(lambda: r.read(PLAY+1, 1) == b'\x02', 'Could not park owned session')
                r.input(0, 0)
        except Exception:
            report['cleanup_error'] = traceback.format_exc()
        save()
    print(json.dumps({k: report.get(k) for k in ('success', 'checks', 'error', 'cleanup_error')}), flush=True)
    return 0 if report['success'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

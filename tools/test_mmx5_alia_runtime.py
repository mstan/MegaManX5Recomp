"""A/B the original intro-road Alia hint; position fixtures are offline only."""
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


def require(value, reason):
    if not value:
        raise AssertionError(reason)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', required=True, type=Path)
    parser.add_argument('--out', required=True, type=Path)
    parser.add_argument('--original-hints', action='store_true')
    args = parser.parse_args()
    session = json.loads(args.session.read_text(encoding='utf-8'))
    exe = Path(session['command'][0]).resolve()
    require(Path(psutil.Process(session['pid']).exe()).resolve() == exe, 'Not owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest() == session['executable_sha256'], 'Binary changed')
    require(session['skip_alia_hints'] == (not args.original_hints), 'Wrong offline feature selection')
    r = Runtime(session['port'])
    require(not r.request('netplay_status')['active'], 'Offline position fixtures only')
    args.out.mkdir(parents=True, exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    report = dict(success=False, session=str(args.session.resolve()),
                  executable_sha256=session['executable_sha256'],
                  harness_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  original_hints=args.original_hints, samples=[],
                  bypassed_prerequisites=['Original intro checkpoint0 loader',
                    'Position/history-only teleports near original hint4 rectangle',
                    'HP/contact protection during setup'],
                  limits='Hint4 on the intro road only; story/capsule/reward behavior needs separate coverage.')

    def save():
        (args.out/'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')

    def observe(label):
        s = r.personal_effects()
        row = dict(label=label, life=s['life'], scene=s['scene'], state=s['state'],
                   bodies=[b.hex() for b in s['bodies']], play=r.read(PLAY, 0x110).hex(),
                   native_hint_pool=r.read(0x8009E7A0, 96*0x30).hex(),
                   native_camera=r.read(0x8009A1F8, 0x54).hex(),
                   hidden_text=counter(r, 'mmx5.skip-alia.suppressed-text'))
        report['samples'].append(row)
        save()
        return row

    def ticks(count):
        start = r.state()['world_ticks']
        deadline = time.monotonic()+25
        while r.state()['world_ticks'] < start+count:
            require(time.monotonic() < deadline, 'World stopped advancing')
            time.sleep(.02)

    try:
        r.release()
        r.resume_native_pause()
        r.fixture(11, hp=3)
        r.fixture(12, hp=3)
        previous = counter(r, 'mmx5.coop.enrolled')
        r.load_location(0, 0, 0)
        wait_ready(r, previous)
        r.release()
        ticks(30)
        before = observe('before-original-hint4')
        char = bytes.fromhex(before['bodies'][0])[2]
        hint_byte = PLAY+0xCD+char*32
        require(not r.read(hint_byte, 1)[0]&16, 'Hint4 already seen; control is invalid')
        resources = [bytes.fromhex(b)[0xA8:0xC8] for b in before['bodies']]
        retries, flags = r.read(PLAY+0x45, 1), r.read(PLAY+0x4C, 8)
        r.teleport(1, 1040, 976)
        r.teleport(0, 1064, 976)
        ticks(15)
        # Teleporting bypasses the camera's ordinary approach. Wait for the
        # native view/placement scan to catch up before crossing the narrow
        # hint rectangle; do not write a camera, visit bit or trigger flag.
        deadline = time.monotonic()+45
        while True:
            if session['local_cameras'] == 'split':
                camera = r.read(r.diagnostic()+0x710, 0x54)
            else:
                camera = r.read(0x8009A1F8, 0x54)
            if struct.unpack_from('<h', camera, 10)[0]+320 > 1088:
                break
            require(time.monotonic() < deadline, 'Original camera did not reach hint4 approach')
            time.sleep(.03)
        ticks(4)
        if args.original_hints:
            pool = r.read(0x8009E7A0, 96*0x30)
            require(any(pool[i] and pool[i+1] == 14 and pool[i+2] == 4
                        for i in range(0, len(pool), 0x30)), 'Original hint4 actor did not activate')
        approach = observe('native-approach')
        r.input(0x20, 0)
        deadline = time.monotonic()+20
        while True:
            play = r.read(PLAY, 0x20)
            body = r.players()[0]
            if args.original_hints:
                if play[0x1C] and body[0xD0]:
                    break
            elif struct.unpack_from('<i', body, 8)[0] > 1100*65536:
                break
            require(time.monotonic() < deadline, 'Original hint rectangle was not reached')
            time.sleep(.02)
        r.release()
        reached = observe('original-hint4-rectangle')
        r.screenshot(args.out/'hint4.png')
        if args.original_hints:
            require(r.read(hint_byte, 1)[0]&16, 'Native hint4 did not record its per-character bit')
            require(reached['hidden_text'] == 0, 'Disabled mod still suppressed hint text')
            # Retain the real hint box and stand-lock as the A/B witness.
            report['native_message_lock_observed'] = True
        else:
            require(reached['hidden_text'] > before['hidden_text'], 'Native hint text was not suppressed')
            require(r.read(hint_byte, 1)[0]&16, 'Silent hint lost the native acknowledgement required by stage scripts')
            require(reached['scene'] == [0, 0] and not bytes.fromhex(reached['play'])[0x1C],
                    'Suppression left a shared script lock')
            require(reached['life'][:2] == [0, 0], 'Hint suppression changed hero availability')
            for seat in (0, 1):
                x = struct.unpack_from('<i', r.players()[seat], 8)[0]
                r.input(0x20, seat)
                ticks(8)
                r.release()
                require(struct.unpack_from('<i', r.players()[seat], 8)[0] > x,
                        'Hero lost native movement through skipped hint')
            require(not r.state()['failed'], 'Co-op failed during hint suppression')
            require(r.read(PLAY+0x45, 1) == retries and r.read(PLAY+0x4C, 8) == flags,
                    'Hint suppression changed retry/boss outcome state')
            require([b[0xA8:0xC8] for b in r.players()] == resources,
                    'Hint suppression changed private inventory')
        report['render_stats'] = stats = r.request('render_pass_stats')
        if session.get('render_verify') and session['local_cameras'] == 'split':
            require(stats.get('verify_checks', 0) > 0, 'No private view restoration checks')
        require(all(not stats.get(key, 0) for key in
                    ('verify_mismatch', 'watchdog', 'vram_leaks', 'device_reads')),
                'Rendering fault during hint check')
        report['success'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        raise
    finally:
        try:
            r.release()
        finally:
            save()
    print('PASS original Alia hint control' if args.original_hints else 'PASS default Alia hint suppression')


if __name__ == '__main__':
    main()

"""Original-disc fresh-boot input check with private saves and an owned process.

No gameplay RAM, savestates or stage fixtures are written. Captures and reports
stay in the caller's ignored run directory. This does not certify a campaign.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import socket
import struct
import subprocess
import time
import traceback
from coop_debug import Runtime, PLAY, PLAYER


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--game', type=Path, required=True)
    parser.add_argument('--disc', type=Path, required=True)
    parser.add_argument('--bios', type=Path, required=True)
    parser.add_argument('--run-dir', type=Path, required=True)
    parser.add_argument('--character', choices=('x', 'zero'), default='x')
    parser.add_argument('--enabled', action='store_true')
    parser.add_argument('--keep-running', action='store_true')
    parser.add_argument('--paced', action='store_true',
                        help='Use a hidden normally paced GL frontend for reliable native menu observation')
    args = parser.parse_args()
    args.run_dir.mkdir(parents=True, exist_ok=True)
    build = args.exe.resolve().parent
    state = build/'mods/state.toml'
    old_state = state.read_bytes() if state.exists() else None
    selection = 'format_version = 2\n'
    if args.enabled:
        selection += '\n[[package]]\nid="mmx5.coop"\n\n[[feature]]\npackage_id="mmx5.coop"\nid="coop"\nenabled=true\n'
    state.write_text(selection, encoding='utf-8')
    with socket.socket() as reserve:
        reserve.bind(('127.0.0.1', 0))
        port = reserve.getsockname()[1]
    runtime = Runtime(port)
    env = os.environ.copy()
    env['PATH'] = 'C:/msys64/mingw64/bin;'+env['PATH']
    env['PSX_DEV_INPUT'] = '0'
    if args.paced:
        env['SDL_AUDIODRIVER'] = 'dummy'
    log = (args.run_dir/'runtime.log').open('w', encoding='utf-8')
    command = [str(args.exe.resolve()), '--hidden-window' if args.paced else '--headless-opengl',
               '--renderer', 'opengl', '--no-launcher',
               '--game', str(args.game.resolve()), '--disc', str(args.disc.resolve()),
               '--bios', str(args.bios.resolve()), '--debug-port', str(port),
               '--memcard-dir', str((args.run_dir/'saves').resolve())]
    process = subprocess.Popen(command, cwd=build, env=env, stdout=log,
                               stderr=subprocess.STDOUT,
                               creationflags=subprocess.CREATE_NO_WINDOW if os.name=='nt' else 0)
    report = dict(pid=process.pid, port=port, command=command, character=args.character,
                  enabled=args.enabled, human_validation=False,
                  executable_sha256=hashlib.sha256(args.exe.read_bytes()).hexdigest())
    print('Owned X5 QA process', process.pid, 'port', port, flush=True)
    success = False
    try:
        deadline = time.monotonic()+35
        while True:
            require(process.poll() is None, 'Runtime exited during boot')
            try:
                runtime.request('frame')
                break
            except OSError:
                require(time.monotonic()<deadline, 'Debug startup timeout')
                time.sleep(.2)
        # Skip native opening movies and confirm Game Start. Character select
        # is original PLAY mode1/substate3; no menu/progression flags are forced.
        deadline = time.monotonic()+50
        already_confirmed = False
        while runtime.read(PLAY, 2) != b'\x01\x03':
            require(time.monotonic()<deadline, 'Native character selection timeout')
            mode = runtime.read(PLAY, 2)
            if mode[0] in (8, 9, 10):
                # An unpaced frame can accept the default character before a
                # TCP observation sees the selector. Verify the actual native
                # body below; do not force a menu/state back into existence.
                already_confirmed = True
                break
            runtime.tap(8, .015)
            time.sleep(.04)
            if runtime.read(PLAY, 2) == b'\x01\x03':
                break
            runtime.tap(0x4000, .015)
            time.sleep(.04)
        if args.character == 'zero' and not already_confirmed:
            runtime.tap(0x20, .10)
            time.sleep(.2)
        runtime.screenshot(args.run_dir/('selection-already-confirmed.png' if already_confirmed else 'character-select.png'))
        report['selector_observed'] = not already_confirmed
        if not already_confirmed:
            runtime.tap(0x4000, .10)
        deadline = time.monotonic()+90
        sample_at = 0
        while True:
            play = runtime.read(PLAY, 0x20)
            body = runtime.read(PLAYER, 0x158)
            if play[0]==10 and body[0] and body[4]==1 and body[5]==2 and not play[0x1C]:
                break
            require(time.monotonic()<deadline, 'Native intro dialogue/control timeout')
            runtime.tap(0x4000, .06)
            time.sleep(.2)
            if time.monotonic()>sample_at:
                print('Native intro', play[:4].hex(), body[:8].hex(), flush=True)
                sample_at = time.monotonic()+10
        runtime.input(0, 0)
        runtime.input(0, 1)
        time.sleep(.2)
        runtime.screenshot(args.run_dir/'idle.png')
        if args.enabled:
            require(runtime.state()['enrolled'], 'Counterpart did not enroll')
            require(not runtime.state()['failed'], 'Co-op resource/render failure')
            before = runtime.state()
            time.sleep(.2)
            after = runtime.state()
            require(after['ticks']-before['ticks']>5, 'Counterpart pass did not advance')
            require(after['ticks']-before['ticks']==after['world_ticks']-before['world_ticks'],
                    'Counterpart and once-per-world boundaries disagree')
            report['tick_check'] = [before, after]
            actors = runtime.players()
            expected = 1 if args.character=='zero' else 0
            require(actors[0][2]==expected and actors[1][2]==(expected^1),
                    'Native selection did not produce the requested X/Zero pair')
            require(actors[1][0xFA]==(0 if expected else 5),
                    'Counterpart inherited the other character roster/parts entry')
            for seat in (1, 0):
                a = runtime.players()
                runtime.input(0x20, seat)
                time.sleep(.15)
                runtime.input(0, seat)
                b = runtime.players()
                require(struct.unpack_from('<i', b[seat], 8)[0]>struct.unpack_from('<i', a[seat], 8)[0],
                        f'P{seat+1} did not move right')
                require(struct.unpack_from('<i', b[seat^1], 8)[0]==struct.unpack_from('<i', a[seat^1], 8)[0],
                        f'P{seat+1} input moved the other player')
                runtime.tap(0x8000, .10, seat)
                runtime.screenshot(args.run_dir/f'p{seat+1}-attack.png')
            report['actors_before'] = [actor.hex() for actor in actors]
            report['actors_after'] = [actor.hex() for actor in runtime.players()]
            report['co_op_state'] = runtime.state()
        else:
            counters = runtime.request('mod_counters')['counters']
            require(not any(c['name']=='mmx5.coop.activated' for c in counters), 'Disabled plugin activated')
            before = runtime.read(PLAYER, 0x158)
            runtime.input(0x20)
            time.sleep(.2)
            runtime.input(0)
            after = runtime.read(PLAYER, 0x158)
            require(struct.unpack_from('<i', after, 8)[0]>struct.unpack_from('<i', before, 8)[0],
                    'Single-player right input did not move')
            runtime.tap(0x8000)
            runtime.tap(0x4000)
            runtime.screenshot(args.run_dir/'single-player-action.png')
        report['counters'] = runtime.request('mod_counters')['counters']
        success = True
        if args.keep_running:
            # Park in the native pause menu while the next tool inspects files.
            # Unpaced hidden runs otherwise keep taking enemy damage at idle.
            runtime.tap(8, .05)
            time.sleep(.2)
            report['parked_in_native_pause'] = runtime.read(PLAY+1,1)==b'\x02'
    except Exception:
        report['error'] = traceback.format_exc()
        print(report['error'], flush=True)
        try:
            runtime.screenshot(args.run_dir/'failure.png')
            report['native_play'] = runtime.read(PLAY, 0x80).hex()
            if args.enabled:
                report['co_op_state'] = runtime.state()
                report['actors'] = [body.hex() for body in runtime.players()]
        except Exception:
            pass
    finally:
        try:
            runtime.release()
        except Exception:
            pass
        if not args.keep_running:
            try:
                runtime.request('quit')
            except Exception:
                pass
            try:
                process.wait(timeout=8)
            except subprocess.TimeoutExpired:
                process.terminate()
                process.wait(timeout=8)
        report['success'] = success
        report['exit_code'] = process.poll()
        if old_state is None:
            state.unlink(missing_ok=True)
        else:
            state.write_bytes(old_state)
        log.close()
        (args.run_dir/'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
        print('PASS' if success else 'FAIL', args.run_dir, flush=True)
    return 0 if success else 1


if __name__ == '__main__':
    raise SystemExit(main())

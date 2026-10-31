"""Original-disc fresh-boot input check with private saves and an owned process.

Boot and controls use native input. Optional audit setup explicitly refills HP;
its report marks the bypass and skips control assertions. Captures and reports
stay in the caller's ignored run directory. This does not certify a campaign.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import shutil
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
    parser.add_argument('--startup-timeout', type=int, default=90,
                        help='Host seconds to wait for the debug endpoint; diagnostic overrides do not change the game watchdog')
    parser.add_argument('--character', choices=('x', 'zero'), default='x')
    parser.add_argument('--ultimate-code', action='store_true',
                        help='Enter the original X title code with native pad edges; cheat, not acquisition')
    parser.add_argument('--ultimate-armor-x',action='store_true',
                        help='Enable automatic native Ultimate X in co-op, or its standalone package when co-op is off')
    parser.add_argument('--black-zero',action='store_true',
                        help='Enable native Black Zero in co-op, or its standalone package when co-op is off')
    parser.add_argument('--hud-layout', choices=('stacked','side_by_side'), default='stacked')
    parser.add_argument('--local-cameras',choices=('unified','split'),default='unified')
    parser.add_argument('--door-players',choices=('either','both'),default='either')
    parser.add_argument('--separate-etanks',action='store_true',
                        help='Enable the optional private E energy policy; acquisition and W remain shared')
    parser.add_argument('--render-verify',action='store_true',help='Verify private draw transactions restore machine state')
    parser.add_argument('--renderer-off',action='store_true',help='Disable standalone renderer preference to test the local-split requirement')
    parser.add_argument('--cameras',choices=('unified','split'),default='unified',
                        help='Offline must remain Unified even when saved preference is Split')
    parser.add_argument('--enabled', action='store_true')
    parser.add_argument('--keep-running', action='store_true')
    parser.add_argument('--paced', action='store_true',
                        help='Use a hidden normally paced GL frontend for reliable native menu observation')
    parser.add_argument('--resident-off', action='store_true',
                        help='Disable resident archive loading for an explicit diagnostic control run')
    parser.add_argument('--keep-alia-hints', action='store_true',
                        help='Disable default hint suppression for an original-message control run')
    parser.add_argument('--capture-arrival',action='store_true',
                        help='Record the native counterpart arrival before testing controls')
    parser.add_argument('--protect-fixture',action='store_true',
                        help='Private cheat build: refill HP during setup; does not prevent native pit/crush deaths')
    parser.add_argument('--prepare-audit',action='store_true',
                        help='Prepare and park an owned audit session without claiming exposed intro control checks')
    args = parser.parse_args()
    if args.ultimate_code and args.character != 'x':
        parser.error('--ultimate-code requires --character x')
    if args.startup_timeout <= 0:
        parser.error('--startup-timeout must be positive')
    if os.name=='nt' and any(p.resolve().drive.upper()=='C:' for p in (args.exe,args.run_dir)):
        parser.error('Runtime and generated QA data must use non-C: storage')
    for label,path in (('Executable',args.exe),('Game config',args.game),
                       ('Disc',args.disc),('BIOS image',args.bios)):
        require(path.is_file(),f'{label} does not exist: {path}')
    if args.prepare_audit and not (args.enabled and args.keep_running and args.protect_fixture):
        parser.error('--prepare-audit requires --enabled --keep-running --protect-fixture')
    args.run_dir.mkdir(parents=True, exist_ok=True)
    harness_source = Path(__file__).read_bytes()
    (args.run_dir/'harness.py').write_bytes(harness_source)
    executable_sha256 = hashlib.sha256(args.exe.read_bytes()).hexdigest()
    build = args.exe.resolve().parent
    state = build/'mods/state.toml'
    old_state = state.read_bytes() if state.exists() else None
    selection = 'format_version = 2\n'
    if args.enabled:
        selection += '\n[[package]]\nid="mmx5.coop"\n\n[[feature]]\npackage_id="mmx5.coop"\nid="coop"\nenabled=true\n'
        selection += f'[feature.values]\ncameras="{args.cameras}"\nhud_layout="{args.hud_layout}"\nlocal_cameras="{args.local_cameras}"\ndoor_players="{args.door_players}"\n'
        selection += f'separate_etanks="{str(args.separate_etanks).lower()}"\n'
        selection += f'ultimate_armor_x="{str(args.ultimate_armor_x).lower()}"\nblack_zero="{str(args.black_zero).lower()}"\n'
    else:
        for enabled,package,feature in ((args.ultimate_armor_x,'mmx5.cheat.ultimate-armor','ultimate-armor'),
                                        (args.black_zero,'mmx5.cheat.black-zero','black-zero')):
            if enabled:
                selection += f'\n[[package]]\nid="{package}"\n\n[[feature]]\npackage_id="{package}"\nid="{feature}"\nenabled=true\n'
    if args.renderer_off:
        selection += '\n[[feature]]\npackage_id="mmx5.enhancement.widescreen"\nid="widescreen"\nenabled=false\n'
    if args.resident_off:
        selection += '\n[[feature]]\npackage_id="mmx5.enhancement.resident-loading"\nid="resident-loading"\nenabled=false\n'
    if args.keep_alia_hints:
        selection += '\n[[feature]]\npackage_id="mmx5.enhancement.skip-alia"\nid="skip-alia"\nenabled=false\n'
    state.write_text(selection, encoding='utf-8')
    with socket.socket() as reserve:
        reserve.bind(('127.0.0.1', 0))
        port = reserve.getsockname()[1]
    runtime = Runtime(port)
    env = os.environ.copy()
    env['PATH'] = 'C:/msys64/mingw64/bin;'+env['PATH']
    env['PSX_DEV_INPUT'] = '0'
    env['SDL_AUDIODRIVER'] = 'dummy'
    if args.render_verify:env['PSX_RENDER_PASS_VERIFY']='1'
    log = (args.run_dir/'runtime.log').open('w', encoding='utf-8')
    command = [str(args.exe.resolve()), '--hidden-window' if args.paced else '--headless-opengl',
               '--renderer', 'opengl', '--no-launcher',
               '--game', str(args.game.resolve()), '--disc', str(args.disc.resolve()),
               '--bios', str(args.bios.resolve()), '--debug-port', str(port),
               '--memcard-dir', str((args.run_dir/'saves').resolve())]
    process_start_wall = time.time()
    process = subprocess.Popen(command, cwd=build, env=env, stdout=log,
                               stderr=subprocess.STDOUT,
                               creationflags=subprocess.CREATE_NO_WINDOW if os.name=='nt' else 0)
    report = dict(pid=process.pid, port=port, command=command, character=args.character,
                  enabled=args.enabled,camera_preference=args.cameras,local_cameras=args.local_cameras,door_players=args.door_players,
                  ultimate_armor_x=args.ultimate_armor_x,black_zero=args.black_zero,ultimate_code=args.ultimate_code,
                  hud_layout=args.hud_layout,render_verify=args.render_verify,renderer_preference_off=args.renderer_off,
                  separate_etanks=args.separate_etanks,
                  human_validation=False,
                  resident_loading=not args.resident_off,
                  skip_alia_hints=not args.keep_alia_hints,
                  executable_sha256=executable_sha256,
                  harness_sha256=hashlib.sha256(harness_source).hexdigest(),
                  startup_timeout_seconds=args.startup_timeout)
    print('Owned X5 QA process', process.pid, 'port', port, flush=True)
    startup_begin=time.monotonic()
    success = False
    try:
        deadline = startup_begin+args.startup_timeout
        while True:
            require(process.poll() is None, 'Runtime exited during boot')
            try:
                runtime.request('frame')
                report['startup_seconds']=time.monotonic()-startup_begin
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
            if mode[0] == 1:
                # Character selection owns mode1, including opening fades.
                # Repeated broad Start/Cross pulses can confirm default X
                # before the driver observes the interactive selector.
                time.sleep(.02)
            elif mode[0] == 20:
                runtime.tap(0x4000, .06)
                time.sleep(.20)
            else:
                runtime.tap(8, .06)
                time.sleep(.08)
        if args.character == 'zero' and not already_confirmed:
            runtime.tap(0x20, .10)
            time.sleep(.2)
        runtime.screenshot(args.run_dir/('selection-already-confirmed.png' if already_confirmed else 'character-select.png'))
        report['selector_observed'] = not already_confirmed
        if args.ultimate_code:
            require(not already_confirmed and runtime.read(PLAY,2)==bytes((1,3)),
                    'Original code requires the actual interactive character selector')
            require(int.from_bytes(runtime.read(0x800EE970,4),'little')==27 and
                    int.from_bytes(runtime.read(0x800EEB60,4),'little')==0x27BDFFE0,
                    'Original BIN23 code handler is not resident')
            require(int.from_bytes(runtime.read(0x800EF894,4),'little')==0,'Native code counter not clear')
            report['original_native_code']=dict(character='x',edges=[],
                provenance='Actual original NewGame code input on private cards; CHEAT, not capsule acquisition')
            for step,button in enumerate([0x10]*2+[0x40]*9,1):
                runtime.tap(button,.07)
                released=runtime.request('frame')['frame'];code_deadline=time.monotonic()+3
                while runtime.request('frame')['frame']<released+3:
                    require(time.monotonic()<code_deadline,'Native code release did not advance')
                    time.sleep(.01)
                actual=int.from_bytes(runtime.read(0x800EF894,4),'little')
                report['original_native_code']['edges'].append(dict(step=step,button=hex(button),counter=actual))
                require(actual==step,'Native Ultimate code edge was not accepted')
            runtime.screenshot(args.run_dir/'native-code-counter11.png')
        if not already_confirmed:
            runtime.tap(0x4000, .10)
        deadline = time.monotonic()+(180 if args.render_verify else 90)
        sample_at = 0
        while True:
            play = runtime.read(PLAY, 0x20)
            body = runtime.read(PLAYER, 0x158)
            if play[0]==10 and body[0] and body[4]==1 and body[5]==2 and not play[0x1C]:
                break
            require(time.monotonic()<deadline, 'Native intro dialogue/control timeout')
            # Confirm native intro gates; after release, allow an existing
            # jump to land instead of creating another Cross edge each poll.
            if play[0] != 10 or play[0x1C]:runtime.tap(0x4000, .06)
            else:runtime.release()
            time.sleep(.2)
            if time.monotonic()>sample_at:
                print('Native intro', play[:4].hex(), body[:8].hex(), flush=True)
                sample_at = time.monotonic()+10
        runtime.input(0, 0)
        runtime.input(0, 1)
        if args.enabled:
            split_active=any(c['name']=='mmx5.coop.split-state' for c in runtime.request('mod_counters')['counters'])
            require(split_active==(args.local_cameras=='split'),
                    'Offline display does not match its local setting')
            deadline=time.monotonic()+(60 if args.render_verify else 12)
            arrival=[]
            while runtime.lifecycle()[1]!=0:
                require(time.monotonic()<deadline,'Counterpart native arrival did not finish')
                if args.capture_arrival:
                    arrival.append(dict(state=runtime.state(),life=runtime.lifecycle(),
                                        body=runtime.players()[1].hex()))
                    runtime.screenshot(args.run_dir/f'arrival-{len(arrival):02}.png')
                time.sleep(.04)
            if args.capture_arrival:report['arrival']=arrival
        time.sleep(.2)
        runtime.screenshot(args.run_dir/'idle.png')
        if args.enabled:
            require(runtime.state()['enrolled'], 'Counterpart did not enroll')
            require(not runtime.state()['failed'], 'Co-op resource/render failure')
            if args.protect_fixture:
                runtime.fixture(11,hp=3)
                require(int.from_bytes(runtime.read(runtime.diagnostic()+0x64,4),'little')==1,
                        'This build has no qualification HP protection')
                report['bypassed_prerequisites']=['incoming damage replenished by explicit setup fixture']
            before = runtime.state()
            deadline=time.monotonic()+12
            while True:
                time.sleep(.05);after=runtime.state()
                if after['ticks']-before['ticks']>=8:break
                require(time.monotonic()<deadline,'Counterpart stopped at the gameplay boundary')
            require(after['ticks']-before['ticks']>5, 'Counterpart pass did not advance')
            require(after['ticks']-before['ticks']==after['world_ticks']-before['world_ticks'],
                    'Counterpart and once-per-world boundaries disagree')
            report['tick_check'] = [before, after]
            actors = runtime.players()
            expected = 1 if args.character=='zero' else 0
            require(actors[0][2]==expected and actors[1][2]==(expected^1),
                    'Native selection did not produce the requested X/Zero pair')
            require(actors[1][0xFA]==((4 if args.ultimate_armor_x else 0) if expected else 5),
                    'Counterpart inherited the other character roster/parts entry')
            if args.prepare_audit:
                report['setup_only'] = True
                report['independent_controls'] = 'unverified; audit setup skips exposed intro movement'
            for seat in (() if args.prepare_audit else (1, 0)):
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
        # The runtime writes postmortems relative to its build directory.
        # Preserve this process's files before another owned run replaces them.
        if report['exit_code'] is not None:
            report['postmortems'] = []
            try:
                for name in ('psx_last_run_report.json', 'starvation_dump.jsonl',
                             'psx_freeze_heartbeat.json'):
                    source = build/name
                    if source.exists() and source.stat().st_mtime >= process_start_wall:
                        shutil.copy2(source, args.run_dir/name)
                        report['postmortems'].append(name)
            except OSError:
                report['postmortem_error'] = traceback.format_exc()
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

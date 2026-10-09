"""Prepare owned visible X5 peers and park at the native eight-boss selector.

Host input overrides are released after validation for the human playtest.
Successful runs stay alive for the owner; failed runs stop only their own PIDs.
"""
import argparse
import ctypes as C
import json
import os
import re
from pathlib import Path
import shutil
import sys
import subprocess
import time
import tomllib
from types import SimpleNamespace
from concurrent.futures import ThreadPoolExecutor

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / 'tools'))
from test_mmx5_coop_netplay import prepare_peer, reserve_ports, change_config, parse_crc_log, sha256_file, require, PLAY
from coop_debug import Runtime


def sony_pad(expected_guid=None,expected_name='Sony controller'):
    os.add_dll_directory('C:/msys64/mingw64/bin')
    s = C.CDLL('C:/msys64/mingw64/bin/SDL2.dll')
    s.SDL_SetHint.argtypes = [C.c_char_p, C.c_char_p]
    s.SDL_SetHint(b'SDL_JOYSTICK_HIDAPI', b'1')
    s.SDL_SetHint(b'SDL_JOYSTICK_RAWINPUT', b'0')
    require(s.SDL_Init(0x2200) == 0, 'SDL controller discovery failed')
    class Guid(C.Structure):
        _fields_ = [('data', C.c_uint8 * 16)]
    s.SDL_JoystickGetDeviceGUID.restype = Guid
    s.SDL_JoystickNameForIndex.restype = C.c_char_p
    s.SDL_JoystickGetGUIDString.argtypes = [Guid, C.c_char_p, C.c_int]
    found = []
    for i in range(s.SDL_NumJoysticks()):
        name = s.SDL_JoystickNameForIndex(i).decode()
        if s.SDL_IsGameController(i) and any(x in name.lower() for x in ('dualsense', 'dualshock', 'ps4', 'ps5')):
            buf = C.create_string_buffer(40)
            s.SDL_JoystickGetGUIDString(s.SDL_JoystickGetDeviceGUID(i), buf, 40)
            guid=buf.value.decode()
            if not expected_guid or guid==expected_guid:
                found.append(dict(name=name, guid=guid, detected=True))
    s.SDL_Quit()
    if not found and expected_guid:
        return dict(name=expected_name,guid=expected_guid,detected=False)
    require(len(found) == 1, f'Expected one connected Sony controller: {found}')
    return found[0]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--build', type=Path, default=ROOT/'build-coop/playtest-independent-displays')
    parser.add_argument('--out', type=Path)
    parser.add_argument('--controller-guid',help='Remembered Sony SDL GUID; allows reconnect after launch')
    parser.add_argument('--controller-name',default='Sony controller',help='Name for a remembered profile')
    options = parser.parse_args()
    staged = options.build.resolve()
    native_settings=tomllib.loads((staged/'game.toml').read_text(encoding='utf-8'))
    checkpoint=json.loads((staged/'CHECKPOINT.json').read_text())
    for filename,key in (('mmx5-runtime.exe','executable_sha256'),('bios/openbios.bin','bios_sha256'),
                         ('saves/card1.mcd','stage_access_card_sha256')):
        require(sha256_file(staged/filename)==checkpoint[key],f'Staged checkpoint mismatch: {filename}')
    out = options.out.resolve() if options.out else ROOT/'build-coop'/('playtest-netplay-'+time.strftime('%Y%m%d-%H%M%S'))
    out.mkdir()
    if options.controller_guid:
        require(bool(re.fullmatch('[0-9a-fA-F]{32}',options.controller_guid)),'Controller GUID must contain32 hex digits')
    pad = sony_pad(options.controller_guid.lower() if options.controller_guid else None,options.controller_name)
    print('Controller:', pad, flush=True)
    seed = out / 'card-seed'
    seed.mkdir()
    shutil.copy2(staged / 'saves/card1.mcd', seed / 'card1.mcd')
    # Free the native directory in a private copy; no old playtest dependency.
    blank = bytearray((seed/'card1.mcd').read_bytes())
    require(len(blank)==131072 and blank[:2]==b'MC', 'Native stage-access card invalid')
    for frame in range(1,16):
        entry=bytearray(128);entry[0]=0xA0;entry[8:10]=b'\xff\xff'
        for value in entry[:127]:entry[127]^=value
        blank[frame*128:(frame+1)*128]=entry
    (seed/'card2.mcd').write_bytes(blank)
    args = SimpleNamespace(exe=staged/'mmx5-runtime.exe', config=staged/'game.toml', game_root=ROOT,
                           bios=staged/'bios/openbios.bin', disc=ROOT/'mmx5/Mega Man X5 (USA).cue',
                           cameras='split', card_seed=seed, seed=1296914485,
                           latency=0, jitter=0, frontend='hidden-window', delay=3, render_verify=False,
                           bios_hle=native_settings.get('runtime',{}).get('bios_hle',True))
    ports, reservations = reserve_ports()
    peers = [prepare_peer(args, out, seat, ports, False, args.seed) for seat in (0, 1)]
    defaults = dict(up='Up', down='Down', left='Left', right='Right', triangle='A', circle='S',
                    cross='X', square='Z', l1='Q', l2='E', r1='W', r2='R', l3='T', r3='Y',
                    start='Return', select='Right Shift', ls_up='Up', ls_down='Down',
                    ls_left='Left', ls_right='Right', rs_up='None', rs_down='None', rs_left='None', rs_right='None')
    for peer in peers:
        peer.argv.remove('--hidden-window')
        peer.environment.update(PATH='C:/msys64/mingw64/bin;'+os.environ['PATH'], PSX_DEV_INPUT='0', PSX_RENDER_PASS_VERIFY='0')
        if peer.seat == 1:
            peer.environment['SDL_AUDIODRIVER'] = 'dummy'  # one audible copy of the shared simulation
        source = (peer.directory/'game.toml').read_text(encoding='utf-8')
        for section, key, value in (
            ('controller', 'p1_device', pad['guid'] if peer.seat == 0 else 'keyboard'),
            ('controller', 'p2_device', 'none'),
            ('video', 'aspect_ratio', '4:3'), ('video', 'renderer', 'opengl'),
            ('video', 'internal_resolution', '1080p'),
            ('runtime', 'window_title', f'Mega Man X5 - P{peer.seat+1} - '+('Sony controller' if peer.seat == 0 else 'Keyboard')),
        ):
            source = change_config(source, section, key, value)
        (peer.directory/'game.toml').write_text(source, encoding='utf-8')
        # Framework keyboard binds remain live alongside a controller, so explicitly
        # unbind them on the controller peer rather than relying on device selection.
        binds = []
        for player in range(1, 6):
            binds.append(f'[player{player}]')
            for key, value in defaults.items():
                binds.append(f'{key} = {value if peer.seat == 1 and player == 1 else "None"}')
            binds.append('')
        (peer.directory/'keybinds.ini').write_text('\n'.join(binds), encoding='utf-8')
        openbios = peer.directory/'saves/openbios'
        openbios.mkdir()
        for name in ('card1.mcd', 'card2.mcd'):
            shutil.copy2(seed/name, openbios/name)

    revision = checkpoint['source_commit']
    session = dict(directory=str(out), source_commit=revision,
                   executable_sha256=sha256_file(args.exe), card_sha256=sha256_file(seed/'card1.mcd'),
                   controller=pad, cameras='split', success=False, input_released=False, peers=[])
    pool = ThreadPoolExecutor(max_workers=2)
    def both(method, *values, **kw):
        futures = [pool.submit(getattr(peer, method), *values, **kw) for peer in peers]
        return [f.result() for f in futures]
    def ticks(n):
        start = min(s['tick'] for s in both('status'))
        deadline = time.monotonic()+40
        while min(s['tick'] for s in both('status')) < start+n:
            require(time.monotonic() < deadline, 'Netplay stalled')
            time.sleep(.025)
    def pulse(button, n=2):
        peers[0].input(button)
        ticks(n)
        peers[0].input(0)
        ticks(8)
    def read(addr, size=1):
        return peers[0].read(addr, size)

    try:
        for sock in reservations:
            sock.close()
        for peer in peers:
            peer.launch()
            session['peers'].append(dict(seat=peer.seat, pid=peer.process.pid, tcp=peer.tcp, udp=peer.udp,
                                         directory=str(peer.directory), argv=peer.argv))
        (out/'SESSION.json').write_text(json.dumps(session, indent=2)+'\n')
        print('Launched owned peers:', session['peers'], flush=True)
        deadline = time.monotonic()+45
        for peer in peers:
            while True:
                try:
                    peer.request('frame')
                    peer.input(0)
                    break
                except OSError:
                    require(peer.running() and time.monotonic() < deadline, 'Native runtime boot failed')
                    time.sleep(.1)
        both('status')
        deadline = time.monotonic()+90
        while read(0x800D1F28) != b'\x06':
            require(time.monotonic() < deadline, 'Native title menu timeout')
            pulse(8, 2)
        peers[0].input(0)
        while read(0x800D1F29) != b'\x01' or read(0x8009A424) != b'\x00':
            require(time.monotonic() < deadline, 'Native title fade timeout')
            ticks(3)
        print('Native title ready', flush=True)
        pulse(0x40)
        require(read(0x800D1F2A) == b'\x01', 'Continue cursor did not move')
        pulse(0x4000)
        deadline = time.monotonic()+120
        last = None
        while True:
            play = read(PLAY, 0x1c0)
            if last != play[:4]:
                last = play[:4]
                print('Native save/selector:', last.hex(), 'stage', list(play[12:14]), flush=True)
            if play[:3] == b'\x04\x04\x00':
                break
            require(time.monotonic() < deadline, 'Native Continue did not reach eight-boss selector')
            require(play[0] not in (7,8,9,10,20), 'Native Continue unexpectedly left stage selection')
            if play[0]==4 and play[1]==3:
                peers[0].input(0);ticks(120);continue
            if play[0]==0 and play[1]==3:
                if play[3]!=4:
                    ticks(3);continue
                if play[10]:
                    pulse(0x80);continue
            pulse(0x4000)
            ticks(12)
        ticks(30)
        for peer in peers:
            play = peer.read(PLAY, 0x1c0)
            require(play[:3] == b'\x04\x04\x00', 'Peer is not in interactive eight-boss selector')
            require(play[0x4c] == 0, 'Save has defeated bosses')
            require(play[0x79] < 5 or play[0xcb]&0x80, 'Zero unavailable')
            table = peer.read(0x800F5050, 8)
            require(set(table) == set(range(1, 9)), 'All eight stages unavailable')
            Runtime(peer.tcp).screenshot(out/f'p{peer.seat+1}-selector.png')
        logs = [parse_crc_log(peer.log) for peer in peers]
        common = sorted(set(logs[0]) & set(logs[1]))
        require(len(common) >= 2, 'No matched simulation digests')
        for tick in common:
            require(logs[0][tick] == logs[1][tick], f'Digest mismatch {tick}')
        if pad['detected']:
            require('opened controller for slot: '+pad['name'] in peers[0].log.read_text(errors='replace'), 'P1 Sony pad not opened')
        require('opened controller for slot:' not in peers[1].log.read_text(errors='replace'), 'P2 incorrectly opened a gamepad')
        session.update(success=True, status=both('status'), matched_through=common[-1],
                       stage_table=list(table), cursor=play[0x28], pad_status=both('request', 'pad_status'))
        (out/'SESSION.json').write_text(json.dumps(session, indent=2)+'\n')
        subprocess.run([sys.executable, str(ROOT/'tools/handoff_mmx5_coop_netplay.py'), str(out)], check=True)
        session = json.loads((out/'SESSION.json').read_text())
        print('READY native eight-boss selector; controller P1, keyboard P2; matched through', common[-1], flush=True)
    except BaseException as error:
        session['error'] = str(error)
        for peer in peers:
            peer.stop()
        raise
    finally:
        (out/'SESSION.json').write_text(json.dumps(session, indent=2)+'\n')
        pool.shutdown()
        for peer in peers:
            if peer.output:
                peer.output.close()
        print('Session file:', out/'SESSION.json', flush=True)


if __name__ == '__main__':
    main()

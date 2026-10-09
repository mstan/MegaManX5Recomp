"""Measure native overlap/separation in two private, paced X5 LAN peers.

Uses original Continue/selector and host pad input only. It never touches the
owner's live processes or cards. Cycle scaling is a per-case diagnostic choice.
"""
import argparse
import ctypes
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import shutil
import struct
import time
import traceback
from coop_debug import Runtime
from test_mmx5_coop_netplay import (PLAY, prepare_peer, reserve_ports, change_config,
                                  parse_crc_log, require, sha256_file)


def main():
    root = Path(__file__).resolve().parents[1]
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', type=Path, default=root/'build-coop/mmx5-runtime.exe')
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--card', type=Path, default=root/'build-coop/stage-access-ready/card1.mcd')
    p.add_argument('--verify', type=int, choices=(0, 1), default=0)
    p.add_argument('--cycle-scale', type=int, choices=(1, 2, 3, 4), default=1)
    p.add_argument('--bios-hle', type=int, choices=(0,1), default=0)
    p.add_argument('--window', type=float, default=6)
    p.add_argument('--overlap-only', action='store_true', help='Bound the sample to native stage entry')
    p.add_argument('--portrait', type=int, choices=(0, 1, 2, 3), default=2)
    p.add_argument('--latency', type=int, default=0)
    p.add_argument('--jitter', type=int, default=0)
    args = p.parse_args()
    args.exe = args.exe.resolve()
    args.out = args.out.resolve()
    args.out.mkdir(parents=True, exist_ok=False)
    args.game_root = root
    args.config = root/'game.toml'
    args.disc = root/'mmx5/Mega Man X5 (USA).cue'
    args.bios = root/'build-coop/bios/openbios.bin'
    args.cameras = 'split'
    args.frontend = 'hidden-window'
    args.delay = 3
    args.seed = 1296914485
    args.card_seed = args.out/'seed'
    args.card_seed.mkdir()
    shutil.copy2(args.card, args.card_seed/'card1.mcd')
    # Clear the native file directory of a copy to make the second card blank.
    blank = bytearray(args.card.read_bytes())
    for frame in range(1, 16):
        start = frame*128
        entry = bytearray(128)
        entry[0] = 0xA0
        entry[8:10] = b'\xff\xff'
        for value in entry[:127]:
            entry[127] ^= value
        blank[start:start+128] = entry
    (args.card_seed/'card2.mcd').write_bytes(blank)
    ports, reservations = reserve_ports()
    peers = [prepare_peer(args, args.out, s, ports, bool(args.latency or args.jitter), args.seed) for s in (0, 1)]
    for peer in peers:
        peer.environment.update(PATH='C:/msys64/mingw64/bin;'+os.environ['PATH'],
                                SDL_AUDIODRIVER='dummy', PSX_DEV_INPUT='0',
                                PSX_RENDER_PASS_VERIFY=str(args.verify))
        source = (peer.directory/'game.toml').read_text(encoding='utf-8')
        for section, key, value in (
            ('video', 'aspect_ratio', '4:3'), ('video', 'renderer', 'opengl'),
            ('video', 'internal_resolution', '1080p'),
            ('controller', 'p1_device', 'none'), ('controller', 'p2_device', 'none'),
            ('timing', 'guest_cycle_scale', args.cycle_scale),
        ):
            source = change_config(source, section, key, value)
        # Native active gameplay only; loaders, menus and story stay faithful.
        source += ('guest_cycle_scale_gate = ['
                   '{addr=0x800D1C00,size=1,value=10},'
                   '{addr=0x800D1C01,size=1,value=0},'
                   '{addr=0x800D1C0F,size=1,value=0},'
                   '{addr=0x800D1C10,size=1,value=0},'
                   '{addr=0x800D1C1C,size=1,value=0}]\n')
        (peer.directory/'game.toml').write_text(source, encoding='utf-8')
        cards = peer.directory/'saves/openbios'
        cards.mkdir()
        for name in ('card1.mcd', 'card2.mcd'):
            shutil.copy2(args.card_seed/name, cards/name)
    report = dict(success=False, human_validation=False, executable_sha256=sha256_file(args.exe),
                  verify=args.verify, cycle_scale=args.cycle_scale, bios_hle=args.bios_hle, portrait=args.portrait,
                  latency=args.latency, jitter=args.jitter, samples=[], checkpoints=[])
    pool = ThreadPoolExecutor(max_workers=2)
    def both(method, *values, **fields):
        futures = [pool.submit(getattr(peer, method), *values, **fields) for peer in peers]
        return [future.result() for future in futures]
    def ticks(n):
        start = min(s['tick'] for s in both('status'))
        deadline = time.monotonic()+45
        while min(s['tick'] for s in both('status')) < start+n:
            require(time.monotonic() < deadline, 'Netplay stopped advancing')
            time.sleep(.03)
    def pulse(button, n=2, seat=0):
        peers[seat].input(button)
        ticks(n)
        peers[seat].input(0)
        ticks(8)
    def read(addr, n=1):
        return peers[0].read(addr, n)
    def agreement(label):
        logs = [parse_crc_log(peer.log) for peer in peers]
        common = sorted(set(logs[0]) & set(logs[1]))
        require(len(common)>2, 'No matched digest samples')
        for tick in common:
            require(logs[0][tick] == logs[1][tick], f'{label}: mismatch at tick {tick}')
        status = both('status')
        report['checkpoints'].append(dict(name=label, through=common[-1], status=status))
    def snapshot(peer):
        # Query actual process CPU time; phase flags also include paced waits.
        class FileTime(ctypes.Structure):
            _fields_=[('low',ctypes.c_uint32),('high',ctypes.c_uint32)]
        cpu_seconds=None
        if os.name=='nt':
            k=ctypes.WinDLL('kernel32',use_last_error=True)
            k.OpenProcess.restype=ctypes.c_void_p
            k.GetProcessTimes.argtypes=[ctypes.c_void_p]+[ctypes.POINTER(FileTime)]*4
            k.CloseHandle.argtypes=[ctypes.c_void_p]
            handle=k.OpenProcess(0x1000,False,peer.process.pid)
            times=[FileTime() for _ in range(4)]
            if handle:
                if k.GetProcessTimes(handle,*[ctypes.byref(t) for t in times]):
                    cpu_seconds=sum((t.high<<32)|t.low for t in times[2:])*1e-7
                k.CloseHandle(handle)
        state = peer.observe()
        frame = peer.request('frame')['frame']
        stats = peer.request('render_pass_stats')
        counters = {r['name']:int(r['count']) for r in peer.request('mod_counters')['counters']}
        layers = peer.read(counters['mmx5.coop.split-state']+16, 2*0xfc)
        cameras = [dict(x=struct.unpack_from('<h',layers,s*0xfc+10)[0],
                        y=struct.unpack_from('<h',layers,s*0xfc+14)[0]) for s in (0,1)]
        return dict(time=time.perf_counter(), cpu_seconds=cpu_seconds, state=state, frame=frame,
                    status=peer.status(), stats=stats, cameras=cameras)
    def sample(label):
        both('input',0)
        ticks(15)
        before = [snapshot(peer) for peer in peers]
        time.sleep(args.window)
        after = [snapshot(peer) for peer in peers]
        rows = []
        for peer,a,b in zip(peers,before,after):
            duration = b['time']-a['time']
            rates = dict(vblank_hz=(b['frame']-a['frame'])/duration,
                         net_hz=(b['status']['tick']-a['status']['tick'])/duration,
                         world_hz=(b['state']['world']-a['state']['world'])/duration,
                         draws_hz=(b['stats']['local_views']-a['stats']['local_views'])/duration)
            if a['cpu_seconds'] is not None and b['cpu_seconds'] is not None:
                rates['cpu_percent']=(b['cpu_seconds']-a['cpu_seconds'])/duration*100
            faults = ('watchdog','vram_leaks','device_reads','verify_mismatch')
            require(all(b['stats'].get(k,0)==0 for k in faults), 'Local sandbox fault')
            require(b['state']['lifecycle'][:2] == [0,0], f'{label}: participant died; performance window invalid')
            rows.append(dict(seat=peer.seat, duration=duration, rates=rates, before=a, after=b,
                             frame_perf=peer.request('frame_perf'), phase=peer.request('phase_profile',window=5)))
            Runtime(peer.tcp).screenshot(peer.directory/f'{label}.png')
        report['samples'].append(dict(name=label, peers=rows))
        agreement(label)
        print(label, json.dumps([dict(seat=r['seat'], rates=r['rates'],
                     pass_ms=r['after']['stats']['avg_pass_ms'], cameras=r['after']['cameras']) for r in rows]), flush=True)
    try:
        for sock in reservations:
            sock.close()
        for peer in peers:
            peer.launch()
        report['pids'] = [peer.process.pid for peer in peers]
        print('Owned perf peers',report['pids'], 'scale',args.cycle_scale,'verify',args.verify,flush=True)
        deadline = time.monotonic()+60
        for peer in peers:
            while True:
                try:
                    peer.request('frame');peer.input(0);break
                except OSError:
                    require(peer.running() and time.monotonic()<deadline,'Runtime boot failed')
                    time.sleep(.1)
        while read(0x800D1F28)!=b'\x06':
            require(time.monotonic()<deadline,'Title timeout');pulse(8,2)
        while read(0x800D1F29)!=b'\x01' or read(0x8009A424)!=b'\x00':
            require(time.monotonic()<deadline,'Title fade timeout');ticks(3)
        pulse(0x40)
        require(read(0x800D1F2A)==b'\x01','Continue cursor wrong')
        pulse(0x4000)
        deadline=time.monotonic()+120
        last_menu = None
        while read(PLAY,3)!=b'\x04\x04\x00':
            play=read(PLAY,16)
            if play[:4] != last_menu:
                last_menu = play[:4]
                print('Native load',last_menu.hex(),list(play[12:14]),flush=True)
            require(time.monotonic()<deadline,'Continue timeout')
            require(play[0] not in (7,8,9,10,20), 'Continue unexpectedly left the stage selector pipeline')
            if play[0]==4 and play[1]==3:
                # Character/roster initialization hands off to the portrait
                # controller on its own. A queued Cross can select boss 0 on
                # that first frame, before the observer sees substate zero.
                peers[0].input(0);ticks(120);continue
            if play[0]==0 and play[1]==3:
                # Never confirm while the original dialog is still opening;
                # its default is Cancel and its initialized state is +3==4.
                if play[3]!=4:
                    ticks(3);continue
                if play[10]:
                    pulse(0x80);continue
            pulse(0x4000);ticks(12)
        ticks(20)
        for _ in range(args.portrait):pulse(0x40)
        require(read(PLAY+0x28)==bytes([args.portrait]),'Wrong boss portrait')
        stage=read(0x800F5050+args.portrait)[0]
        pulse(0x4000)
        deadline=time.monotonic()+120
        while True:
            states=both('observe')
            if all(s['mode']==10 and s['minor']==0 and s['enrolled'] and not s['gate'] and s['lifecycle'][:2]==[0,0] for s in states):break
            require(time.monotonic()<deadline,'Selected native stage did not enroll')
            pulse(0x4000)
        require(all(s['stage']==stage for s in states),'Wrong native stage')
        report['stage']=stage
        both('input',0)
        ticks(25)
        sample('overlap')
        if args.overlap_only:
            report['success']=True
            return 0
        report['movement'] = []
        for step in range(40):
            states=both('observe')
            report['movement'].append(states)
            require(all(s['lifecycle'][:2]==[0,0] for s in states),'Participant died during native separation')
            if all(s['p2']['x']-s['p1']['x']>500 for s in states):break
            # Move and jump separately from the native grounded saber lock.
            peers[1].input(0x20); ticks(35)
            moved=both('observe')
            if all(b['p2']['x']-a['p2']['x']<8 for a,b in zip(states,moved)):
                # Native destructible props can block the fortress entrance.
                pulse(0x8000,15,seat=1)
                for _ in range(4):
                    peers[1].input(0x20|0x4000); ticks(10)
                    peers[1].input(0x20); ticks(8)
                # Let Zero land on the fortress ledge beneath the overhang;
                # repeatedly jumping there holds him against the upper stone.
                peers[1].input(0x20); ticks(65)
            peers[1].input(0)
            if step%5==0:
                print('Separate', step, [(s['p1']['x'],s['p2']['x'],s['p2']['y']) for s in both('observe')],flush=True)
        require(all(s['p2']['x']-s['p1']['x']>500 for s in both('observe')),'Native separation did not reach two disjoint views')
        sample('separated')
        report['success']=True
    except BaseException:
        report['error']=traceback.format_exc()
        print(report['error'],flush=True)
        for peer in peers:
            try:
                Runtime(peer.tcp).screenshot(peer.directory/'failure.png')
            except Exception:
                pass
    finally:
        for peer in peers:
            peer.stop()
        pool.shutdown()
        (args.out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    return 0 if report['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())

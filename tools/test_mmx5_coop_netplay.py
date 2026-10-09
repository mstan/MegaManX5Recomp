"""Exercise two private X5 delay-sync peers, using host input and original assets.

Example (run only after rebuilding the co-op title):
  python tools/test_mmx5_coop_netplay.py --exe build-coop/mmx5-runtime.exe \
      --bios build-coop/bios/openbios.bin --disc "mmx5/Mega Man X5 (USA).cue" \
      --out build-coop/qa-netplay

Each case copies the executable and its declared settings manifest into its own
run directory, disables offline co-op, uses private saves, and binds newly reserved
loopback UDP/TCP ports. The trusted profile must enable co-op itself. No RAM writes, save loads, lobby rooms,
or simulation-layer pad overrides are used. Only PIDs created here are stopped.
Native state reads are asynchronous observations; matching sim-tick CRCs and
the existing matched-state watermark provide the network agreement evidence.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
from dataclasses import dataclass
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import socket
import struct
import subprocess
import time
import tomllib
import traceback

PLAY = 0x800D1C00
PLAYER = 0x8009A0A0
MENU = 0x800D4F40
BODY_BYTES = 0x158
START, CROSS, SQUARE, LEFT, RIGHT = 0x0008, 0x4000, 0x8000, 0x0080, 0x0020
CIRCLE = 0x2000
CRC_LINE = re.compile(
    r"rb live dig local sim=(\d+) core=([0-9a-fA-F]{8}).*? mod=([0-9a-fA-F]{8})"
)


class ValidationError(RuntimeError):
    pass


def require(condition: bool, reason: str) -> None:
    if not condition:
        raise ValidationError(reason)


def sha256_file(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest()


def change_config(source: str, section: str, key: str, value: object) -> str:
    """Change one scalar without rewriting unrelated title configuration."""
    encoded = "true" if value is True else "false" if value is False else json.dumps(value)
    block = re.search(r"(?m)^\[" + re.escape(section) + r"\][ \t]*$", source)
    if not block:
        return source.rstrip() + f"\n\n[{section}]\n{key} = {encoded}\n"
    end = re.search(r"(?m)^\[", source[block.end():])
    stop = block.end() + end.start() if end else len(source)
    body = source[block.end():stop]
    pattern = re.compile(r"(?m)^[ \t]*" + re.escape(key) + r"[ \t]*=.*$")
    if pattern.search(body):
        body = pattern.sub(lambda _match: f"{key} = {encoded}", body, count=1)
    else:
        body = body.rstrip() + f"\n{key} = {encoded}\n\n"
    return source[:block.end()] + body + source[stop:]


def parse_crc_log(path: Path) -> dict[int, tuple[str, str]]:
    result: dict[int, tuple[str, str]] = {}
    for match in CRC_LINE.finditer(path.read_text(errors="replace")):
        tick = int(match[1])
        value = (match[2].lower(), match[3].lower())
        require(tick not in result or result[tick] == value,
                f"{path.name}: conflicting state digests for sim tick {tick}")
        result[tick] = value
    return result


@dataclass
class Peer:
    seat: int
    directory: Path
    tcp: int
    udp: int
    argv: list[str]
    environment: dict[str, str]
    process: subprocess.Popen | None = None
    diagnostic: int | None = None
    output: object | None = None

    @property
    def log(self) -> Path:
        return self.directory / "runtime.log"

    def launch(self) -> None:
        require(self.process is None, "Attempted to relaunch an owned peer")
        self.output = self.log.open("wb")
        flags = subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0
        self.process = subprocess.Popen(
            self.argv, cwd=self.directory, env=self.environment,
            stdin=subprocess.DEVNULL, stdout=self.output, stderr=subprocess.STDOUT,
            creationflags=flags,
        )

    def running(self) -> bool:
        return self.process is not None and self.process.poll() is None

    def request(self, cmd: str, *, allow_error: bool = False, **fields) -> dict:
        require(self.running(), f"P{self.seat + 1} process exited before {cmd}")
        with socket.create_connection(("127.0.0.1", self.tcp), timeout=2) as connection:
            connection.settimeout(3)
            connection.sendall((json.dumps(dict(id=1, cmd=cmd, **fields)) + "\n").encode())
            with connection.makefile("rb") as stream:
                line = stream.readline(2 * 1024 * 1024)
            require(bool(line), f"P{self.seat + 1} closed its debug connection during {cmd}")
            result = json.loads(line)
        require(result.get("id") == 1,
                f"P{self.seat + 1}: unexpected debug response for {cmd}: {result}")
        require(allow_error or result.get("ok") is True, f"P{self.seat + 1}: {cmd}: {result}")
        return result

    def read(self, address: int, size: int) -> bytes:
        data = bytes.fromhex(self.request("read_ram", addr=hex(address), len=size)["hex"])
        require(len(data) == size, f"P{self.seat + 1}: truncated state read")
        return data

    def input(self, held: int = 0) -> None:
        # Both peers sample their own local host P1 device. The session routes
        # it to seat 0/1; writing simulation port 2 would bypass that contract.
        self.request("set_input", layer="host", buttons=f"0x{(~held) & 0xffff:04x}",
                     lx=128, ly=128, rx=128, ry=128)

    def status(self) -> dict:
        status = self.request("netplay_status")
        require(status.get("active") == 1, f"P{self.seat + 1}: netplay is inactive")
        require(status.get("rollback") == 0, f"P{self.seat + 1}: profile did not force delay-sync")
        require(status.get("players") == 2, f"P{self.seat + 1}: session is not two-player")
        require(status.get("desync") == 0, f"P{self.seat + 1}: input contract desynchronized: {status}")
        return status

    def observe(self) -> dict:
        if self.diagnostic is None:
            counters = self.request("mod_counters")["counters"]
            self.diagnostic = next(int(row["count"]) for row in counters
                                   if row["name"] == "mmx5.coop.diagnostic-address")
        play = self.read(PLAY, 0x80)
        data = self.read(self.diagnostic, 0x500)
        ticks, ready, enrolled, failed, counterpart, banks, world, timer = struct.unpack_from("<8I", data)
        require(not failed, "Co-op plugin reported failure")
        return dict(mode=play[0], minor=play[1], stage=play[12], section=play[13],
                    campaign=play[0x44], lives=play[0x45], gate=play[0x1C],
                    ticks=ticks, enrolled=enrolled, world=world, timer=timer,
                    lifecycle=list(data[0x40:0x44]), owner=data[0x48],
                    p1=body_state(data[0x300:0x458]), p2=body_state(data[0x100:0x258]))

    def stop(self) -> None:
        if self.running():
            self.process.terminate()
            try:
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                self.process.kill()
                self.process.wait(timeout=5)
        if self.output is not None:
            self.output.close()
            self.output = None


def body_state(body: bytes) -> dict:
    return dict(active=body[0], character=body[2], visible=body[3], state=body[4],
                action=body[5], grounded=bool(body[0x70] & 8),
                hp=body[0x5C] & 0x7F, x=struct.unpack_from("<i", body, 8)[0] / 65536,
                y=struct.unpack_from("<i", body, 12)[0] / 65536)


def reserve_ports() -> tuple[list[int], list[socket.socket]]:
    sockets, ports = [], []
    try:
        for kind in (socket.SOCK_DGRAM, socket.SOCK_DGRAM, socket.SOCK_STREAM, socket.SOCK_STREAM):
            while True:
                candidate = socket.socket(socket.AF_INET, kind)
                candidate.bind(("127.0.0.1", 0))
                port = candidate.getsockname()[1]
                if port not in ports:
                    break
                candidate.close()
            sockets.append(candidate)
            ports.append(port)
        return ports, sockets
    except BaseException:
        for connection in sockets:
            connection.close()
        raise


def prepare_peer(args, case: Path, seat: int, ports: list[int], jitter: bool,
                 session: int) -> Peer:
    directory = case / f"peer-{seat}"
    directory.mkdir()
    executable = directory / args.exe.name
    shutil.copy2(args.exe, executable)
    for dll in args.exe.parent.glob("*.dll"):
        if dll.is_file():
            shutil.copy2(dll, directory / dll.name)
    (directory / "mods").mkdir()
    settings = args.exe.parent / "mods/bundled/mmx5.coop"
    require(settings.is_dir(), "Built executable is missing its declared co-op settings package")
    shutil.copytree(settings, directory / "mods/bundled/mmx5.coop")
    (directory / "mods/state.toml").write_text(
        'format_version = 2\n\n[[feature]]\npackage_id = "mmx5.coop"\n'
        f'id = "coop"\nenabled = false\n[feature.values]\ncameras = "{args.cameras}"\n', encoding="utf-8")
    saves = directory / "saves"
    saves.mkdir()
    card_seed = getattr(args, 'card_seed', None)
    if card_seed is not None:
        for name in ('card1.mcd', 'card2.mcd'):
            source_card = card_seed / name
            require(source_card.is_file(), f'Missing private save seed: {source_card}')
            shutil.copy2(source_card, saves / name)
    source = args.config.read_text(encoding="utf-8")
    original = tomllib.loads(source)
    game_exe = Path(original["game"]["exe"])
    if not game_exe.is_absolute():
        game_exe = args.game_root / game_exe
    for section, key, value in (
        ("game", "exe", str(game_exe.resolve())),
        ("game", "disc", str(args.disc)),
        ("runtime", "memcard_dir", str(saves)),
        ("runtime", "overlay_cache", False),
        ("runtime", "overlay_autocompile_cmd", ""),
        ("runtime", "turbo_loads", False),
        ("runtime", "bios_hle", False),
        # Disagree offline preferences deliberately. The trusted session must
        # force 4:3; explicit CLI OpenGL chooses the supported presentation
        # backend while unequal quality settings leave guest VRAM unchanged.
        ("video", "aspect_ratio", "16:9" if seat == 0 else "21:9"),
        ("video", "renderer", "software" if seat == 0 else "vulkan"),
        ("video", "internal_resolution", "1080p" if seat == 0 else "720p"),
        ("video", "auto_skip_fmv", False),
        ("video", "frame_interpolation", False),
        ("netplay", "content_negotiation", False),
    ):
        source = change_config(source, section, key, value)
    config = directory / "game.toml"
    tomllib.loads(source)  # Refuse malformed config before starting any process.
    config.write_text(source, encoding="utf-8")
    environment = {key: value for key, value in os.environ.items()
                   if not key.upper().startswith(("PSX_", "RNET_"))}
    environment.update(PSX_NET_TRANSPORT="lan", PSX_NET_MODE="delay",
                       PSX_RENDER_PASS_VERIFY="1",
                       RNET_SIM_LATENCY_MS=str(args.latency if jitter else 0),
                       RNET_SIM_JITTER_MS=str(args.jitter if jitter else 0),
                       RNET_SIM_SEED=str(args.seed + seat), RNET_SIM_LOSS_PCT="0")
    argv = [str(executable), "--bios", str(args.bios), "--game", str(config),
            "--disc", str(args.disc), "--memcard-dir", str(saves), "--renderer", "opengl",
            "--no-launcher", "--headless-opengl" if args.frontend == "headless-opengl"
            else "--hidden-window", "--netplay", "--net-slot", str(seat),
            "--net-input-player", "0", "--net-bind", f"127.0.0.1:{ports[seat]}",
            "--net-peer", f"127.0.0.1:{ports[1 - seat]}", "--net-delay", str(args.delay),
            "--net-session-id", str(session), "--debug-port", str(ports[2 + seat])]
    return Peer(seat, directory, ports[2 + seat], ports[seat], argv, environment)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--disc', type=Path, required=True)
    parser.add_argument('--bios', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--character', choices=('x', 'zero'), default='x')
    parser.add_argument('--cameras',choices=('unified','split'),default='unified')
    parser.add_argument('--split-separation',action='store_true')
    parser.add_argument('--split-withdrawal',action='store_true')
    parser.add_argument('--frontend',choices=('headless-opengl','hidden-window'),default='headless-opengl')
    parser.add_argument('--latency', type=int, default=0)
    parser.add_argument('--jitter', type=int, default=0)
    parser.add_argument('--delay', type=int, default=3)
    args = parser.parse_args()
    require(not args.split_separation or args.cameras=='split','Separation requires Split')
    require(not args.split_withdrawal or args.cameras=='split','Withdrawal view test requires Split')
    for key in ('exe', 'disc', 'bios', 'out'):
        setattr(args, key, getattr(args, key).resolve())
    args.game_root = Path(__file__).resolve().parent.parent
    args.config = args.game_root/'game.toml'
    args.seed = 1296914485
    args.out.mkdir(parents=True, exist_ok=False)
    ports, reservations = reserve_ports()
    peers = [prepare_peer(args, args.out, seat, ports, bool(args.latency), args.seed) for seat in (0,1)]
    report = dict(human_validation=False, success=False, cameras=args.cameras,exe_sha256=sha256_file(args.exe),
                  checkpoints=[], observations=[])
    pool = ThreadPoolExecutor(max_workers=2)

    def both(method, *values):
        futures = [pool.submit(getattr(peer, method), *values) for peer in peers]
        return [future.result() for future in futures]

    def ticks(count):
        start = min(s['tick'] for s in both('status'))
        deadline = time.monotonic()+45
        while min(s['tick'] for s in both('status'))<start+count:
            require(time.monotonic()<deadline, 'Netplay simulation stopped advancing')
            time.sleep(.05)

    def pulse(held, seat=0, count=12):
        peers[seat].input(held)
        ticks(count)
        peers[seat].input(0)
        ticks(max(args.delay+4,8))

    def observe():
        states = both('observe')
        report['observations'].append(states)
        return states

    def checkpoint(name, after=0):
        deadline = time.monotonic()+45
        while time.monotonic()<deadline:
            status = both('status')
            logs = [parse_crc_log(peer.log) for peer in peers]
            common = sorted(set(logs[0])&set(logs[1]))
            for tick in common:
                require(logs[0][tick]==logs[1][tick], f'{name}: core/mod mismatch at {tick}')
            watermark = min(s['resolved_through'] for s in status)
            samples = [dict(tick=tick, core=logs[0][tick][0], mod=logs[0][tick][1])
                       for tick in common if after<tick<=watermark]
            if len(samples)>=2:
                report['checkpoints'].append(dict(name=name, status=status, samples=samples))
                print(name, 'matched', len(samples), 'CRC samples through', watermark, flush=True)
                return max(s['tick'] for s in status)
            time.sleep(.2)
        raise ValidationError(f'{name}: insufficient matched CRC evidence')

    try:
        for reservation in reservations:
            reservation.close()
        for peer in peers:
            peer.launch()
        report['pids'] = [peer.process.pid for peer in peers]
        print('Owned private X5 pair', report['pids'], flush=True)
        deadline = time.monotonic()+180
        for peer in peers:
            while True:
                try:
                    peer.status()
                    peer.input(0)
                    break
                except (OSError, ValidationError):
                    require(peer.running() and time.monotonic()<deadline, 'Pair startup failed')
                    time.sleep(.2)
        ticks(30)
        previous = None
        already_selected=False
        while True:
            state = observe()
            modes = [(s['mode'],s['minor']) for s in state]
            if modes!=previous:
                print('Native boot', modes, flush=True)
                previous=modes
            if all(s['mode']==1 and s['minor']==3 for s in state):
                break
            if args.character=='x' and all(s['mode'] in (20,7,8,9,10) for s in state):
                # An unpaced original Cross pulse can complete the default
                # selector between observations. Verify actual X bodies at
                # enrollment below; never force a selector or identity.
                already_selected=True;break
            require(time.monotonic()<deadline, 'Native character selection timed out')
            pulse(START)
            if any(s['mode']==1 and s['minor']==3 for s in observe()):
                ticks(20)
                continue
            pulse(CROSS)
        if not already_selected:
            if args.character=='zero':pulse(RIGHT)
            pulse(CROSS)
        while not all(s['enrolled'] and s['mode']==10 and not s['gate'] for s in observe()):
            require(time.monotonic()<deadline, 'Native enrollment timed out')
            pulse(CROSS)
        both('input',0)
        while not all(s['lifecycle'][:2]==[0,0] for s in observe()):
            require(time.monotonic()<deadline,'Native counterpart arrival did not finish')
            ticks(10)
        ticks(20)
        state=observe()
        expected=int(args.character=='zero')
        require(all(s['p1']['character']==expected and s['p2']['character']==(expected^1) for s in state),
                'Wrong native character pair')
        last=checkpoint('intro')
        if args.cameras=='split':
            ticks(20)
            probes=[]
            for peer in peers:
                counters={row['name']:int(row['count']) for row in peer.request('mod_counters')['counters']}
                stats=peer.request('render_pass_stats')
                probes.append(dict(seat=peer.seat,counters=counters,stats=stats))
                report['view_probes']=probes
                require(counters.get('mmx5.coop.local-view-committed',0)>0,'No committed local camera view')
                require(counters.get('mmx5.coop.local-view-state-leak',0)==0,'Local draw changed canonical host state')
                require(stats.get('verify_checks',0)>0,'No sandbox restoration checks')
                require(all(stats.get(key,0)==0 for key in ('verify_mismatch','watchdog','vram_leaks','device_reads')),
                        f'Own-view sandbox fault: {stats}')
                peer.request('screenshot_hires',path=str(peer.directory/'own-intro.png'))
        for seat in (0,1):
            before=observe()
            # Zero's original grounded saber can hold position. Check motion
            # on its own before exercising that native attack separately.
            pulse(RIGHT, seat, 30)
            after=observe()
            key=f'p{seat+1}'
            require(all(b[key]['x']>a[key]['x']+1 for a,b in zip(before,after)), 'Seat did not move independently')
            other=f'p{2-seat}'
            require(all(b[other]['x']==a[other]['x'] for a,b in zip(before,after)), 'Moved the other seat')
            pulse(SQUARE,seat,12)
            last=checkpoint(f'P{seat+1} movement and attack',last)
        if args.split_withdrawal:
            inventory=[]
            for peer in peers:
                data=peer.read(peer.diagnostic+0x100,0x158)
                inventory.append((data[0x5c],data[0xa8:0xc8]))
            peers[1].input(1);ticks(110);peers[1].input(0)
            deadline=time.monotonic()+25
            while not all(s['lifecycle'][1]==2 for s in observe()):
                require(time.monotonic()<deadline,'Native departure did not complete');ticks(8)
            ticks(8)
            for peer in peers:
                counters={r['name']:int(r['count']) for r in peer.request('mod_counters')['counters']}
                layers=peer.read(counters['mmx5.coop.split-state']+16,2*0xfc)
                require(layers[:0xfc]==layers[0xfc:],'Absent seat did not adopt survivor camera')
                peer.request('screenshot_hires',path=str(peer.directory/'absent-spectator.png'))
            last=checkpoint('Native absence and spectator view',last)
            pulse(1,1,6)
            deadline=time.monotonic()+25
            while not all(s['lifecycle'][1]==0 for s in observe()):
                require(time.monotonic()<deadline,'Native returning arrival did not complete');ticks(8)
            for peer,previous in zip(peers,inventory):
                data=peer.read(peer.diagnostic+0x100,0x158)
                require((data[0x5c],data[0xa8:0xc8])==previous,'Return changed native private inventory')
                peer.request('screenshot_hires',path=str(peer.directory/'returned-own-view.png'))
            last=checkpoint('Native return and private resources',last)
        for seat in (1,0):
            pulse(START,seat)
            ticks(60)
            before=observe()
            require(all(s['minor']==2 and s['owner']==seat+1 for s in before),'Wrong native menu owner')
            for peer in peers:
                peer.request('screenshot_hires',path=str(peer.directory/f'menu-p{seat+1}.png'))
            ticks(40)
            after=observe()
            require(all(a['world']==b['world'] and a['timer']==b['timer'] for a,b in zip(before,after)),
                    'Menu did not freeze the world')
            pulse(RIGHT,seat^1)
            require(all(s['minor']==2 for s in observe()),'Other seat disturbed the menu')
            pulse(START,seat)
            ticks(60)
            require(all(s['minor']==0 for s in observe()), 'Native menu failed to close')
            last=checkpoint(f'P{seat+1} menu and restore',last)
        if args.split_separation:
            observations=[]
            def split_observe(label):
                rows=[]
                for peer in peers:
                    counters={r['name']:int(r['count']) for r in peer.request('mod_counters')['counters']}
                    address=counters['mmx5.coop.split-state']
                    layers=peer.read(address+16,2*0xfc)
                    cameras=[dict(x=struct.unpack_from('<h',layers,seat*0xfc+10)[0],
                                  y=struct.unpack_from('<h',layers,seat*0xfc+14)[0]) for seat in (0,1)]
                    pool=peer.read(0x80092090,48*0x9c)
                    enemies=[dict(slot=i,type=pool[i*0x9c+1],hp=pool[i*0x9c+0x5c],
                                  x=struct.unpack_from('<h',pool,i*0x9c+10)[0],
                                  y=struct.unpack_from('<h',pool,i*0x9c+14)[0]) for i in range(48) if pool[i*0x9c]]
                    common=struct.unpack('<I',peer.read(0x1f80008c,4))[0]
                    rows.append(dict(seat=peer.seat,cameras=cameras,enemies=enemies,
                                     common_callbacks=[hex(v) for v in struct.unpack('<8I',peer.read(common,32))],
                                     players=peer.observe(),counters=counters,stats=peer.request('render_pass_stats')))
                    peer.request('screenshot_hires',path=str(peer.directory/f'{label}.png'))
                observations.append(dict(name=label,peers=rows));report['separation']=observations
                return rows
            before=split_observe('before-separation')
            # The native opening road is a bounded authored region. Move P2
            # through it with original input; do not write positions online.
            for step in range(20):
                before_move=observe()
                pulse(RIGHT,1,20)
                after_move=observe()
                if all(b['p2']['x']-a['p2']['x']<5 for a,b in zip(before_move,after_move)):
                    pulse(RIGHT|CROSS,1,45)  # Original jump over an authored truck/obstacle.
                if all(s['p2']['x']-s['p1']['x']>480 for s in observe()):break
            separated=split_observe('separated')
            require(all(abs(row['cameras'][1]['x']-row['cameras'][0]['x'])>416 for row in separated),
                    'Native cameras did not separate beyond two disjoint activation windows')
            require(all(row['counters'].get('mmx5.coop.split-placement-scans',0)>0 for row in separated),
                    'No native dual-view placement scans')
            require(all(row['counters'].get('mmx5.coop.p2-native-ai',0)>0 for row in separated),
                    'No native nearby-P2 actor update')
            require(all(row['counters'].get('mmx5.coop.local-view-state-leak',0)==0 and
                        row['counters'].get('mmx5.coop.local-view-background-invalid',0)==0 for row in separated),
                    'Distant local view leaked state or failed map rendering')
            last=checkpoint('Separated native cameras, enemies and own views',last)
            for step in range(20):
                before_move=observe()
                pulse(LEFT,1,20)
                after_move=observe()
                if all(a['p2']['x']-b['p2']['x']<5 for a,b in zip(before_move,after_move)):
                    pulse(LEFT|CROSS,1,45)
                if all(abs(s['p2']['x']-s['p1']['x'])<80 for s in observe()):break
            rejoined=split_observe('rejoined')
            require(all(abs(row['players']['p2']['x']-row['players']['p1']['x'])<80 for row in rejoined),
                    'Native separated actors could not rejoin')
            last=checkpoint('Native overlap and rejoin',last)
        for peer in peers:
            peer.request('screenshot_hires',path=str(peer.directory/'final.png'))
        report['success']=True
    except BaseException:
        report['error']=traceback.format_exc()
        print(report['error'],flush=True)
    finally:
        for peer in peers:
            peer.stop()
        pool.shutdown(wait=True)
        for reservation in reservations:
            reservation.close()
        (args.out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    return 0 if report['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())

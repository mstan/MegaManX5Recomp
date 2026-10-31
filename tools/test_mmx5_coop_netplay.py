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
from coop_debug import Runtime

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
    debug_timeout: float = 10.0

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
            connection.settimeout(self.debug_timeout)
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

    def screenshot(self,path: Path) -> dict:
        return Runtime(self.tcp).screenshot(path)

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
                    genuine_flags4A=play[0x4A],
                    ticks=ticks, enrolled=enrolled, world=world, timer=timer,
                    lifecycle=list(data[0x40:0x44]), owner=data[0x48],
                    p1=body_state(data[0x300:0x458]), p2=body_state(data[0x100:0x258]))

    def stop(self) -> None:
        if self.running():
            try:
                self.request('quit')
                self.process.wait(timeout=5)
            except (OSError,ValueError,ValidationError,subprocess.TimeoutExpired):
                if self.running():self.process.terminate()
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
                action=body[5], native_roster=body[0xFA], grounded=bool(body[0x70] & 8),
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


def peer_mod_selection(args, seat: int) -> str:
    selection = ('format_version = 2\n\n[[feature]]\npackage_id = "mmx5.coop"\n'
                 f'id = "coop"\nenabled = false\n[feature.values]\ncameras = "{args.cameras}"\n')
    if getattr(args, 'automatic_forms', False):
        # The host's co-op values become the trusted session configuration.
        # Conflicting standalone choices must have no effect on either peer.
        selection += 'ultimate_armor_x="true"\nblack_zero="true"\n'
        for package, feature, enabled in (
                ('mmx5.cheat.ultimate-armor', 'ultimate-armor', seat == 0),
                ('mmx5.cheat.black-zero', 'black-zero', seat == 1)):
            selection += (f'\n[[package]]\nid="{package}"\n\n[[feature]]\n'
                          f'package_id="{package}"\nid="{feature}"\n'
                          f'enabled={str(enabled).lower()}\n')
    if getattr(args, 'alia_hints', False):
        selection += ('\n[[feature]]\npackage_id="mmx5.enhancement.skip-alia"\n'
                      f'id="skip-alia"\nenabled={"true" if seat else "false"}\n')
    return selection


def prepare_peer(args, case: Path, seat: int, ports: list[int], jitter: bool,
                 session: int) -> Peer:
    directory = case / f"peer-{seat}"
    directory.mkdir()
    executable = directory / args.exe.name
    shutil.copy2(args.exe, executable)
    # Bundled-BIOS builds verify their image next to each staged executable.
    # An external --bios path does not replace that packaging contract.
    bundled_bios=args.exe.parent/'bios/openbios.bin'
    require(bundled_bios.is_file(),f'Missing built bundled BIOS: {bundled_bios}')
    (directory/'bios').mkdir()
    shutil.copy2(bundled_bios,directory/'bios/openbios.bin')
    notice=bundled_bios.parent/'OpenBIOS.LICENSE'
    if notice.is_file():shutil.copy2(notice,directory/'bios/OpenBIOS.LICENSE')
    for dll in args.exe.parent.glob("*.dll"):
        if dll.is_file():
            shutil.copy2(dll, directory / dll.name)
    (directory / "mods").mkdir()
    settings = args.exe.parent / "mods/bundled/mmx5.coop"
    require(settings.is_dir(), "Built executable is missing its declared co-op settings package")
    # Split now derives the modern renderer. Exercise the staged catalog,
    # including default diagnostics, rather than an incomplete co-op-only copy.
    shutil.copytree(settings.parent, directory / "mods/bundled")
    if (args.exe.parent/'assets').is_dir():
        shutil.copytree(args.exe.parent/'assets',directory/'assets')
    if getattr(args, 'automatic_forms', False):
        for package in ('mmx5.cheat.ultimate-armor', 'mmx5.cheat.black-zero'):
            require((directory/'mods/bundled'/package).is_dir(),
                    f'Automatic form control is missing bundled package: {package}')
    (directory/'mods/state.toml').write_text(peer_mod_selection(args, seat), encoding='utf-8')
    saves = directory / "saves"
    saves.mkdir()
    card_seed = getattr(args, 'card_seed', None)
    if card_seed is not None:
        for name in ('card1.mcd', 'card2.mcd'):
            source_card = card_seed / name
            if name == 'card1.mcd':
                require(source_card.is_file(), f'Missing downloaded save seed: {source_card}')
            if source_card.is_file():
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
        ("runtime", "bios_hle", bool(getattr(args, 'bios_hle', False))),
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
                       SDL_AUDIODRIVER="dummy",
                       # Expensive restoration hashing belongs to QA. Human
                       # launchers explicitly opt out while retaining the sandbox.
                       PSX_RENDER_PASS_VERIFY="1" if getattr(args, 'render_verify', True) else "0",
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
    parser.add_argument('--perf-window',type=float,default=0,help='Seconds per stationary performance sample')
    parser.add_argument('--no-render-verify',action='store_true',help='Measure without QA restoration hashing')
    parser.add_argument('--bios-hle',action='store_true',help='Use the normal title BIOS HLE mode instead of QA LLE')
    parser.add_argument('--personal-effects',action='store_true',
                        help='Check native full-charge glow and matched CRCs while it is active')
    parser.add_argument('--scene-fatal',action='store_true',
                        help='Use replicated qualification inputs to test native Grizzly scene-owner death')
    parser.add_argument('--scene-owner',type=int,choices=(0,1),default=0,
                        help='Scene-owner scenario leader: 0 for P1, 1 for P2')
    parser.add_argument('--shared-scroll',action='store_true',
                        help='Observe the native train origins and wrap during the existing Grizzly mission setup')
    parser.add_argument('--alia-hints',action='store_true',
                        help='Verify native hint suppression despite disagreeing offline preferences')
    parser.add_argument('--capture-arrival',action='store_true',
                        help='Retain own-view captures and native state during the paired incoming beams')
    parser.add_argument('--ultimate-nova',action='store_true',
                        help='Enter native X title code, separate P2 by input, and check Nova/private views')
    parser.add_argument('--automatic-forms',action='store_true',
                        help='Agree on both automatic co-op forms despite conflicting offline standalone choices; Nova skips the title code')
    args = parser.parse_args()
    args.render_verify=not args.no_render_verify
    require(not args.ultimate_nova or (args.character=='x' and args.cameras=='split'),
            'Ultimate Nova requires native X/P1 and Split')
    require(not args.split_separation or args.cameras=='split','Separation requires Split')
    require(not args.split_withdrawal or args.cameras=='split','Withdrawal view test requires Split')
    require(not args.scene_fatal or args.cameras=='split','Scene-owner test requires Split')
    require(not args.shared_scroll or args.scene_fatal,'Shared scroll requires the native Grizzly mission setup')
    require(not (args.scene_fatal and args.split_separation), 'Scene-owner death and two-living-actor separation are separate cases')
    for key in ('exe', 'disc', 'bios', 'out'):
        setattr(args, key, getattr(args, key).resolve())
    for key in ('exe','disc','bios'):
        require(getattr(args,key).is_file(),f'Missing input {key}: {getattr(args,key)}')
    require(os.name!='nt' or args.out.drive.upper()!='C:','QA outputs must use non-C: storage')
    args.game_root = Path(__file__).resolve().parent.parent
    args.config = args.game_root/'game.toml'
    args.seed = 1296914485
    args.out.mkdir(parents=True, exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    ports, reservations = reserve_ports()
    peers = [prepare_peer(args, args.out, seat, ports, bool(args.latency), args.seed) for seat in (0,1)]
    report = dict(human_validation=False, success=False, cameras=args.cameras,exe_sha256=sha256_file(args.exe),
                  character=args.character, frontend=args.frontend, bios_hle=args.bios_hle,
                  latency_ms=args.latency, jitter_ms=args.jitter, render_verify=args.render_verify,
                  harness_sha256=sha256_file(Path(__file__)),checkpoints=[], observations=[])
    if args.automatic_forms:
        selections = [tomllib.loads((peer.directory/'mods/state.toml').read_text()) for peer in peers]
        host_values = next(feature['values'] for feature in selections[0]['feature']
                           if feature['package_id']=='mmx5.coop')
        report['automatic_forms'] = dict(
            host_options={key:host_values[key] for key in ('ultimate_armor_x','black_zero')},
            offline_standalone=[dict(seat=peer.seat,features=[feature for feature in selection['feature']
                if feature['package_id'] in ('mmx5.cheat.ultimate-armor','mmx5.cheat.black-zero')])
                for peer,selection in zip(peers,selections)],
            policy='Trusted co-op options own both forms; standalone offline choices disagree and must be ignored online',
            native_code_entered=False)
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

    def performance(label):
        if not args.perf_window:return
        before=both('observe');frames=both('request','frame');net=both('status')
        start=time.perf_counter();time.sleep(args.perf_window)
        after=both('observe');finish=both('request','frame');status=both('status')
        duration=time.perf_counter()-start
        rows=[dict(seat=i,world_hz=(b['world']-a['world'])/duration,
                   vblank_hz=(f['frame']-s['frame'])/duration,
                   net_hz=(n['tick']-p['tick'])/duration,lifecycle=b['lifecycle'],
                   stats=peer.request('render_pass_stats'))
              for i,(peer,a,b,s,f,p,n) in enumerate(zip(peers,before,after,frames,finish,net,status))]
        report.setdefault('performance',[]).append(dict(name=label,duration=duration,peers=rows))
        print(label,'performance',json.dumps([{k:r[k] for k in ('seat','world_hz','vblank_hz','net_hz','lifecycle')} for r in rows]),flush=True)

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
            if all(s['mode'] in (8,9,10) for s in state):
                # An unpaced original Cross pulse can complete the default
                # selector between observations. Verify actual X bodies at
                # enrollment below; never force a selector or identity.
                already_selected=True;break
            require(time.monotonic()<deadline, 'Native character selection timed out')
            if any(s['mode']==1 for s in state):
                # Native selector fades ignore input; broad Start/Cross pulses
                # can confirm default X before observing an interactive menu.
                ticks(8)
            elif any(s['mode']==20 for s in state):pulse(CROSS)
            else:pulse(START)
        if not already_selected:
            if args.character=='zero':pulse(RIGHT)
            if args.ultimate_nova and not args.automatic_forms:
                require(all(peer.read(PLAY,2)==bytes((1,3)) and
                            int.from_bytes(peer.read(0x800EE970,4),'little')==27 and
                            int.from_bytes(peer.read(0x800EF894,4),'little')==0 for peer in peers),
                        'Original title code requires the actual native selector')
                report['ultimate_code']=dict(provenance='Original Up2/Down9 cheat, not capsule acquisition',edges=[])
                for step,button in enumerate([0x10]*2+[0x40]*9,1):
                    pulse(button,count=6)
                    counters=[int.from_bytes(peer.read(0x800EF894,4),'little') for peer in peers]
                    report['ultimate_code']['edges'].append(dict(step=step,counters=counters))
                    require(counters==[step,step],'Native Ultimate code edge was not accepted on both peers')
            pulse(CROSS)
        require(not args.ultimate_nova or args.automatic_forms or not already_selected,
                'Missed native Ultimate code selector')
        while not all(s['enrolled'] and s['mode']==10 and not s['gate'] for s in observe()):
            require(time.monotonic()<deadline, 'Native enrollment timed out')
            if args.capture_arrival:
                arrivals=[Runtime(peer.tcp).personal_effects() for peer in peers]
                if all(row['state']['enrolled'] and row['life'][1]==3 for row in arrivals):
                    if 'paired_arrival' not in report and all(row['bodies'][0][5]==0 for row in arrivals):
                        report['paired_arrival']=[]
                        for peer,row in zip(peers,arrivals):
                            report['paired_arrival'].append(dict(seat=peer.seat,state=row['state'],
                                life=row['life'],scene=row['scene'],bodies=[b.hex() for b in row['bodies']],
                                play=peer.read(PLAY,0x30).hex(),stats=peer.request('render_pass_stats'),
                                counters=peer.request('mod_counters')['counters']))
                            peer.screenshot(peer.directory/'native-paired-arrival.png')
                    ticks(2)
                    continue
            pulse(CROSS,count=2 if args.capture_arrival else 12)
        both('input',0)
        while not all(s['lifecycle'][:2]==[0,0] for s in observe()):
            require(time.monotonic()<deadline,'Native counterpart arrival did not finish')
            ticks(10)
        if args.capture_arrival:
            require('paired_arrival' in report, 'Native paired arrival was not observed')
        ticks(20)
        state=observe()
        expected=int(args.character=='zero')
        require(all(s['p1']['character']==expected and s['p2']['character']==(expected^1) for s in state),
                'Wrong native character pair')
        last=checkpoint('intro')
        if args.automatic_forms:
            rows=[]
            for peer,observed in zip(peers,state):
                require([observed[f'p{seat+1}']['native_roster'] for seat in (0,1)] ==
                        ([4,5] if expected==0 else [5,4]),
                        'Agreed automatic forms did not initialize the actual native body rosters')
                require(not observed['genuine_flags4A']&0x18,
                        'Automatic forms changed genuine native cheat flags')
                rows.append(dict(seat=peer.seat,genuine_flags4A=observed['genuine_flags4A'],
                                 p1=observed['p1'],p2=observed['p2'],status=peer.status()))
            report['automatic_forms']['native_initialization']=rows
            last=checkpoint('Automatic forms, genuine cheat flags clear and conflicting offline choices',last)
            report['automatic_forms']['agreement_checkpoint']=len(report['checkpoints'])-1
        if args.alia_hints:
            rows=[]
            for peer in peers:
                counters={row['name']:int(row['count']) for row in peer.request('mod_counters')['counters']}
                play=peer.read(PLAY, 0x110)
                hidden_text=counters.get('mmx5.skip-alia.suppressed-text',0)
                require(hidden_text>0, 'Online native hint path did not suppress Alia')
                require(not play[0x1C] and play[0xCD+expected*32]&8,
                        'Silent intro hint left a lock or lost its native acknowledgement')
                rows.append(dict(seat=peer.seat,hidden_text=hidden_text,play=play.hex(),
                                 offline_preference=bool(peer.seat)))
            report['alia_hints']=dict(peers=rows,policy='Fixed on in trusted online profile')
            last=checkpoint('Alia hints skipped with disagreeing offline preferences',last)
        performance('overlap')
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
                if args.render_verify:
                    require(stats.get('verify_checks',0)>0,'No sandbox restoration checks')
                require(all(stats.get(key,0)==0 for key in ('verify_mismatch','watchdog','vram_leaks','device_reads')),
                        f'Own-view sandbox fault: {stats}')
                peer.screenshot(peer.directory/'own-intro.png')
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
        if args.personal_effects:
            seat = int(args.character == 'zero')
            peers[seat].input(SQUARE)
            ticks(120)
            rows = [Runtime(peer.tcp).personal_effects() for peer in peers]
            report['personal_effects'] = []
            for peer, row in zip(peers, rows):
                body = row['bodies'][seat]
                active = [actor for actor in row['glow'][seat] if actor[3]]
                require(body[0x95] in (0x12, 0x14) and active,
                        'Native X full charge has no active personal glow')
                x, y = struct.unpack_from('<ii', body, 8)
                require(all(abs(struct.unpack_from('<i', actor, 8)[0]-x) < 65536 and
                            abs(struct.unpack_from('<i', actor, 12)[0]-y) < 65536 for actor in active),
                        'Network charge glow follows the other hero')
                report['personal_effects'].append(dict(seat=peer.seat, x_seat=seat,
                    body=body.hex(), glow=[actor.hex() for actor in row['glow'][seat]]))
                peer.screenshot(peer.directory/'native-charge.png')
            last=checkpoint('Active native personal glow',last)
            peers[seat].input(0)
            ticks(40)
            require(all(not any(actor[3] for actor in Runtime(peer.tcp).personal_effects()['glow'][seat])
                        for peer in peers), 'Native charge glow persisted after release')
            last=checkpoint('Native personal glow release',last)
        if args.split_withdrawal:
            inventory=[]
            for peer in peers:
                data=peer.read(peer.diagnostic+0x100,0x158)
                inventory.append((data[0x5c],data[0xa8:0xc8]))
            held_start=observe()
            peers[1].input(1)
            deadline=time.monotonic()+45
            while not all(s['lifecycle'][1] in (1,2) for s in observe()):
                require(time.monotonic()<deadline,'Native Select hold did not begin departure')
                ticks(8)
            held_end=observe()
            peers[1].input(0)
            report['withdrawal_hold'] = dict(before=held_start, after=held_end)
            require(all(b['world']-a['world']>=90 for a,b in zip(held_start,held_end)),
                    'Departure began before the native90-update Select threshold')
            deadline=time.monotonic()+25
            while not all(s['lifecycle'][1]==2 for s in observe()):
                require(time.monotonic()<deadline,'Native departure did not complete');ticks(8)
            ticks(8)
            for peer in peers:
                counters={r['name']:int(r['count']) for r in peer.request('mod_counters')['counters']}
                layers=peer.read(counters['mmx5.coop.split-state']+16,2*0xfc)
                require(layers[:0xfc]==layers[0xfc:],'Absent seat did not adopt survivor camera')
                peer.screenshot(peer.directory/'absent-spectator.png')
            last=checkpoint('Native absence and spectator view',last)
            pulse(1,1,6)
            deadline=time.monotonic()+25
            while not all(s['lifecycle'][1]==0 for s in observe()):
                require(time.monotonic()<deadline,'Native returning arrival did not complete');ticks(8)
            for peer,previous in zip(peers,inventory):
                data=peer.read(peer.diagnostic+0x100,0x158)
                require((data[0x5c],data[0xa8:0xc8])==previous,'Return changed native private inventory')
                peer.screenshot(peer.directory/'returned-own-view.png')
            last=checkpoint('Native return and private resources',last)
        for seat in (1,0):
            pulse(START,seat)
            ticks(60)
            deadline=time.monotonic()+25
            while not all(peer.read(MENU+4,1)==b'\x01' for peer in peers):
                require(time.monotonic()<deadline, 'Native menu did not become interactive')
                ticks(4)
            before=observe()
            require(all(s['minor']==2 and s['owner']==seat+1 for s in before),'Wrong native menu owner')
            for peer in peers:
                peer.screenshot(peer.directory/f'menu-p{seat+1}.png')
            ticks(40)
            after=observe()
            require(all(a['world']==b['world'] and a['timer']==b['timer'] for a,b in zip(before,after)),
                    'Menu did not freeze the world')
            pulse(RIGHT,seat^1)
            require(all(s['minor']==2 for s in observe()),'Other seat disturbed the menu')
            pulse(START,seat)
            ticks(60)
            deadline=time.monotonic()+25
            while not all(s['minor']==0 for s in observe()):
                require(time.monotonic()<deadline, 'Native menu failed to close')
                ticks(4)
            last=checkpoint(f'P{seat+1} menu and restore',last)
        if args.ultimate_nova:
            from test_mmx5_coop_scene_fatal_runtime import serial
            runtimes=[Runtime(peer.tcp) for peer in peers]
            def nova_observe():
                rows=[]
                for peer,runtime in zip(peers,runtimes):
                    z=runtime.personal_effects()
                    camera=peer.read(peer.diagnostic+0x710,0x1F8)
                    origins=[[struct.unpack_from('<h',camera,seat*0xFC+offset)[0]
                              for offset in (10,14)] for seat in (0,1)]
                    rows.append(dict(personal=serial(z),cameras=origins,
                                     play=peer.read(PLAY,0x30).hex(),stats=peer.request('render_pass_stats')))
                return rows
            # Use the same bounded authored road/jump route as separation QA.
            for step in range(20):
                before_move=observe();pulse(RIGHT,1,20);after_move=observe()
                if all(b['p2']['x']-a['p2']['x']<5 for a,b in zip(before_move,after_move)):
                    pulse(RIGHT|CROSS,1,45)
                if all(s['p2']['x']-s['p1']['x']>480 for s in observe()):break
            deadline=time.monotonic()+30
            while True:
                rows=nova_observe()
                if all(all(body[4:6]==bytes((1,2)) and not body[0x67] and body[0x70]&8
                           for body in (bytes.fromhex(value) for value in row['personal']['bodies']))
                       for row in rows):break
                # Native hurt11 can already contact the floor while controls
                # are still unavailable. Wait for actual idle recovery too.
                require(time.monotonic()<deadline,'Native pair did not land/recover before Nova');ticks(2)
            report['ultimate_nova']=dict(before=rows,setup='Original controller movement only; no state/position/energy writes',
                form_source='automatic co-op options' if args.automatic_forms else 'original native title code')
            energy=[]
            for row in rows:
                body=bytes.fromhex(row['personal']['bodies'][0])
                require(body[0xFA]==4 and abs(row['cameras'][1][0]-row['cameras'][0][0])>400,
                        'Native Ultimate/separated-view prerequisite missing')
                value=struct.unpack_from('<H',body,0xA8)[0]
                require(value==struct.unpack_from('<H',body,0x148)[0]>0,'Native Giga not full')
                energy.append(value)
            peers[0].input(0x0200)
            deadline=time.monotonic()+10
            while True:
                rows=nova_observe()
                if all(bytes.fromhex(row['personal']['bodies'][0])[5]==0x22 for row in rows):break
                require(time.monotonic()<deadline,'Native network Nova not admitted');ticks(1)
            peers[0].input(0)
            report['ultimate_nova']['admitted']=rows
            y_before=[struct.unpack_from('<i',bytes.fromhex(row['personal']['bodies'][1]),12)[0] for row in rows]
            peers[1].input(CROSS);ticks(6);active=nova_observe();peers[1].input(0)
            report['ultimate_nova']['active']=active
            for old,row,y in zip(report['ultimate_nova']['before'],active,y_before):
                p=bytes.fromhex(row['play']);bodies=[bytes.fromhex(body) for body in row['personal']['bodies']]
                require(bodies[0][5]==0x22 and p[0x1C] and row['personal']['scene']==[0,0],
                        'Missed actual native Nova window')
                require(struct.unpack_from('<i',bodies[1],12)[0]<y and bodies[1][0x67],
                        'Nova froze network partner jump')
                require(abs(row['cameras'][1][0]-row['cameras'][0][0])>400 and
                        abs(row['cameras'][1][0]-old['cameras'][1][0])<32,'Nova reset partner camera')
                require(row['stats']['verify_checks']>old['stats']['verify_checks'],'No private draw during Nova')
            for peer in peers:peer.screenshot(peer.directory/'native-Nova-own-view.png')
            deadline=time.monotonic()+30
            while True:
                rows=nova_observe()
                if all(bytes.fromhex(row['personal']['bodies'][0])[4:6]==bytes((1,2)) and
                       not bytes.fromhex(row['play'])[0x1C] for row in rows):break
                require(time.monotonic()<deadline,'Native Nova did not recover');ticks(2)
            report['ultimate_nova']['released']=rows
            require([struct.unpack_from('<H',bytes.fromhex(row['personal']['bodies'][0]),0xA8)[0]
                     for row in rows]==energy,'Native Ultimate Giga energy changed')
            last=checkpoint('Native Ultimate Nova, independent partner jump and private cameras',last)
        if args.scene_fatal:
            from test_mmx5_coop_scene_fatal_runtime import serial
            leader=args.scene_owner;survivor=leader^1
            runtimes = [Runtime(peer.tcp) for peer in peers]
            diagnostics = [runtime.diagnostic() for runtime in runtimes]
            def command(number):
                before = [int.from_bytes(peer.read(address+0x80,4),'little')
                          for peer,address in zip(peers,diagnostics)]
                pulse(6|(number<<8),count=12)
                deadline = time.monotonic()+30
                while True:
                    receipts = [struct.unpack('<III',peer.read(address+0x80,12))
                                for peer,address in zip(peers,diagnostics)]
                    if all(row[0] == old+1 for row,old in zip(receipts,before)):
                        require(receipts[0] == receipts[1] and receipts[0][1] == number and receipts[0][2],
                                f'Replicated qualification command rejected/disagreed: {number} {receipts}')
                        report['scene_fatal']['commands'].append(dict(command=number,receipts=receipts))
                        return
                    require(time.monotonic() < deadline, 'Replicated qualification command was not acknowledged')
                    ticks(4)
            def native_arrival(previous, confirm_mission_intro=False):
                deadline = time.monotonic()+150
                while True:
                    states = observe()
                    counts = [next(row['count'] for row in peer.request('mod_counters')['counters']
                                   if row['name'] == 'mmx5.coop.enrolled') for peer in peers]
                    if all(count>old and state['mode'] == 10 and state['stage'] == 1 and
                           state['minor'] == 0 and not state['gate'] and state['lifecycle'][:2] == [0,0]
                           for state,count,old in zip(states,counts,previous)):
                        return
                    require(time.monotonic() < deadline, 'Native Grizzly qualification arrival did not finish')
                    if any(state['mode'] in (4,5,6,20) or
                           (confirm_mission_intro and state['mode'] == 10 and state['gate'])
                           for state in states):pulse(CROSS,count=6)
                    else:ticks(4)
            def enrollment_counts():
                return [next(row['count'] for row in peer.request('mod_counters')['counters']
                             if row['name'] == 'mmx5.coop.enrolled') for peer in peers]
            def handoffs():
                return [next((row['count'] for row in peer.request('mod_counters')['counters']
                              if row['name'] == 'mmx5.coop.dead-scene-owner-handoffs'),0) for peer in peers]
            report['scene_fatal'] = dict(commands=[],leader=leader,harness_sha256=sha256_file(Path(__file__)),
                bypassed_prerequisites=['Replicated HP-refill qualification command',
                    'Original Grizzly mission and checkpoint4 loader; preceding door bypassed',
                    'Replicated owner fatal-HP qualification command'],
                limits='Same-build loopback native boss script/owner recovery only; no door, defeat, reward or Internet pass.')
            require(all(any(row['name'] == 'mmx5.coop.test-cheats-enabled' and row['count'] == 1
                            for row in peer.request('mod_counters')['counters']) for peer in peers),
                    'Scene-owner fixture requires a qualification build')
            command(4)
            previous = enrollment_counts();command(12);native_arrival(previous,confirm_mission_intro=True)
            if args.shared_scroll:
                both('input',0)
                scroll=dict(samples=[],wraps=[[False,False]for _ in peers],
                            clock_steps=[[0,0]for _ in peers],
                            bypasses=['Existing replicated original Grizzly mission loader and HP-only refill.'],
                            limits='Stationary CP0 clock; diagnostic world/camera observations can straddle one native update. No road traversal or distant-view claim.')
                report['shared_scroll']=scroll
                started=None;deadline=time.monotonic()+120
                while not all(any(row['name']=='mmx5.coop.shared-scroll-published' and row['count']>0
                                  for row in peer.request('mod_counters')['counters'])for peer in peers):
                    require(time.monotonic()<deadline,'Native train producer did not become active')
                    ticks(4)
                for peer in peers:peer.screenshot(peer.directory/'shared-scroll-start.png')
                while True:
                    rows=[]
                    for peer,diag in zip(peers,diagnostics):
                        # One cached diagnostic boundary contains both saved
                        # planes and its world counter. Do not compare RAM
                        # reads from different peers as atomic world states.
                        data=peer.read(diag,0x908)
                        origins=[[struct.unpack_from('<i',data,0x710+seat*0xFC+off)[0]
                                  for off in (0x5C,0xB0)]for seat in (0,1)]
                        require(data[0x40:0x42]==bytes(2),'Train clock lost a living hero')
                        require(origins[0]==origins[1],'Shared train origins disagree between private views')
                        rows.append(dict(world=struct.unpack_from('<I',data,24)[0],origins=origins))
                    if started is None:started=[row['world']for row in rows]
                    previous=scroll['samples'][-1]if scroll['samples']else None
                    scroll['samples'].append(rows)
                    if previous:
                        for index,(old,row)in enumerate(zip(previous,rows)):
                            delta=row['world']-old['world']
                            require(0<=delta<128,'Native train observation undersampled its clock')
                            for axis,(speed,period)in enumerate(((8,1024),(4,512))):
                                before,after=old['origins'][0][axis],row['origins'][0][axis]
                                advance=(after-before)%(period*65536)
                                require(advance%(speed*65536)==0,'Native train origin lost its authored step')
                                steps=advance//(speed*65536)
                                scroll['clock_steps'][index][axis]+=steps
                                # The cached counter can be updated before
                                # its saved camera. Bound that phase offset
                                # over the whole window as well as each read,
                                # so the allowance cannot accumulate drift.
                                require(abs(steps-delta)<=1 and
                                        abs(scroll['clock_steps'][index][axis]-(row['world']-started[index]))<=1,
                                        'Native train clock accumulated missing or extra updates')
                                if after<before:scroll['wraps'][index][axis]=True
                    if all(row['world']-start>=150 for row,start in zip(rows,started)):break
                    require(time.monotonic()<deadline,'Native train observation deadline')
                    ticks(4)
                require(all(all(wraps)for wraps in scroll['wraps']),'No actual native layer wrap observed')
                for peer in peers:
                    counters={row['name']:row['count']for row in peer.request('mod_counters')['counters']}
                    require(counters.get('mmx5.coop.shared-scroll-published',0)>0,'Shared scroll producer did not publish')
                    peer.screenshot(peer.directory/'shared-scroll-native-wrap.png')
                last=checkpoint('Native shared train origins and panorama wraps',last)
            previous = enrollment_counts();command(2);native_arrival(previous)
            both('input',0)
            before = handoffs()
            peers[leader].input(RIGHT)
            deadline = time.monotonic()+60
            while True:
                rows = [runtime.personal_effects() for runtime in runtimes]
                if any(row['scene'][0] for row in rows):peers[leader].input(0)
                if all(row['scene'] == [leader+1,3] for row in rows):break
                require(all(row['life'][:2] == [0,0] for row in rows), 'Native approach lost a living hero')
                require(time.monotonic() < deadline, 'Requested hero did not acquire the original boss scene')
                ticks(4)
            require(all(row['bodies'][survivor][4] == 3 and not row['bodies'][survivor][3] for row in rows),
                    'Native scene did not establish a hidden living passenger')
            report['scene_fatal']['locked'] = serial(rows)
            retries = [peer.read(PLAY+0x45,1) for peer in peers]
            flags = [peer.read(PLAY+0x4C,8) for peer in peers]
            last = checkpoint(f'Original P{leader+1} boss lock and living passenger',last)
            command(5+leader)
            deadline = time.monotonic()+150
            while True:
                rows = [runtime.personal_effects() for runtime in runtimes]
                require([peer.read(PLAY+0x45,1) for peer in peers] == retries,
                        'Living scene passenger caused a team retry')
                require(all(row['life'][survivor] == 0 for row in rows), 'Native scene lost its living passenger')
                states = observe()
                if all(row['scene'] == [0,0] and row['life'][leader] == 5 and row['life'][survivor] == 0 and
                       row['bodies'][survivor][4:6] == bytes((1,2)) and not state['gate'] and state['minor'] == 0
                       for row,state in zip(rows,states)):
                    break
                require(time.monotonic() < deadline, 'Original boss controller did not release the survivor')
                if any(state['gate'] for state in states):pulse(CROSS,survivor,6)
                else:ticks(4)
            require(all(after-old == 1 for after,old in zip(handoffs(),before)),
                    'Native boss owner did not hand control over exactly once on both peers')
            require([peer.read(PLAY+0x4C,8) for peer in peers] == flags,
                    'Scene owner death changed boss completion outcomes')
            report['scene_fatal']['released'] = serial(rows)
            before_move = observe();pulse(RIGHT,survivor,30);after_move = observe()
            key=f'p{survivor+1}'
            require(all(after[key]['x'] > before[key]['x']+1 for before,after in zip(before_move,after_move)),
                    'Survivor did not regain original movement')
            report['scene_fatal']['handoffs'] = [after-old for after,old in zip(handoffs(),before)]
            last = checkpoint('Native boss owner fatal, survivor and controller release',last)
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
                    rows.append(dict(seat=peer.seat,cameras=cameras,camera_bytes=layers.hex(),
                                     canonical_camera=peer.read(0x8009A1F8,0xFC).hex(),enemies=enemies,
                                     common_callbacks=[hex(v) for v in struct.unpack('<8I',peer.read(common,32))],
                                     players=peer.observe(),counters=counters,stats=peer.request('render_pass_stats')))
                    peer.screenshot(peer.directory/f'{label}.png')
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
            performance('separated')
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
            peer.screenshot(peer.directory/'final.png')
        if args.automatic_forms:
            final_forms=observe()
            report['automatic_forms']['final_observations']=final_forms
            require(all(not row['genuine_flags4A']&0x18 for row in final_forms),
                    'Combined regression persisted automatic form cheat flags')
        if args.cameras == 'split':
            report['final_view_probes'] = []
            for peer in peers:
                stats = peer.request('render_pass_stats')
                counters = {row['name']: int(row['count']) for row in peer.request('mod_counters')['counters']}
                report['final_view_probes'].append(dict(seat=peer.seat, stats=stats, counters=counters))
                require(not counters.get('mmx5.coop.local-view-state-leak', 0),
                        'Later private draw changed canonical host state')
                require(all(not stats.get(key, 0) for key in
                            ('verify_mismatch', 'watchdog', 'vram_leaks', 'device_reads')),
                        f'Later own-view sandbox fault: {stats}')
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

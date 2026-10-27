"""Observe and control a private X5 development process through its debug port.

Boot navigation and gameplay use controller input. No process is launched,
foregrounded, or stopped by this module.
"""
import json
from pathlib import Path
import socket
import struct
import time

PLAYER = 0x8009A0A0
PLAY = 0x800D1C00


class Runtime:
    def __init__(self, port):
        self.port = port
        self._diagnostic = None

    def request(self, command, **fields):
        with socket.create_connection(('127.0.0.1', self.port), timeout=10) as sock:
            sock.sendall((json.dumps(dict(id=1, cmd=command, **fields))+'\n').encode())
            result = json.loads(sock.makefile().readline())
        if not result.get('ok'):
            raise RuntimeError(result)
        return result

    def read(self, address, size):
        return bytes.fromhex(self.request('read_ram', addr=hex(address), len=size)['hex'])

    def input(self, pressed=0, seat=0):
        return self.request('set_input', buttons=hex(0xffff ^ pressed), port=seat+1)

    def fixture(self, command, seat=0, hp=0, ammo=0):
        address = self.diagnostic()
        sequence = struct.unpack('<I', self.read(address+0x50, 4))[0]+1
        if sequence>255:
            raise RuntimeError('Restart the private process after 255 fixture requests')
        self.request('write_ram', addr=hex(address+0x54), val=hex(command))
        self.request('write_ram', addr=hex(address+0x55), val=hex(seat))
        self.request('write_ram', addr=hex(address+0x56), val=hex(hp))
        self.request('write_ram', addr=hex(address+0x58), val=hex(ammo&255))
        self.request('write_ram', addr=hex(address+0x59), val=hex(ammo>>8))
        self.request('write_ram', addr=hex(address+0x50), val=hex(sequence))
        deadline = time.monotonic()+8
        while struct.unpack('<I', self.read(address+0x60, 4))[0]!=sequence:
            if time.monotonic()>deadline:
                raise TimeoutError('Native world boundary did not acknowledge fixture')
            time.sleep(.02)

    def load_location(self, stage, section, checkpoint, continuation=False):
        """Bypass mission selection using the original title/loader and record.

        This establishes setup only. A loaded location never passes traversal,
        checkpoint activation, boss defeat or an actual exit transition.
        """
        self.fixture(14 if continuation else 9, hp=stage, ammo=(section << 8) | checkpoint)
        if not int.from_bytes(self.read(self.diagnostic()+0x64, 4), 'little'):
            raise RuntimeError('Native location request rejected')

    def teleport(self, seat, x, y):
        """Position/history only; preserve native gravity and control state."""
        payload=struct.pack('<ii', round(x*65536), round(y*65536))
        for i,value in enumerate(payload):
            self.request('write_ram', addr=hex(self.diagnostic()+0x90+i), val=hex(value))
        self.fixture(10, seat=seat)
        if not int.from_bytes(self.read(self.diagnostic()+0x64, 4), 'little'):
            raise RuntimeError('Native teleport request rejected')

    def teleport_pair(self, positions, camera):
        """Move both on-foot bodies/history and main camera in one world tick.

        Offline debug setup only: camera must fit every current native view's
        authored bounds; active scenes, mounts and pending exits are rejected.
        Preserve velocity, contacts, controls and all progression flags. This
        bypasses traversal and proves no camera, entry or transition behavior.
        Secondary/parallax layers are left for their original native update.
        """
        positions=tuple(tuple(position) for position in positions)
        camera=tuple(camera)
        if len(positions)!=2 or any(len(position)!=2 for position in positions) or len(camera)!=2:
            raise ValueError('Expected two (x, y) positions and one (x, y) camera')
        payload=struct.pack('<6i',*(round(value*65536) for pair in (*positions,camera) for value in pair))
        if self.request('netplay_status')['active']:
            raise RuntimeError('Paired teleport setup is offline only')
        address=self.diagnostic()
        # +A4 is live door diagnostics: keep camera Y in the unused +B4.
        for i,value in enumerate(payload):
            offset=0x90+i if i<20 else 0xB4+i-20
            self.request('write_ram', addr=hex(address+offset), val=hex(value))
        self.fixture(16)
        if not int.from_bytes(self.read(address+0x64,4),'little'):
            raise RuntimeError('Native paired teleport request rejected: require live unlocked on-foot heroes and camera within current bounds')

    def lifecycle(self):
        return tuple(self.read(self.diagnostic()+0x40, 4))

    def rescan_placements(self, left, right, top, bottom):
        """Load original placements after teleporting past viewport admission.

        Offline setup only. The native scanner retains allocation/cooldown
        checks; this proves no ordinary entry, actor interaction or reward.
        """
        if not (0 <= left <= right <= 32767 and 0 <= top <= bottom <= 32767
                and right-left <= 512 and bottom-top <= 480):
            raise ValueError('Expected a small nonnegative native placement rectangle')
        if self.request('netplay_status')['active']:
            raise RuntimeError('Placement rescan setup is offline only')
        for i, value in enumerate(struct.pack('<4i', left, right, top, bottom)):
            self.request('write_ram', addr=hex(self.diagnostic()+0x90+i), val=hex(value))
        self.fixture(17)
        if not int.from_bytes(self.read(self.diagnostic()+0x64, 4), 'little'):
            raise RuntimeError('Native placement rescan rejected: require unlocked on-foot gameplay')

    def teleport_car(self,seat,x,y):
        """Move an existing rider/car together; never set contacts or success."""
        payload=struct.pack('<ii',round(x*65536),round(y*65536))
        for i,value in enumerate(payload):
            self.request('write_ram',addr=hex(self.diagnostic()+0x90+i),val=hex(value))
        self.fixture(15,seat=seat)
        if not int.from_bytes(self.read(self.diagnostic()+0x64,4),'little'):
            raise RuntimeError('Native car-position request rejected')

    def release(self):
        self.request('clear_input')
        self.request('clear_input', port=2)

    def tap(self, pressed, seconds=.15, seat=0):
        self.input(pressed, seat)
        time.sleep(seconds)
        self.input(0, seat)

    def confirm_native_text(self, timeout=12, seat=0):
        """Confirm a ready original story page with its owning controller.

        Original23CC0/23D78 waits for text state FF/3 and a fresh native pad
        edge. Short pulses during printing can miss the page entirely. The
        caller owns the surrounding story/selector transition and deadline;
        this helper does not confirm inventory/save/character choices.
        Frontends use P1 by default; an in-world P2-owned scene needs seat1.
        """
        if seat not in (0, 1):
            raise ValueError('Native dialogue controller must be seat0 or seat1')
        deadline = time.monotonic()+timeout

        def wait(predicate, message):
            while not predicate():
                if time.monotonic() > deadline:
                    raise TimeoutError(message)
                time.sleep(.03)

        self.input(0, seat)
        wait(lambda: self.read(0x80091C44, 2) == b'\xff\x03',
             'Original story page did not become ready')
        if not seat:
            wait(lambda: not self.read(0x800C931C, 1)[0] & 0x40,
                 'Original story pad release was not acknowledged')
        else:
            # P2 input is projected only during its native world pass. The
            # canonical PAD read can show P1 between passes; let the released
            # P2 value reach native polling before requesting a fresh edge.
            released = self.request('frame')['frame']
            wait(lambda: self.request('frame')['frame'] >= released+2,
                 'Original world did not poll the released dialogue pad')
        before = self.read(0x80091C38, 16)
        self.input(0x4000, seat)
        try:
            wait(lambda: self.read(0x80091C38, 16) != before,
                 f'Original story page did not consume P{seat+1} Cross')
            after = self.read(0x80091C38, 16)
        finally:
            self.input(0, seat)
        return dict(seat=seat, button='Cross', before=before.hex(), after=after.hex())

    def resume_native_pause(self):
        if self.read(PLAY+1,1)!=b'\x02':
            return
        # Native menu initialization ignores Start during its opening fade.
        deadline=time.monotonic()+8
        while self.read(0x800D4F44,1)!=b'\x01':
            if self.read(PLAY+1,1)!=b'\x02':return
            if time.monotonic()>deadline:raise TimeoutError('Native menu did not become interactive')
            time.sleep(.03)
        seat=max(0,self.read(self.diagnostic()+0x48,1)[0]-1)
        self.tap(8,.10,seat)
        retry_at=time.monotonic()+.4
        while self.read(PLAY+1,1)==b'\x02':
            if time.monotonic()>deadline:raise TimeoutError('Native menu did not close')
            # A short wall-clock pulse can miss the native pad poll during
            # slow frames. Retry only while the original menu is interactive;
            # its closing fade must finish without another Start edge.
            if time.monotonic()>retry_at and self.read(0x800D4F44,1)==b'\x01':
                self.tap(8,.10,seat);retry_at=time.monotonic()+.4
            time.sleep(.03)

    def screenshot(self, path):
        path=Path(path).resolve()
        try:
            result=self.present_screenshot(path)
            surface='composed_host_present'
        except RuntimeError as error:
            if 'present_shot unavailable' not in str(error):
                raise
            result=self.render_screenshot(path)
            surface='game_render_buffer_headless'
        metadata=dict(surface=surface,completed=path.is_file(),response=result)
        path.with_suffix('.capture.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf-8')
        return metadata

    def render_screenshot(self, path):
        """Explicit pre-fit game buffer; excludes host composition/OSD."""
        deadline = time.monotonic()+3
        while True:
            gpu = self.request('gpu_state')
            command = 'screenshot' if gpu.get('depth24') else 'screenshot_hires'
            try:
                return self.request(command, path=str(Path(path).resolve()))
            except RuntimeError as error:
                # Original menu/stage fades can briefly disable the display.
                # Wait for an actual frame without changing input or game state.
                if 'display disabled' not in str(error) or time.monotonic()>deadline:
                    raise
                time.sleep(.03)

    def present_screenshot(self, path):
        """Capture fitted host output and wait for the actual written file."""
        path=Path(path).resolve()
        sequence=self.request('present_shot_seq')['seq']
        self.request('present_shot',path=str(path))
        deadline=time.monotonic()+5
        while True:
            result=self.request('present_shot_seq')
            if result['seq']!=sequence:
                if not result.get('wrote') or not path.is_file():
                    raise RuntimeError('Host present capture completed without writing a file')
                return result
            if time.monotonic()>deadline:
                raise TimeoutError('Host present capture did not complete')
            time.sleep(.025)

    def diagnostic(self):
        if self._diagnostic is not None:
            return self._diagnostic
        counters = self.request('mod_counters')['counters']
        for counter in counters:
            if counter['name'] == 'mmx5.coop.diagnostic-address':
                self._diagnostic = counter['count']
                return self._diagnostic
        raise RuntimeError('X5 co-op is inactive')

    def players(self):
        data = self.read(self.diagnostic(), 0x458)
        return data[0x300:0x458], data[0x100:0x258]

    def cars(self):
        data = self.read(self.diagnostic()+0x480, 0x170)
        return data[:0xB0], data[0xC0:0x170]

    def personal_effects(self):
        """One coherent observation of native bodies, personal pools and clocks."""
        if not getattr(self,'_personal_effects_schema',False):
            counters=self.request('mod_counters')['counters']
            if not any(c['name']=='mmx5.coop.personal-effects-diagnostic-version' and c['count']>=1 for c in counters):
                raise RuntimeError('This binary has no personal-effects diagnostics')
            self._personal_effects_schema=True
        data=self.read(self.diagnostic(),0xE88)
        keys=('ticks','ready','enrolled','failed','counterpart','banks','world_ticks','native_timer')
        return dict(state=dict(zip(keys,struct.unpack_from('<8I',data))),life=list(data[0x40:0x44]),
                    scene=list(data[0x4c:0x4e]),
                    bodies=[data[0x300:0x458],data[0x100:0x258]],
                    glow=[[data[base+i*0x50:base+(i+1)*0x50] for i in range(3)] for base in (0xA00,0xB00)],
                    trails=[[data[base+i*0x60:base+(i+1)*0x60] for i in range(3)] for base in (0xC00,0xD40)],
                    native_effect_ticks=list(struct.unpack_from('<2I',data,0xE80)))

    def state(self):
        data = self.read(self.diagnostic(), 32)
        keys = ('ticks', 'ready', 'enrolled', 'failed', 'counterpart', 'banks', 'world_ticks', 'native_timer')
        return dict(zip(keys, struct.unpack('<8I', data)))

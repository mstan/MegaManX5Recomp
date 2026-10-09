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

    def lifecycle(self):
        return tuple(self.read(self.diagnostic()+0x40, 4))

    def release(self):
        self.request('clear_input')
        self.request('clear_input', port=2)

    def tap(self, pressed, seconds=.15, seat=0):
        self.input(pressed, seat)
        time.sleep(seconds)
        self.input(0, seat)

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
        while self.read(PLAY+1,1)==b'\x02':
            if time.monotonic()>deadline:raise TimeoutError('Native menu did not close')
            time.sleep(.03)

    def screenshot(self, path):
        gpu = self.request('gpu_state')
        command = 'screenshot' if gpu.get('depth24') else 'screenshot_hires'
        return self.request(command, path=str(Path(path).resolve()))

    def diagnostic(self):
        counters = self.request('mod_counters')['counters']
        for counter in counters:
            if counter['name'] == 'mmx5.coop.diagnostic-address':
                return counter['count']
        raise RuntimeError('X5 co-op is inactive')

    def players(self):
        data = self.read(self.diagnostic(), 0x458)
        return data[0x300:0x458], data[0x100:0x258]

    def state(self):
        data = self.read(self.diagnostic(), 32)
        keys = ('ticks', 'ready', 'enrolled', 'failed', 'counterpart', 'banks', 'world_ticks', 'native_timer')
        return dict(zip(keys, struct.unpack('<8I', data)))

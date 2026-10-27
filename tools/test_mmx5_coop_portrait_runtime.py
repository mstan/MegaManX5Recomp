"""Read native earned portrait tiles and one real selector cursor refresh.

Uses the original selector's source/destination tables and 27514 tile layout.
Never writes tiles, palettes, clear bits, menu phases or camera state.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time

import psutil

from coop_debug import Runtime, PLAY
from test_mmx5_coop_grizzly_clear_runtime import require


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--stage', type=int, choices=range(1, 9), required=True)
    args = parser.parse_args()
    parent = json.loads(args.session.read_text(encoding='utf-8'))
    exe = Path(parent['command'][0]).resolve()
    require(Path(psutil.Process(parent['pid']).exe()).resolve() == exe, 'Not owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest() == parent['executable_sha256'], 'Binary changed')
    r = Runtime(parent['port'])
    require(not r.request('netplay_status')['active'], 'Private selector check only')
    args.out.mkdir(parents=True, exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    report = dict(success=False, human_validation=False, stage=args.stage,
                  executable_sha256=parent['executable_sha256'], samples=[], input=[],
                  limits='Earned portrait tile selection/cursor refresh; composed rendering requires separate visual review. No all-boss or cold-reload pass.')

    def header():
        p = r.read(PLAY, 0x1C0)
        require(p[:3] == b'\x04\x04\x00' and p[12:14] == b'\x0D\x00' and
                not r.read(0x8009A424, 1)[0] and not r.read(0x80091C44, 1)[0], 'Not the actual stable native selector')
        require(r.read(0x800EE970, 4) == b'\x18\x00\x00\x00', 'Wrong selector overlay')
        return p

    def pulse(button):
        report['input'].append(dict(seat=0, button=hex(button), cursor=r.read(PLAY+0x28, 1)[0]))
        for value, count in ((button, 6), (0, 8)):
            r.input(value, 0)
            frame = r.request('frame')['frame']
            deadline = time.monotonic()+10
            while r.request('frame')['frame'] < frame+count:
                require(time.monotonic() < deadline, 'Native cursor stopped')
                time.sleep(.02)

    def sample(label, rects, destination):
        p = header()
        roots = r.read(0x1F800004, 8)
        layout, tiles = struct.unpack('<II', roots)
        stride = struct.unpack('<H', r.read(0x80091D58, 2))[0]
        width, height = p[0x1BC:0x1BE]
        require(0x80010000 <= layout < 0x80200000 and 0x80010000 <= tiles < 0x80200000 and
                width and height and stride, 'Invalid native tile roots/dimensions')
        def rectangle(x, y):
            words = []
            for row in range(4):
                for col in range(4):
                    tx, ty = x+16*col, y+16*row
                    require(tx//256 < width and ty//256 < height, 'Portrait outside original layout')
                    cell = layout+stride+(ty//256)*width+tx//256
                    screen = r.read(cell, 1)[0]
                    address = tiles+screen*512+((ty & 240)//16)*32+((tx & 240)//16)*2
                    require(address < 0x801FFFFE, 'Tile word outside RAM')
                    words.append(dict(x=tx, y=ty, layout_address=hex(cell), screen=screen,
                                      address=hex(address), value=struct.unpack('<H', r.read(address, 2))[0]))
            return words
        result = dict(label=label, play=p.hex(), layout=hex(layout), tiles=hex(tiles), stride=stride,
                      dimensions=[width, height], camera=r.read(0x8009A1F8, 0xFC).hex(),
                      destination=rectangle(*destination), normal=rectangle(*rects[0][:2]),
                      cleared=rectangle(*rects[1][:2]))
        after = header()
        require(after[:4] == p[:4] and after[0x28] == p[0x28] and after[0x44:0x4D] == p[0x44:0x4D] and
                r.read(0x1F800004, 8) == roots, 'Selector changed during tile observation')
        values = {k: [w['value'] for w in result[k]] for k in ('destination', 'normal', 'cleared')}
        require(values['normal'] != values['cleared'], 'Authored variants have no nonvacuous tile discriminator')
        require(values['destination'] == values['cleared'], 'Earned portrait did not select native cleared art')
        r.screenshot(args.out/f'{label}.png')
        report['samples'].append(result)

    try:
        r.release()
        before = header()
        require(before[0x4C] & (1 << (args.stage-1)), 'Portrait must be earned by the actual game')
        stages = r.read(0x800F5050, 8)
        require(set(stages) == set(range(1, 9)), 'Unexpected original portrait stage table')
        index = stages.index(args.stage)
        rects = [struct.unpack('<hhhh', r.read(base+index*8, 8)) for base in (0x800F5058, 0x800F50B0)]
        require(all(rect[2:] == (4, 4) for rect in rects), 'Unexpected authored portrait rectangle')
        destination = struct.unpack('<hh', r.read(0x800F5108+index*4, 4))
        report['native_tables'] = dict(stage_order=list(stages), index=index, normal=rects[0],
                                       cleared=rects[1], destination=destination)
        if header()[0x28] == 8:
            # Dynamo can own the center selection after real progression;
            # directions do not unlock it. Retain a read-only art check and
            # leave refresh unverified until the native encounter completes.
            sample('earned-during-native-encounter', rects, destination)
            report['tile_selection_status'] = 'passed bounded native cleared art'
            report['cursor_refresh_status'] = 'unverified: mandatory native encounter owns selection'
            raise AssertionError('Native encounter owns selection; no cursor refresh attempted')
        for _ in range(8):
            cursor = header()[0x28]
            require(cursor < 8, 'Invalid native grid cursor')
            if cursor == index:
                break
            pulse((0x20 if index >= 4 else 0x80) if cursor//4 != index//4 else
                  (0x40 if cursor%4 < index%4 else 0x10))
        require(header()[0x28] == index, 'Native direction input did not reach earned portrait')
        sample('earned-before-refresh', rects, destination)
        pulse(0x10 if index%4 else 0x40)
        require(header()[0x28] != index, 'Cursor never left earned portrait')
        pulse(0x40 if index%4 else 0x10)
        require(header()[0x28] == index, 'Cursor did not return natively')
        sample('earned-after-refresh', rects, destination)
        require(header()[0x4C] == before[0x4C] and header()[0x44] == before[0x44], 'Cursor changed campaign/progress')
        report['success'] = True
    except Exception as error:
        report['error'] = str(error)
    finally:
        r.release()
        (args.out/'report.json').write_bytes((json.dumps(report, indent=2)+'\n').encode('utf-8'))
    print(json.dumps({k: report.get(k) for k in ('success', 'error', 'native_tables')}), flush=True)
    return 0 if report['success'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

"""Disassemble original X5 EXE or a proven ROCK_X5.BIN overlay, without writes.

Requires Capstone. BIN destinations follow the original loader evidence in
docs/AOT_OVERLAYS.md; members 38/39 deliberately have no inferred destination.
Example: --bin build-coop/ROCK_X5.BIN --member 6 0x800f2a4c:80
"""
import argparse
from pathlib import Path
import struct

from capstone import Cs, CS_ARCH_MIPS, CS_MODE_MIPS32, CS_MODE_LITTLE_ENDIAN


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    source = parser.add_mutually_exclusive_group(required=True)
    source.add_argument('--exe', type=Path)
    source.add_argument('--bin', type=Path)
    parser.add_argument('--member', type=int)
    parser.add_argument('--words', action='store_true', help='Read pointer/data words instead of instructions')
    parser.add_argument('ranges', nargs='+', help='Address:word-count, e.g. 0x80022d98:64')
    args = parser.parse_args()
    data = (args.exe or args.bin).read_bytes()
    if args.exe:
        if data[:8] != b'PS-X EXE' or args.member is not None:
            parser.error('EXE needs a PS-X EXE header and no --member')
        base, size = struct.unpack_from('<II', data, 0x18)
        data = data[2048:2048+size]
    else:
        if args.member is None or args.member not in (*range(38), *range(40, 59)):
            parser.error('Select a loader-qualified --member (0..37 or 40..58)')
        sector, size = struct.unpack_from('<II', data, args.member*8)
        data = data[sector*2048:sector*2048+size]
        base = 0x800ee970 if args.member < 30 else 0x800fa000
    disasm = Cs(CS_ARCH_MIPS, CS_MODE_MIPS32 | CS_MODE_LITTLE_ENDIAN)
    disasm.skipdata = True
    for spec in args.ranges:
        address, count = spec.split(':')
        address, count = int(address, 0), int(count, 0)
        offset = address-base
        if address & 3 or count < 1 or offset < 0 or offset+count*4 > len(data):
            parser.error(f'Range outside original source: {spec}')
        chunk = data[offset:offset+count*4]
        if args.words:
            for i, (word,) in enumerate(struct.iter_unpack('<I', chunk)):
                print(f'{address+i*4:08X} {word:08X}')
        else:
            for op in disasm.disasm(chunk, address):
                print(f'{op.address:08X} {int.from_bytes(op.bytes, "little"):08X} {op.mnemonic} {op.op_str}')


if __name__ == '__main__':
    main()

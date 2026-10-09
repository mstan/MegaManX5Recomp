"""Find candidate X5 counterparts of verified X6 resident MIPS routines.

This is a research aid, not hook authority. Inspect original instructions and
callers before using any candidate. It never modifies binaries or generated C.
"""
import argparse
from collections import defaultdict, Counter
from pathlib import Path
import struct


def executable(path):
    data = Path(path).read_bytes()
    if data[:8] != b'PS-X EXE':
        raise ValueError('Expected an original PS-X EXE')
    base, size = struct.unpack_from('<II', data, 0x18)
    return base, struct.unpack('<' + str(size // 4) + 'I', data[2048:2048+size])


def normalized(word):
    opcode = word >> 26
    if opcode in (2, 3):
        return word & 0xfc000000
    if opcode == 0:
        return word
    # Keep opcode and register choices; relocations, branches and data offsets
    # differ between titles. Exact instructions still require manual review.
    return word & 0xffff0000


def candidates(reference, target, start, count=64):
    ref = tuple(normalized(w) for w in reference[start:start+count])
    index = defaultdict(list)
    width = 6
    for pos in range(len(target)-width+1):
        index[tuple(target[pos:pos+width])].append(pos)
    votes = Counter()
    for offset in range(len(ref)-width+1):
        if len(set(ref[offset:offset+width])) < 3:
            continue
        for pos in index.get(ref[offset:offset+width], ()):
            if 0 <= pos-offset <= len(target)-len(ref):
                votes[pos-offset] += 1
    scored = []
    for pos, votes_count in votes.most_common(40):
        score = sum(a == b for a, b in zip(ref, target[pos:pos+len(ref)]))
        scored.append((score / len(ref), votes_count, pos))
    return sorted(scored, reverse=True)[:3]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('reference')
    parser.add_argument('target')
    parser.add_argument('addresses', nargs='+', type=lambda value: int(value, 0))
    parser.add_argument('--words', type=int, default=64)
    args = parser.parse_args()
    ref_base, reference = executable(args.reference)
    target_base, original = executable(args.target)
    target = tuple(normalized(word) for word in original)
    for address in args.addresses:
        matches = candidates(reference, target, (address-ref_base)//4, args.words)
        print(hex(address), [(hex(target_base+pos*4), round(score, 3), votes)
                             for score, votes, pos in matches])


if __name__ == '__main__':
    main()

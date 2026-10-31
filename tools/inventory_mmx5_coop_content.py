"""Inventory original SLUS-01334 checkpoint records for reproducible co-op QA.

Read-only: no generated save, stage-success flag or game patch. The original
8002ACC4..8002AE4C indexes 80072D10 by (stage*2+section), then checkpoint*4,
then a 0x24-byte position/camera/arrival record. Table extents are proven by
adjacent pointer arrays and the following non-pointer data at 80072DC8.
Placement lists additionally follow both native roots: viewport-scanned
80072EAC (8002B070) and entry-spawned 80072F64 (8002B520 -> 8002B61C).
Emit metadata only; original executable bytes stay local.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct

ROOT = 0x80072D10
ROOT_END = 0x80072DC8
RECORDS = (0x80071B68, 0x80072B4C)


def inventory(path, bin_path=None):
    image = path.read_bytes()
    if image[:8] != b'PS-X EXE':
        raise ValueError('Expected original PS-X EXE')
    base, size = struct.unpack_from('<II', image, 0x18)

    def offset(address, count):
        at = address - base
        if at < 0 or at + count > size or at + count + 2048 > len(image):
            raise ValueError(f'Address outside original image: {address:08X}')
        return at + 2048

    def word(address):
        return struct.unpack_from('<I', image, offset(address, 4))[0]

    # Guard the identity and indexing instructions rather than silently reading
    # a different revision as this revision's content tables.
    for address, expected in ((0x8002ACC4, 0x3C058007), (0x8002ACC8, 0x24A52D10),
                              (0x8002ACDC, 0x00021040), (0x8002ACE4, 0x00021080),
                              (0x8002ACF4, 0x00031880), (0x8002AE44, 0x90A20022)):
        if word(address) != expected:
            raise ValueError(f'Unsupported revision/indexing instruction at {address:08X}')
    tables = [word(address) for address in range(ROOT, ROOT_END, 4)]
    starts = sorted(set(value for value in tables if value) | {ROOT})
    locations = []
    for slot, table in enumerate(tables):
        if not table:
            continue
        if table & 3 or not RECORDS[1] <= table < ROOT:
            raise ValueError(f'Invalid checkpoint table {table:08X}')
        end = starts[starts.index(table) + 1]
        for checkpoint, address in enumerate(range(table, end, 4)):
            record = word(address)
            if record < RECORDS[0] or record + 0x24 > RECORDS[1] or (record - RECORDS[0]) % 0x24:
                raise ValueError(f'Invalid checkpoint record {record:08X}')
            values = struct.unpack_from('<12h', image, offset(record, 0x24))
            locations.append(dict(
                id=f's{slot//2:02d}-a{slot%2}-c{checkpoint:02d}',
                stage=slot//2, section=slot%2, checkpoint=checkpoint,
                table=f'{table:08X}', record=f'{record:08X}',
                player=dict(x=values[0], y=values[1]),
                camera=dict(x=values[2], y=values[3],
                            min_x=values[8], max_x=values[9],
                            min_y=values[10], max_y=values[11]),
                facing=image[offset(record+0x18, 1)],
                status='unverified',
                checks={name:'unverified' for name in (
                    'terrain_collision_scroll', 'enemies_interactions', 'music_sfx',
                    'checkpoint_retry', 'boss', 'exit_actual_transition')},
            ))
    report = dict(format_version=1, source_sha256=hashlib.sha256(image).hexdigest(),
                source='SLUS-01334 original executable',
                evidence='8002ACC4..8002AE4C; pointer tables80072B4C..80072DC8',
                limits='Authored checkpoint inventory, not complete alternate-route/actor/exit coverage. '
                       'Some stage slots are reused/story-specific; do not infer campaign reachability.',
                locations=locations)
    if bin_path is not None:
        archive = bin_path.read_bytes()
        for address, expected in ((0x800136CC,0x2484FD50),(0x800136DC,0x00031840),
                                  (0x8002B078,0x25292EAC),(0x8002B134,0x92420003),
                                  (0x8002B138,0x2404000F),(0x8002B574,0x24A52F64),
                                  (0x8002B598,0x8C440000),(0x8002B634,0x92030003),
                                  (0x8002B638,0x2402000F),(0x8002B64C,0x0C00AD39)):
            if word(address) != expected:
                raise ValueError(f'Unsupported actor/overlay indexing at {address:08X}')
        # Native 8002B520 first scans the primary list, then passes the
        # parallel entry list to 8002B61C. Each has its own kind15 terminator;
        # reading beyond the primary terminator would merge unrelated tables.
        for list_name, root in (('placement_sections', 0x80072EAC),
                                ('secondary_placement_sections', 0x80072F64)):
            sections=[]
            for slot in range(len(tables)):
                table=word(root+slot*4)
                if not table:
                    continue
                member=image[offset(0x8006FD50+slot,1)]
                sector, payload_size=struct.unpack_from('<II',archive,member*8)
                start=sector*2048
                if member>=30 or start<2048 or not payload_size or start+payload_size>len(archive):
                    raise ValueError(f'Invalid stage overlay extent {member}')
                payload=archive[start:start+payload_size]
                resident=base<=table<base+size
                source=image[2048:2048+size] if resident else payload
                source_base=base if resident else 0x800EE970
                at=table-source_base
                if table&3 or at<0 or at+8>len(source):
                    raise ValueError(f'Placement table outside stage source: slot{slot}, {table:08X}')
                placements=[]
                for index in range(4096):
                    cursor=at+index*8
                    if cursor+8>len(source):
                        raise ValueError(f'Unterminated placement table {table:08X}')
                    flags,byte1,byte2,kind,x,y=struct.unpack_from('<4B2h',source,cursor)
                    if kind==15:
                        break
                    placements.append(dict(index=index,address=f'{table+index*8:08X}',
                                           flags=flags,byte1=byte1,byte2=byte2,kind=kind,x=x,y=y,
                                           status='unverified'))
                else:
                    raise ValueError(f'Placement table exceeds native QA bound {table:08X}')
                sections.append(dict(stage=slot//2,section=slot%2,table=f'{table:08X}',
                                     source='EXE' if resident else f'BIN member{member}',
                                     loader_member=member,placements=placements))
            report[list_name]=sections
        report['bin_sha256']=hashlib.sha256(archive).hexdigest()
        report['placement_evidence']='800136C8..800136EC selector; 8002B070..8002B444 eight-byte records, kind15 terminator'
        report['placement_limits']='Primary viewport-scanned authored records, not actor identities or exit mechanics. Story-dependent overlays and dynamically allocated actors require separate runtime coverage.'
        report['secondary_placement_evidence']='8002B520..8002B5A0 indexes80072F64 by stage*2+section; 8002B61C..8002B6BC entry allocation, eight-byte records, kind15 terminator'
        report['secondary_placement_limits']='Native entry list is allocated without viewport admission tests; it includes controllers/scenery as well as interaction actors. Presence is not runtime admission, mechanics or reachability proof; other dynamic producers are not exhaustively inventoried.'
    return report


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--bin', type=Path, help='Original extracted ROCK_X5.BIN for authored actor placements')
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    report = inventory(args.exe,args.bin)
    args.out.parent.mkdir(parents=True, exist_ok=True)
    with args.out.open('x', encoding='utf-8', newline='\n') as output:
        json.dump(report, output, indent=2)
        output.write('\n')
    locations = report['locations']
    print(f'{len(locations)} checkpoint locations, '
          f'{len({(row["stage"],row["section"]) for row in locations})} stage/section tables; all unverified')
    if 'placement_sections' in report:
        sections=report['placement_sections']
        print(f'{sum(len(row["placements"]) for row in sections)} primary placement records in {len(sections)} stage/section tables; identities/mechanics unverified')
        secondary=report['secondary_placement_sections']
        print(f'{sum(len(row["placements"]) for row in secondary)} secondary entry placement records in {len(secondary)} stage/section tables; identities/mechanics unverified')


if __name__ == '__main__':
    main()

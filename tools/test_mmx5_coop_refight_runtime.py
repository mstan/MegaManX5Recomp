"""Reach refights through original hub pads in an owned offline X5 session.

The hub checkpoint, HP/contact protection and pad-approach positions bypass
earlier traversal. Original pad collision, script, checkpoint selection and
loader must perform the room transition. Boss defeat/rewards remain unverified.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time
import traceback

import psutil
from coop_debug import Runtime, PLAY
from audit_mmx5_coop_locations import counter, wait_ready


def require(value, message):
    if not value:
        raise AssertionError(message)


def serial(value):
    if isinstance(value, bytes):
        return value.hex()
    if isinstance(value, list):
        return [serial(item) for item in value]
    if isinstance(value, dict):
        return {key: serial(item) for key, item in value.items()}
    return value


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--inventory', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--pads', default='2', help='Original pad subtypes2..9, comma separated')
    parser.add_argument('--leader', type=int, choices=(0, 1), default=0)
    parser.add_argument('--hp-only',action='store_true',
                        help='Preserve native hurt recovery for a subsequent active fight; do not hold contact timer61')
    parser.add_argument('--pending-passenger', action='store_true',
                        help='Observe a pending X/P2 charge emitter before P1 triggers one native pad')
    args = parser.parse_args()
    session = json.loads(args.session.read_text())
    exe = Path(session['command'][0]).resolve()
    require(Path(psutil.Process(session['pid']).exe()).resolve() == exe, 'Not the owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest() == session['executable_sha256'], 'Owned binary changed')
    r = Runtime(session['port'])
    require(not r.request('netplay_status')['active'], 'Offline fixture required')
    inventory = json.loads(args.inventory.read_text())
    sections = [row for row in inventory['placement_sections'] if row['stage'] == 12 and row['section'] == 0]
    pads = {row['byte2']: row for row in sections[0]['placements']
            if row['kind'] == 4 and row['byte1'] == 23 and 2 <= row['byte2'] <= 9}
    selected = [int(value) for value in args.pads.split(',')]
    require(selected and all(value in pads for value in selected), 'Unknown original refight pad')
    require(not args.pending_passenger or (args.leader == 0 and len(selected) == 1),
            'Pending-passenger check requires P1 leading a single pad')
    args.out.mkdir(parents=True, exist_ok=False)
    report = dict(success=False, human_validation=False, cases=[], leader=args.leader,
                  harness_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  session=str(args.session.resolve()), executable_sha256=session['executable_sha256'],
                  resident_loading=session.get('resident_loading', True), source_sha256=inventory['source_sha256'],
                  bypassed_prerequisites=['native stage12 hub checkpoint1 load', 'HP-only refill' if args.hp_only else 'HP/contact protection',
                                         'native collision-box-derived pad position/history'],
                  limits='Original pad/loader and both-hero arrival/control boundary only; no boss defeat, reward or campaign pass.')

    def save():
        (args.out/'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')

    def observe():
        state = r.personal_effects()
        # Keep the observations and their asynchronous boundary explicit.
        return dict(personal=serial(state), play=r.read(PLAY, 0x80).hex())

    def pending_emitters():
        pool = r.read(0x80094398, 0x2A00)
        return [dict(address=hex(0x80094398+i), bytes=pool[i:i+0x70].hex())
                for i in range(0, len(pool), 0x70)
                if pool[i] and pool[i+1:i+3] == b'\x02\x12' and not pool[i+5]]

    try:
        r.resume_native_pause()
        r.release()
        r.fixture(11, hp=3)
        r.fixture(12, hp=0 if args.hp_only else 3)
        for subtype in selected:
            case = dict(pad=pads[subtype], status='unverified', milestones=[])
            report['cases'].append(case)
            save()
            previous = counter(r, 'mmx5.coop.enrolled')
            r.load_location(12, 0, 1)
            wait_ready(r, previous)
            case['hub'] = observe()
            handoffs_before = counter(r, 'mmx5.coop.p2-refight-pad-handoffs')
            r.screenshot(args.out/f'pad{subtype}-hub.png')
            def pad_actors():
                pool = r.read(0x800C6A10, 0x2900)
                return [pool[i:i+0xA4] for i in range(0, len(pool), 0xA4)
                        if pool[i] and pool[i+1] == 23 and pool[i+2] == subtype]

            actors = pad_actors()
            if not actors:
                # Let the original directional placement scan discover pads
                # outside the hub's initial view. Stay off the collision box.
                offset = -64 if pads[subtype]['x'] == max(pad['x'] for pad in pads.values()) else 64
                approach = (pads[subtype]['x']+offset, pads[subtype]['y']-80)
                case['activation_approach'] = approach
                r.teleport(args.leader, *approach)
                deadline = time.monotonic()+15
                while not actors:
                    require(time.monotonic() < deadline, 'Native scan did not activate the requested pad')
                    time.sleep(.04)
                    actors = pad_actors()
            require(len(actors) == 1, 'Original pad actor not active in the hub')
            require(r.read(0x800EF65C, 4) == bytes.fromhex('e8ffbd27'), 'Unexpected native pad overlay')
            pad = actors[0]
            body = r.players()[args.leader]
            pad_box = r.read(struct.unpack_from('<I', pad, 0x68)[0], 4)
            body_box = r.read(struct.unpack_from('<I', body, 0x68)[0], 4)
            x, y = struct.unpack_from('<ii', pad, 8)
            y += (struct.unpack_from('<b', pad_box, 1)[0]-struct.unpack_from('<b', body_box, 1)[0])*65536
            # First vertical contact from above. Centering inside the box can
            # bypass the one-way platform before P2's earlier terrain pass.
            y -= (pad_box[3]+body_box[3])*65536
            case['approach'] = dict(x=x/65536, y=y/65536, pad=pad.hex(),
                                    pad_box=pad_box.hex(), body_box=body_box.hex())
            cleared_before = counter(r, 'mmx5.coop.p2-buster-effects-cleared')
            if args.pending_passenger:
                require(r.players()[1][2] == 0, 'Pending-passenger check requires X/P2')
                r.input(0x8000, 1)
                start = r.state()['world_ticks']
                deadline = time.monotonic()+25
                while r.state()['world_ticks'] < start+120:
                    require(time.monotonic() < deadline, 'World stopped while charging passenger')
                    time.sleep(.02)
                require(r.players()[1][0x95] == 0x12, 'Passenger did not reach original full charge')
                # Prepare the ordinary position/history fixture without
                # submitting it. Only its sequence byte is written during
                # the short native pending-emitter window.
                diagnostic = r.diagnostic()
                sequence = int.from_bytes(r.read(diagnostic+0x50, 4), 'little')+1
                require(sequence <= 255, 'Fixture sequence exhausted')
                fields = {0x54: 10, 0x55: 0, 0x56: 0, 0x58: 0, 0x59: 0}
                fields.update({0x90+i: value for i, value in enumerate(struct.pack('<ii', x, y))})
                for offset, value in fields.items():
                    r.request('write_ram', addr=hex(diagnostic+offset), val=hex(value))
                r.request('fn_clear')
                r.request('fn_filter', lo='0x8002D078', hi='0x8002D07C')
                r.input(0, 1)
                deadline = time.monotonic()+3
                pending = pending_emitters()
                while not pending:
                    require(time.monotonic() < deadline, 'No pending native passenger emitter observed')
                    time.sleep(.01)
                    pending = pending_emitters()
                case['pending_emitters'] = pending
                r.request('write_ram', addr=hex(diagnostic+0x50), val=hex(sequence))
            else:
                r.teleport(args.leader, x/65536, y/65536)
            deadline = time.monotonic()+90
            activation_deadline = time.monotonic()+20
            last = None
            transitioned = False
            activated = False
            while True:
                state = r.personal_effects()
                play = r.read(PLAY, 0x20)
                require(not state['state']['failed'], 'Co-op reported failure during native warp')
                require(not (state['life'][1] == 3 and state['scene'][0] == 2),
                        'Incoming P2 acquired the native script and stranded its arrival')
                key = (play[0], play[1], play[0x1D], *state['life'], *state['scene'])
                if key != last:
                    case['milestones'].append(dict(personal=serial(state), play=play.hex()))
                    last = key
                    save()
                if play[0x1D] == subtype and (play[0] == 9 or play[1] == 1):
                    transitioned = True
                bodies = state['bodies']
                activated |= transitioned or bool(state['scene'][0]) or bodies[args.leader][5] in (0x14, 0x16)
                require(activated or state['life'][args.leader] == 0,
                        'Pad approach lost its leader before native activation')
                require(activated or time.monotonic() < activation_deadline,
                        'Original pad did not acquire the approaching hero')
                unlocked = all(body[4] == 1 and not any(body[o] for o in (0x7A, 0xD0, 0xD4)) for body in bodies)
                if transitioned and play[0] == 10 and not play[1] and not play[0x1C] and \
                   state['life'][:2] == [0, 0] and not state['scene'][0] and unlocked:
                    case['arrival'] = dict(personal=serial(state), play=play.hex())
                    r.release()
                    case['handoffs'] = counter(r, 'mmx5.coop.p2-refight-pad-handoffs')-handoffs_before
                    require(case['handoffs'] == args.leader,
                            'Native pad ownership did not follow the approaching hero exactly once')
                    if args.pending_passenger:
                        counters = r.request('mod_counters')['counters']
                        cleared = next(row for row in counters if row['name'] == 'mmx5.coop.p2-buster-effects-cleared')
                        case['cleared_emitters'] = cleared['count']-cleared_before
                        case['clear_frame'] = cleared['last_frame']
                        require(case['cleared_emitters'] > 0,
                                'Passenger did not cancel an observed pending emitter; setup window unqualified')
                        case['allocations'] = r.request('fn_entry_dump', count=256)
                        require(not any(int(row['ra'], 16) == 0x8004F768 and row['frame'] > cleared['last_frame']
                                        for row in case['allocations']['entries']),
                                'A canceled passenger emitter allocated a later native child')
                        r.request('fn_disable')
                    r.screenshot(args.out/f'pad{subtype}-arrival.png')
                    case['render_stats'] = r.request('render_pass_stats')
                    require(all(not case['render_stats'][key] for key in
                                ('verify_mismatch', 'watchdog', 'vram_leaks', 'device_reads')),
                            'Native warp left a private rendering fault')
                    require(not counter(r, 'mmx5.coop.local-view-state-leak'), 'Native warp leaked host view state')
                    case['status'] = 'passed'
                    save()
                    print('PASS original pad', subtype, 'transition and both native arrival/control unlocks', flush=True)
                    break
                require(time.monotonic() < deadline, 'Native pad/loader/both-hero arrival did not finish')
                # Dismiss original script dialogue only. No completion flags.
                if play[0] == 10:
                    r.tap(0x4000, .025, max(0, state['scene'][0]-1))
                time.sleep(.04)
        report['success'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        if report['cases']:
            report['cases'][-1]['status'] = 'failed'
        try:
            report['failure'] = observe()
            r.screenshot(args.out/'failure.png')
        except Exception:
            report['capture_error'] = traceback.format_exc()
        raise
    finally:
        try:
            if args.pending_passenger:
                r.request('fn_disable')
            r.release()
            r.fixture(12, hp=0)
            if r.read(PLAY+1, 1) == b'\x00':
                r.tap(8, .15, 0 if r.lifecycle()[0] == 0 else 1)
        except Exception:
            report['cleanup_error'] = traceback.format_exc()
        save()


if __name__ == '__main__':
    main()

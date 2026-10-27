"""Qualify native X armor selection using two owned delay-sync peers.

Load an unmodified native card on both peers, then use replicated host input
for boss selection, Cancel, X-only armor selection and actual stage entry.
No online RAM writes or snapshots. This is local transport evidence; physical
LAN and manual visual acceptance remain separate checks.
"""
from __future__ import annotations

import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import shutil
import time
import traceback
from pathlib import Path

from test_mmx5_coop_netplay import (
    PLAY, prepare_peer, reserve_ports, parse_crc_log, require, sha256_file,
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--exe', type=Path, required=True)
    parser.add_argument('--disc', type=Path, required=True)
    parser.add_argument('--bios', type=Path, required=True)
    parser.add_argument('--card', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--x-seat', type=int, choices=(0, 1), required=True)
    parser.add_argument('--expect-defeated-mask', type=lambda value: int(value, 0), required=True)
    parser.add_argument('--stage', type=int, choices=range(1, 9), default=2)
    args = parser.parse_args()
    for name in ('exe', 'disc', 'bios', 'card', 'out'):
        setattr(args, name, getattr(args, name).resolve())
    for name in ('exe', 'disc', 'bios', 'card'):
        require(getattr(args, name).is_file(), f'Missing {name}')
    card = args.card.read_bytes()
    require(len(card) == 131072 and card[:2] == b'MC' and
            card[0x80] == 0x51 and card[0x2000:0x2002] == b'SC',
            'Requires a real native card with X5 in block1/slot1')
    expected = 0 if card[0x225F] else 1
    require(expected == args.x_seat, 'The native saved hero differs from --x-seat')
    args.out.mkdir(parents=True, exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    args.game_root = Path(__file__).resolve().parents[1]
    args.config = args.game_root/'game.toml'
    args.card_seed = args.out/'seed'
    args.card_seed.mkdir()
    shutil.copy2(args.card, args.card_seed/'card1.mcd')
    args.cameras = 'split'
    args.frontend = 'hidden-window'
    args.delay = 3
    args.latency = args.jitter = 0
    args.seed = 1296914485
    args.render_verify = True
    args.bios_hle = False
    ports, reservations = reserve_ports()
    peers = [prepare_peer(args, args.out, seat, ports, False, args.seed)
             for seat in (0, 1)]
    for peer in peers:
        cards = peer.directory/'saves/openbios'
        cards.mkdir()
        shutil.copy2(args.card, cards/'card1.mcd')
    report = dict(success=False, human_validation=False, physical_lan=False,
                  executable_sha256=sha256_file(args.exe),
                  source_card_sha256=sha256_file(args.card),
                  x_seat=args.x_seat, stage=args.stage, checkpoints=[], trace=[],
                  limits='Native saved progress supplies frontend access; no acquisition, '
                         'boss defeat, physical LAN or exact visual acceptance claim.')
    pool = ThreadPoolExecutor(max_workers=2)

    def both(method, *values):
        futures = [pool.submit(getattr(peer, method), *values) for peer in peers]
        return [future.result() for future in futures]

    def ticks(count):
        start = min(row['tick'] for row in both('status'))
        end = time.monotonic()+45
        while min(row['tick'] for row in both('status')) < start+count:
            require(time.monotonic() < end, 'Network simulation stopped advancing')
            time.sleep(.03)

    def pulse(button, seat=0):
        peers[seat].input(button)
        try:
            ticks(6)
        finally:
            peers[seat].input(0)
        ticks(10)

    def wait(test, label, seconds=90):
        end = time.monotonic()+seconds
        while not test():
            require(time.monotonic() < end, label)
            ticks(4)

    def all_read(address, size=1):
        return [peer.read(address, size) for peer in peers]

    def checkpoint(label):
        ticks(70)
        status = both('status')
        logs = [parse_crc_log(peer.log) for peer in peers]
        watermark = min(row['resolved_through'] for row in status)
        common = sorted(tick for tick in set(logs[0]) & set(logs[1])
                        if tick <= watermark)
        previous = report['checkpoints'][-1]['through'] if report['checkpoints'] else 0
        require(sum(tick > previous for tick in common) >= 2,
                'No new matched simulation evidence')
        for tick in common:
            require(logs[0][tick] == logs[1][tick], f'{label}: digest mismatch at {tick}')
        row = dict(name=label, through=common[-1], matched_samples=len(common),
                   status=status, play=[value.hex() for value in all_read(PLAY, 0x1C0)])
        report['checkpoints'].append(row)
        for peer in peers:
            peer.screenshot(peer.directory/(label+'.png'))
        print('PASS', label, 'matched through', common[-1], flush=True)

    try:
        for reservation in reservations:
            reservation.close()
        for peer in peers:
            peer.launch()
        report['pids'] = [peer.process.pid for peer in peers]
        print('Owned network armor peers', report['pids'], flush=True)
        deadline = time.monotonic()+120
        for peer in peers:
            while True:
                try:
                    peer.status()
                    peer.input(0)
                    break
                except (OSError, AssertionError, RuntimeError):
                    require(peer.running() and time.monotonic() < deadline, 'Peer startup failed')
                    time.sleep(.2)
        while all_read(0x800D1F28) != [b'\x06']*2:
            require(time.monotonic() < deadline, 'Native title timeout')
            pulse(8)
        wait(lambda: all_read(0x800D1F29) == [b'\x01']*2 and
             all_read(0x8009A424) == [b'\0']*2, 'Native title fade timeout')
        pulse(0x40)
        require(all_read(0x800D1F2A) == [b'\x01']*2, 'Continue cursor differs')
        pulse(0x4000)
        deadline = time.monotonic()+180
        signature = None
        while all_read(PLAY, 3) != [b'\x04\x04\0']*2:
            play = peers[0].read(PLAY, 0x1C0)
            if signature != play[:4]:
                signature = play[:4]
                report['trace'].append(dict(play=play.hex()))
                print('Native card load', play[:4].hex(), flush=True)
            require(time.monotonic() < deadline, 'Native card load did not reach boss menu')
            require(play[0] not in (7, 8, 9, 10), 'Entered stage before armor selection')
            if play[0] == 20 or (play[0] == 4 and play[1] == 3):
                ticks(4)
            elif play[:2] == b'\0\x03':
                if play[3] != 4:
                    ticks(4)
                else:
                    pulse(0x80 if play[10] else 0x4000)
            else:
                pulse(0x4000)
        require(all_read(PLAY+0x44) == [bytes([expected])]*2, 'Native hero changed')
        require(all_read(PLAY+0x4C) == [bytes([args.expect_defeated_mask])]*2,
                'Native progress differs from the real saved slot')
        before = all_read(PLAY+0x28)
        pulse(0x40, 1)
        pulse(0x4000, 1)
        require(all_read(PLAY, 3) == [b'\x04\x04\0']*2 and
                all_read(PLAY+0x28) == before, 'P2 controls boss menu')
        table = peers[0].read(0x800F5050, 8)
        target = table.index(args.stage)
        for _ in range(8):
            cursor = peers[0].read(PLAY+0x28, 1)[0]
            if cursor == target:
                break
            pulse((0x20 if target >= 4 else 0x80) if cursor//4 != target//4
                  else (0x40 if cursor % 4 < target % 4 else 0x10))
        require(all_read(PLAY+0x28) == [bytes([target])]*2, 'P1 did not select stage')
        checkpoint('p1-boss-selection')
        pulse(0x4000)
        wait(lambda: all_read(PLAY, 3) == [b'\x04\x04\x05']*2, 'Native armor menu timeout')
        rosters = [list(peer.read(0x800F6DF0, peer.read(0x800F6DFB, 1)[0])) for peer in peers]
        require(rosters[0] == rosters[1] and 1 in rosters[0] and
                all(form in range(5) for form in rosters[0]), 'Invalid X-only roster')
        report['roster'] = rosters[0]
        cursor = all_read(PLAY+0x30)
        pulse(0x20, expected ^ 1)
        pulse(0x4000, expected ^ 1)
        require(all_read(PLAY, 3) == [b'\x04\x04\x05']*2 and
                all_read(PLAY+0x30) == cursor, 'Other hero controls armor selection')
        pulse(0x1000, 0)
        wait(lambda: all_read(PLAY, 3) == [b'\x04\x04\0']*2, 'P1 Cancel did not return')
        pulse(0x4000, 0)
        wait(lambda: all_read(PLAY, 3) == [b'\x04\x04\x05']*2, 'Armor reentry failed')
        cursor = all_read(PLAY+0x30)
        pulse(0x20, expected)
        require(all(a != b for a, b in zip(cursor, all_read(PLAY+0x30))),
                'X cannot move the armor cursor')
        for _ in range(5):
            if all_read(PLAY+0x2A) == [b'\x01']*2:
                break
            pulse(0x20, expected)
        require(all_read(PLAY+0x2A) == [b'\x01']*2, 'Fourth not selectable')
        checkpoint('x-owned-fourth-selection')
        pulse(0x4000, expected)
        deadline = time.monotonic()+150
        while True:
            state = both('observe')
            if all(row['enrolled'] and row['mode'] == 10 and not row['minor'] and
                   not row['gate'] and row['stage'] == args.stage and
                   row['lifecycle'][:2] == [0, 0] for row in state):
                break
            require(time.monotonic() < deadline, 'Native selected stage entry timed out')
            if any(row['gate'] or row['mode'] == 20 for row in state):
                pulse(0x4000)
            else:
                ticks(4)
        require(all(row['p1']['character'] == expected and
                    row['p2']['character'] == (expected ^ 1) and
                    row[f'p{expected+1}']['native_roster'] == 1 for row in state),
                'Stage entry changed seats or selected form')
        report['entry'] = state
        checkpoint('native-entry-preserves-seats')
        report['view_probes'] = []
        for peer in peers:
            counters = {row['name']: row['count'] for row in peer.request('mod_counters')['counters']}
            stats = peer.request('render_pass_stats')
            require(not counters.get('mmx5.coop.local-view-state-leak', 0), 'Private draw leaked')
            require(stats.get('verify_checks', 0) and
                    all(not stats.get(key, 0) for key in
                        ('verify_mismatch', 'watchdog', 'vram_leaks', 'device_reads')),
                    'Private draw sandbox failed')
            for path in ('saves/card1.mcd', 'saves/openbios/card1.mcd'):
                require(sha256_file(peer.directory/path) == report['source_card_sha256'],
                        'Private native card changed')
            report['view_probes'].append(dict(seat=peer.seat, stats=stats, counters=counters))
        require(sha256_file(args.card) == report['source_card_sha256'], 'Source native card changed')
        report['success'] = True
    except BaseException:
        report['error'] = traceback.format_exc()
        print(report['error'], flush=True)
    finally:
        for peer in peers:
            peer.stop()
        pool.shutdown(wait=True)
        for reservation in reservations:
            reservation.close()
        (args.out/'report.json').write_text(json.dumps(report, indent=2)+'\n')
    return 0 if report['success'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

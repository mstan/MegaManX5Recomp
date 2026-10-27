"""Reproduce delayed X buster allocation across native fatal entry.

Mission selection, contact protection and fatal HP are explicit setup bypasses.
Charge/release, the shared emitter and the death sequence use original code.
This qualifies cancellation of a pending emitter, not damage or campaign death.
"""
import argparse
import hashlib
import json
from pathlib import Path
import time
import traceback

import psutil
from coop_debug import Runtime, PLAY
from audit_mmx5_coop_locations import counter, snapshot, wait_ready


def require(value, message):
    if not value:
        raise AssertionError(message)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    session = json.loads(args.session.read_text())
    exe = Path(session['command'][0]).resolve()
    require(Path(psutil.Process(session['pid']).exe()).resolve() == exe, 'Not the owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest() == session['executable_sha256'], 'Owned binary changed')
    r = Runtime(session['port'])
    require(not r.request('netplay_status')['active'], 'Offline fixture required')
    args.out.mkdir(parents=True, exist_ok=False)
    report = dict(success=False, human_validation=False, samples=[],
                  executable_sha256=session['executable_sha256'], session=str(args.session.resolve()),
                  resident_loading=session.get('resident_loading', True),
                  bypassed_prerequisites=['Grizzly checkpoint1 loader', 'HP/contact protection', 'fatal HP fixture'])

    def ticks(count):
        start = r.state()['world_ticks']
        deadline = time.monotonic()+15
        while r.state()['world_ticks'] < start+count:
            require(time.monotonic() < deadline, 'Native world stopped')
            time.sleep(.01)

    def effects():
        data = r.read(0x80094398, 0x2a00)
        return [dict(address=hex(0x80094398+i), bytes=data[i:i+0x70].hex())
                for i in range(0, len(data), 0x70) if data[i] and data[i+1] == 2]

    try:
        r.resume_native_pause()
        r.release()
        r.fixture(11, hp=3)
        r.fixture(12, hp=3)
        previous = counter(r, 'mmx5.coop.enrolled')
        r.load_location(1, 0, 1)
        wait_ready(r, previous)
        ticks(30)
        seat = next(i for i, body in enumerate(r.players()) if body[2] == 0)
        report['x_seat'] = seat
        cleared_name = f'mmx5.coop.p{seat+1}-buster-effects-cleared'
        cleared_before = counter(r, cleared_name)
        r.input(0x8000, seat)
        ticks(120)
        kind = r.players()[seat][0x95]
        require(kind in (0x12, 0x14), 'Native full charge not established')
        # Prepare the mailbox while charging. The sequence byte alone submits
        # it later, leaving the short pending-emitter window observable.
        diagnostic = r.diagnostic()
        sequence = int.from_bytes(r.read(diagnostic+0x50, 4), 'little')+1
        require(sequence <= 255, 'Fixture sequence exhausted')
        for offset, value in ((0x54, 1), (0x55, seat), (0x56, 0), (0x58, 0), (0x59, 0)):
            r.request('write_ram', addr=hex(diagnostic+offset), val=hex(value))
        r.request('fn_clear')
        r.request('fn_filter', lo='0x8002D078', hi='0x8002D07C')
        r.input(0, seat)
        deadline = time.monotonic()+3
        pending = []
        while time.monotonic() < deadline:
            pending = [e for e in effects() if (b := bytes.fromhex(e['bytes']))[2] == kind and not b[5]]
            if pending:
                break
            time.sleep(.01)
        require(pending, 'Setup did not observe a pending native charge emitter')
        report['pending'] = pending
        r.request('write_ram', addr=hex(diagnostic+0x50), val=hex(sequence))
        deadline = time.monotonic()+8
        while int.from_bytes(r.read(diagnostic+0x60, 4), 'little') != sequence:
            require(time.monotonic() < deadline, 'Fatal fixture not acknowledged')
            time.sleep(.01)
        report['fatal_ack_frame'] = r.request('frame')['frame']
        for _ in range(12):
            report['samples'].append(dict(snapshot=snapshot(r), effects=effects()))
            ticks(1)
        report['allocations'] = r.request('fn_entry_dump', count=128)
        report['cleared_emitters'] = counter(r, cleared_name)-cleared_before
        require(report['cleared_emitters'] > 0,
                'Fatal entry did not cancel an observed pending emitter; timing/setup unqualified')
        require(not [e for e in report['allocations']['entries']
                     if int(e['ra'], 16) == 0x8004F768 and e['frame'] > report['fatal_ack_frame']],
                'A delayed charge allocated a child after fatal entry')
        require(r.lifecycle()[seat] in (4, 5) and r.lifecycle()[seat ^ 1] == 0,
                'Native fatal/survivor sequence not established')
        require(not effects(), 'Delayed emitter remained active after attack reset')
        if counter(r, 'mmx5.coop.personal-effects-diagnostic-version'):
            visuals = r.personal_effects()
            report['personal_effects_after_fatal'] = {
                pool: [actor.hex() for actor in visuals[pool][seat]] for pool in ('glow', 'trails')}
            require(not any(actor[3] for pool in ('glow', 'trails') for actor in visuals[pool][seat]),
                    'Fatal hero left visible personal effects')
        report['success'] = True
        print('PASS pending native buster emitter canceled at fatal entry', flush=True)
    except Exception:
        report['error'] = traceback.format_exc()
        raise
    finally:
        try:
            r.request('fn_disable')
            r.release()
            r.fixture(12, hp=0)
            if r.read(PLAY+1, 1) == b'\x00':
                owner = 0 if r.lifecycle()[0] == 0 else 1
                r.tap(8, .15, owner)
        except Exception:
            report['cleanup_error'] = traceback.format_exc()
        finally:
            (args.out/'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')


if __name__ == '__main__':
    main()

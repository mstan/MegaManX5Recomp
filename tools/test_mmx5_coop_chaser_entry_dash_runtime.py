"""Observe one native Ride Chaser entrance and a P2-only dash, offline.

Use an owned development session already in gameplay. Checkpoint0 setup
bypasses mission selection, not READY/arrival/control or any success flag.
Composed captures need visual review; this driver never promotes them to a
READY-count, animation/layering or full-road pass by their existence alone.
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
from test_mmx5_coop_chaser_runtime import snapshot as car_snapshot

CAMERA = 0x8009A1F8
UI_POOL, UI_BYTES, UI_STRIDE = 0x8009E7A0, 0x1200, 0x30
RIGHT, CIRCLE = 0x20, 0x2000


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def signed_word(data, offset):
    return struct.unpack_from('<i', data, offset)[0]


def describe_car(car):
    return dict(active=car[0], kind=car[1], draw=car[3], phase=list(car[4:7]),
                x=signed_word(car, 8)/65536, y=signed_word(car, 12)/65536,
                vx=signed_word(car, 0x20)/65536, vy=signed_word(car, 0x24)/65536,
                ax=signed_word(car, 0x28)/65536, facing=car[0x15],
                ground=car[0x70], boost=car[0x7D],
                held=struct.unpack_from('<H', car, 0x8A)[0],
                edge=struct.unpack_from('<H', car, 0x8C)[0],
                entrance_actor=struct.unpack_from('<I', car, 0xA0)[0])


def observe(r, label, ui=False):
    # The existing diagnostic snapshot keeps both riders/cars in one read.
    # Camera/UI are subsequent reads: retain the sampling span, never pretend
    # their coordinates came from that same world update on an unpaused run.
    row = car_snapshot(r)
    row['label'] = label
    row['cars_decoded'] = [describe_car(bytes.fromhex(c)) for c in row['cars']]
    camera = r.read(CAMERA, 0xFC)
    row['camera'] = camera.hex()
    row['camera_layers'] = [dict(x=struct.unpack_from('<h', camera, a+0xA)[0],
                                  y=struct.unpack_from('<h', camera, a+0xE)[0],
                                  previous_x=struct.unpack_from('<h', camera, a+0x16)[0],
                                  previous_y=struct.unpack_from('<h', camera, a+0x1A)[0])
                             for a in (0, 0x54, 0xA8)]
    if ui:
        pool = r.read(UI_POOL, UI_BYTES)
        row['entrance_control_actors'] = [dict(address=hex(UI_POOL+a), data=pool[a:a+UI_STRIDE].hex())
            for a in range(0, UI_BYTES, UI_STRIDE) if pool[a] and pool[a+1] == 0x1B]
        # Original EEE20..EEE3C allocates type1B into CAR+A0 using 2D144.
        # Its existence/count is NOT a verified count of visible READY logos.
    row['world_after_reads'] = r.state()['world_ticks']
    return row


def live_cars(row):
    play = bytes.fromhex(row['play'])
    return (play[0] == 10 and play[12:14] == b'\x05\x00' and
            row['life'][:2] == [0, 0] and row['state']['enrolled'] and
            all(c['active'] and not c['kind'] and c['phase'][0] == 1 for c in row['cars_decoded']))


def driving(row):
    return live_cars(row) and all(c['phase'][1] == 1 and c['ground'] & 8
                                  for c in row['cars_decoded'])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--stop-owned', action='store_true',
                        help='Quit this verified process after preserving results; cars may not accept pause')
    args = parser.parse_args()
    require(args.out.resolve().drive.upper() in ('F:', 'G:'), 'Evidence must stay on F: or G:')
    args.out.mkdir(parents=True, exist_ok=False)
    source = Path(__file__).read_bytes()
    (args.out/'harness.py').write_bytes(source)
    tools = Path(__file__).resolve().parent
    dependencies = {}
    for name in ('coop_debug.py', 'test_mmx5_coop_chaser_runtime.py'):
        data = (tools/name).read_bytes()
        (args.out/name).write_bytes(data)
        dependencies[name] = hashlib.sha256(data).hexdigest()
    report = dict(success=False, qualified=False, human_validation=False,
                  source_sha256=hashlib.sha256(source).hexdigest(), dependencies=dependencies,
                  session=str(args.session.resolve()), samples=[], captures=[], inputs=[], checks={},
                  bypassed_prerequisites=['Original stage5/section0/checkpoint0 loader fixture bypasses mission selection'],
                  native_contract={
                      'entrance': 'BIN7 EEC50 initializes car phase1/0 and mounted rider; no phase writes here',
                      'camera': 'BIN7 EF05C..EF098 adds current-minus-previous camera origin to car world position',
                      'dash': 'BIN7 F0924..F0984: native car+8C edge0x100 sets +7D=1 and +5=5',
                      'neutral': 'BIN7 F0864..F0918 clears grounded normal-mode +20/+28 when held direction bits are zero',
                      'binding': 'Native default Circle+Right; actual dash admission is required, not assumed from input'},
                  limits=['One offline roster/display/loading setup; no road completion or netplay qualification',
                          'Camera/UI reads are sampled after the coherent rider/car diagnostic read',
                          'READY count, rider animation, car/exhaust layering and input-specific presentation need composed-image review'])
    r = session = process = exe = created = None
    offline_owned = False

    def save():
        (args.out/'report.json').write_bytes((json.dumps(report, indent=2)+'\n').encode('utf-8'))

    def check(name, status, reason):
        report['checks'][name] = dict(status=status, reason=reason)

    def sample(label, ui=False):
        row = observe(r, label, ui)
        report['samples'].append(row)
        require(not row['state']['failed'], 'Co-op diagnostics report failure')
        return row

    def capture(label):
        metadata = r.screenshot(args.out/(label+'.png'))
        report['captures'].append(dict(label=label, **metadata))
        save()

    def hold(seat, buttons, reason):
        report['inputs'].append(dict(seat=seat, buttons=hex(buttons), reason=reason,
                                     world=r.state()['world_ticks']))
        r.input(buttons, seat)

    def interval(label, ticks, seconds=8):
        rows = []
        start = r.state()['world_ticks']
        deadline = time.monotonic()+seconds
        while True:
            row = sample(label)
            rows.append(row)
            require(live_cars(row), 'Rider left the living Chaser control window')
            if row['state']['world_ticks']-start >= ticks:
                return rows
            require(time.monotonic() < deadline, 'Bounded Chaser input window stopped')
            time.sleep(.005)

    try:
        session = json.loads(args.session.read_text(encoding='utf-8'))
        exe = Path(session['command'][0]).resolve()
        process = psutil.Process(session['pid'])
        created = process.create_time()
        require(Path(process.exe()).resolve() == exe, 'Not the owned process')
        digest = hashlib.sha256(exe.read_bytes()).hexdigest()
        require(digest == session['executable_sha256'], 'Owned binary changed')
        report.update(executable_sha256=digest, pid=process.pid, process_created=created)
        r = Runtime(session['port'])
        require(not r.request('netplay_status')['active'], 'Entrance fixture is offline only')
        offline_owned = True
        report['initial'] = sample('initial')
        require(bytes.fromhex(report['initial']['play'])[0] == 10,
                'Attach to owned gameplay; this driver does not skip title/story prerequisites')
        r.resume_native_pause()
        r.release()
        enrollments = next(c['count'] for c in r.request('mod_counters')['counters']
                           if c['name'] == 'mmx5.coop.enrolled')
        # Right reaches the authored short road after arrival. No jump/READY,
        # pose, collision or success state is forced. P1 becomes neutral below.
        hold(0, RIGHT, 'native opening approach'); hold(1, RIGHT, 'native opening approach')
        r.load_location(5, 0, 0)
        save()
        deadline = time.monotonic()+80
        entrance_seen = [False, False]
        entrance_captures = 0
        previous_phase = None
        while True:
            row = sample('native-entry', ui=True)
            if live_cars(row):
                for seat, car in enumerate(row['cars_decoded']):
                    entrance_seen[seat] |= car['phase'][1] == 0
                phase = tuple(tuple(c['phase']) for c in row['cars_decoded'])
                if phase != previous_phase and entrance_captures < 4:
                    capture('entry-'+str(entrance_captures))
                    entrance_captures += 1
                    previous_phase = phase
                if driving(row):
                    report['control_release'] = row
                    break
            require(time.monotonic() < deadline, 'Native checkpoint0 entrance did not reach grounded driving')
            time.sleep(.01)
        now = next(c['count'] for c in r.request('mod_counters')['counters']
                   if c['name'] == 'mmx5.coop.enrolled')
        require(now > enrollments, 'No fresh native co-op enrollment')
        require(r.read(0x800EE970, 4) == b'\x0b\x00\x00\x00', 'Wrong original Chaser overlay')
        for body_hex, car in zip(row['players'], row['cars_decoded']):
            body = bytes.fromhex(body_hex)
            require(body[0xD5] == 0xFF and body[0x15] == car['facing'] and body[0x47] != 0xFF,
                    'Native mounted rider identity/facing is not ready')
        check('native_entrance', 'passed' if all(entrance_seen) else 'unverified',
              'Observed car mode0 then native grounded mode1 for both riders' if all(entrance_seen)
              else 'Grounded driving reached, but polling missed at least one original arrival mode0')
        report['entrance_seen'] = entrance_seen
        check('shared_ready_sequence', 'unverified', 'Review entry composed captures and original READY semantics; type1B actor count alone is insufficient')
        hold(0, 0, 'P1 neutral throughout baseline and P2-only dash')
        hold(1, 0, 'release native direction before the measured dash')
        baseline = interval('neutral-baseline', 4)
        report['baseline_world_span'] = [baseline[0]['state']['world_ticks'], baseline[-1]['state']['world_ticks']]
        require(driving(baseline[-1]) and not any(c['boost'] for c in baseline[-1]['cars_decoded']),
                'Both riders must start the dash check in ordinary grounded driving')
        hold(1, CIRCLE | RIGHT, 'P2-only native dash; never write car boost/phase/velocity')
        dash = interval('p2-dash', 8)
        capture('p2-only-dash')
        hold(1, 0, 'end measured dash input')
        post = sample('after-dash', ui=True)
        admitted = [s for s in dash if s['cars_decoded'][1]['boost'] and
                    s['cars_decoded'][1]['phase'][1] == 5]
        require(admitted, 'No native P2 dash admission; inspect controls before inferring a gameplay defect')
        check('p2_native_dash', 'passed', 'Observed original +7D boost and mode5 following P2 Circle+Right')
        require(not any(s['cars_decoded'][0]['boost'] for s in dash), 'P2 dash enabled P1 boost')
        require(not any(s['cars_decoded'][0]['held'] & 0x183 for s in dash),
                'P1 native controls were not neutral; independent acceleration remains unqualified')
        ordinary_p1 = [s['cars_decoded'][0] for s in dash
                       if s['cars_decoded'][0]['phase'][1] == 1 and s['cars_decoded'][0]['ground'] & 8]
        require(ordinary_p1, 'No grounded neutral P1 sample during dash; compare the retained road observations')
        require(all(abs(c['vx']) < 1/65536 for c in ordinary_p1), 'P2 dash changed grounded neutral P1 velocity')
        before = baseline[-1]
        p2_before = abs(before['cars_decoded'][1]['vx'])
        p2_peak = max(abs(s['cars_decoded'][1]['vx']) for s in dash)
        require(p2_peak > p2_before, 'Native P2 dash did not accelerate its own car')
        report['motion'] = dict(p2_baseline_abs_vx=p2_before, p2_dash_peak_abs_vx=p2_peak,
            p1_grounded_neutral_samples=len(ordinary_p1),
            cars=[dict(seat=seat, world_dx=post['cars_decoded'][seat]['x']-before['cars_decoded'][seat]['x'],
                       camera_dx=post['camera_layers'][0]['x']-before['camera_layers'][0]['x'],
                       approximate_screen_dx=(post['cars_decoded'][seat]['x']-post['camera_layers'][0]['x'])-
                                             (before['cars_decoded'][seat]['x']-before['camera_layers'][0]['x']))
                  for seat in (0, 1)])
        check('independent_acceleration', 'passed', 'P2 native velocity increases; grounded neutral P1 retains zero native velocity and no boost. World-X includes authored camera carry')
        check('rider_animation_and_layering', 'unverified', 'Review composed entry/dash captures for mounted poses, car/body/exhaust priorities and native colors')
        stats = report['render_stats'] = r.request('render_pass_stats')
        require(all(not stats.get(k, 0) for k in ('verify_mismatch', 'watchdog', 'device_reads', 'vram_leaks')),
                'Private draw verification fault')
        report['success'] = True
        # Success means this bounded driver completed, not that visual READY,
        # every animation/layer or the whole comparative report is qualified.
        report['qualified'] = all(c['status'] == 'passed' for c in report['checks'].values())
    except Exception:
        report['error'] = traceback.format_exc()
        if r is not None:
            try:
                report['failure'] = observe(r, 'failure', ui=True)
                capture('failure')
            except Exception:
                report['capture_error'] = traceback.format_exc()
    finally:
        if r is not None and offline_owned:
            try:
                r.release()
                report['final'] = observe(r, 'final')
            except Exception:
                report['cleanup_error'] = traceback.format_exc()
        if args.stop_owned and offline_owned and r is not None and process is not None:
            try:
                require(process.create_time() == created and Path(process.exe()).resolve() == exe,
                        'Owned process identity changed before quit')
                try:
                    r.request('quit')
                except (OSError, RuntimeError):
                    pass
                process.wait(timeout=10)
                report['owned_stopped'] = True
            except psutil.NoSuchProcess:
                report['owned_stopped'] = True
            except Exception:
                report['stop_error'] = traceback.format_exc()
        elif r is not None and offline_owned:
            report['session_left_live'] = 'Inputs released; native Chaser may reject pause and continue into hazards. Main owns further handling.'
        save()
    print(json.dumps({k: report.get(k) for k in ('success', 'qualified', 'checks', 'error', 'stop_error')}), flush=True)
    return 0 if report['success'] and not report.get('stop_error') and not report.get('cleanup_error') else 1


if __name__ == '__main__':
    raise SystemExit(main())

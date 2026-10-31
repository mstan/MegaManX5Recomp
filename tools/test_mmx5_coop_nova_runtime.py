"""Check native Ultimate X's Nova Strike against a separated living partner.

Attach to an owned native-code or automatic-form X/P1 boot with local Split
and render verification.
Only HP/contact-grace and P2 position/history are setup cheats. Native controller
input must admit and retire the attack; never write its pose, energy or lock.
This bounded intro check does not certify acquisition or enhanced X/P2 assets.
"""
import argparse
import json
from pathlib import Path
import struct
import time
import traceback

from coop_debug import Runtime, PLAY
from test_mmx5_coop_scene_fatal_runtime import serial


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    session = json.loads(args.session.read_text(encoding='utf-8-sig'))
    runtime = Runtime(session['port'])
    args.out.mkdir(parents=True, exist_ok=False)
    report = dict(success=False, comparison_status='unverified', samples=[],
                  candidate_sha256=session['executable_sha256'],
                  automatic_form=bool(session.get('ultimate_armor_x')),
                  bypasses=[('Automatic form option is a cheat, not capsule acquisition.'
                             if session.get('ultimate_armor_x') else
                             'Original title code is a cheat, not capsule acquisition.'),
                            'HP-only refill; contact grace off; P2 position/history to native intro floor646/1130. Route bypassed.'])

    def observe(label):
        personal = runtime.personal_effects()
        play = runtime.read(PLAY, 0x30)
        cameras = runtime.read(runtime.diagnostic()+0x710, 0x1F8)
        origins = [[struct.unpack_from('<h', cameras, seat*0xFC+offset)[0]
                    for offset in (10, 14)] for seat in (0, 1)]
        row = dict(label=label, personal=serial(personal), play=play.hex(),
                   cameras=origins, camera_bytes=cameras.hex())
        report['samples'].append(row)
        return row, personal, play, origins

    def wait(predicate, label, seconds=15):
        deadline = time.monotonic()+seconds
        while True:
            row, personal, play, cameras = observe(label)
            assert personal['life'][:2] == [0, 0] and not personal['state']['failed'], label+' lost hero'
            if predicate(personal, play, cameras):
                return row, personal, play, cameras
            assert time.monotonic() < deadline, label
            time.sleep(.01)

    try:
        runtime.release()
        runtime.resume_native_pause()
        runtime.fixture(11, hp=3)
        runtime.fixture(12, hp=0)
        wait(lambda z, p, c: all(b[4:6] == b'\1\2' for b in z['bodies']) and
             not p[1] and not p[0x1C], 'native-idle')
        runtime.teleport(1, 646, 1130)
        row, z, p, cameras = wait(lambda z, p, c: all(b[4:6] == b'\1\2' and b[0x70] & 8
                                                   for b in z['bodies']) and
                                  abs(c[1][0]-c[0][0]) > 400, 'native-separated-views')
        report['before'] = row
        body = z['bodies'][0]
        assert body[0xFA] == 4 and p[0x0C:0x0E] == bytes((0, 0)), 'Require original Ultimate intro'
        if report['automatic_form']:
            report['native_flags_before'] = runtime.read(PLAY+0x4A, 1)[0]
            assert not report['native_flags_before'] & 0x18, 'Automatic forms changed native grants'
        energy = struct.unpack_from('<H', body, 0xA8)[0]
        assert energy == struct.unpack_from('<H', body, 0x148)[0] > 0, 'Native Giga not ready'
        report['giga_energy_before'] = energy
        native_button = struct.unpack('<H', runtime.read(0x8006FAE4+6*2, 2))[0]
        button = ((native_button & 255) << 8) | (native_button >> 8)
        assert button == 0x0200, 'Original Giga mapping changed'
        report['stats_before'] = runtime.request('render_pass_stats')
        runtime.input(button, 0)
        row, z, p, cameras = wait(lambda z, p, c: z['bodies'][0][5] == 0x22,
                                  'native-Nova-admission', 6)
        runtime.input(0, 0)
        report['admitted'] = row
        start = z['state']['world_ticks']
        y_before = struct.unpack_from('<i', z['bodies'][1], 12)[0]
        # The native floor correction may leave P2 at the truck's side.
        # Jump exercises controls without depending on a clear walk lane.
        runtime.input(0x4000, 1)
        row, z, p, cameras = wait(lambda z, p, c: z['state']['world_ticks'] >= start+6,
                                  'active-Nova-partner-input')
        runtime.input(0, 1)
        report['active'] = row
        report['stats_active'] = runtime.request('render_pass_stats')
        report['p2_rise'] = (y_before-struct.unpack_from('<i', z['bodies'][1], 12)[0])/65536
        assert z['bodies'][0][5] == 0x22 and p[0x1C] and z['scene'] == [0, 0], 'Missed actual Nova window'
        assert report['p2_rise'] > 0 and z['bodies'][1][0x67], 'Nova froze partner jump'
        assert abs(cameras[1][0]-report['before']['cameras'][1][0]) < 32, 'Nova reset partner camera'
        assert abs(cameras[1][0]-cameras[0][0]) > 400, 'Nova joined private cameras'
        assert report['stats_active']['verify_checks'] > report['stats_before']['verify_checks'], 'No verified private draw during Nova'
        runtime.screenshot(args.out/'native-Nova-active.png')
        row, z, p, cameras = wait(lambda z, p, c: z['bodies'][0][4:6] == b'\1\2' and
                                  not p[0x1C] and z['scene'] == [0, 0], 'native-Nova-release')
        report['released'] = row
        report['giga_energy_after'] = struct.unpack_from('<H', z['bodies'][0], 0xA8)[0]
        assert report['giga_energy_after'] == energy, 'Native Ultimate energy changed'
        if report['automatic_form']:
            report['native_flags_after'] = runtime.read(PLAY+0x4A, 1)[0]
            assert not report['native_flags_after'] & 0x18, 'Automatic forms persisted native grants'
        runtime.screenshot(args.out/'native-Nova-released.png')
        report['stats_final'] = runtime.request('render_pass_stats')
        assert not any(report['stats_final'].get(key, 0) for key in
                       ('verify_mismatch', 'watchdog', 'vram_leaks', 'device_reads', 'aborted')), 'Private draw fault'
        report.update(success=True, comparison_status='passed')
    except Exception:
        report['error'] = traceback.format_exc()
        print(report['error'], flush=True)
    finally:
        runtime.release()
        try:
            report['final'] = observe('final')[0]
            if runtime.read(PLAY+1, 1) == b'\0':
                runtime.tap(8, .12, 0)
        except Exception:
            pass
        (args.out/'report.json').write_bytes((json.dumps(report, indent=2)+'\n').encode())
    print(json.dumps({key: report.get(key) for key in
                      ('success', 'comparison_status', 'p2_rise', 'giga_energy_before', 'giga_energy_after', 'error')}), flush=True)
    return 0 if report['success'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

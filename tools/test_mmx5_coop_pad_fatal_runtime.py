"""Kill the actual P2 refight-pad owner after its living passenger hides.

One original pad timing, not a seat/character/camera matrix. Setup uses the
native hub loader, position/history teleports and resource/fatal-HP cheats.
No scene, death-action, boss, checkpoint-completion or release flag is set.
Death-presentation observations remain separate from the recovery result.
"""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path
import struct
import time
import traceback

import psutil
from coop_debug import Runtime, PLAY
from audit_mmx5_coop_locations import counter, wait_ready
from test_mmx5_coop_refight_runtime import require, serial


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--inventory', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    session = json.loads(args.session.read_text(encoding='utf-8'))
    exe = Path(session['command'][0]).resolve()
    require(Path(psutil.Process(session['pid']).exe()).resolve() == exe, 'Not the owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest() == session['executable_sha256'], 'Owned binary changed')
    r = Runtime(session['port'])
    require(not r.request('netplay_status')['active'], 'Offline setup only')
    inventory = json.loads(args.inventory.read_text(encoding='utf-8'))
    sections = [row for row in inventory['placement_sections'] if row['stage'] == 12 and row['section'] == 0]
    pad_entry = next(row for row in sections[0]['placements']
                     if row['kind'] == 4 and row['byte1'] == 0x17 and row['byte2'] == 2)
    args.out.mkdir(parents=True, exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    report = dict(success=False, human_validation=False, samples=[], pool_samples=[],
                  session=str(args.session.resolve()), executable_sha256=session['executable_sha256'],
                  harness_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  source_sha256=inventory['source_sha256'], pad=pad_entry,
                  comparisons=['F103', 'F4', 'legacy3', 'F133 partial'],
                  bypasses=['original hub12/0/CP1 loader', 'HP-only setup protection, disabled before seeding',
                            'P1 HP13/ammo121 and P2 HP17/ammo73 setup', 'collision-derived position/history teleport',
                            'P2 fatal HP80 only, after native passenger hiding'],
                  limits='One actual pad timing. No boss defeat, all death paths, live color or natural radial age-expiry claim.')

    def save():
        (args.out/'report.json').write_bytes((json.dumps(report, indent=2)+'\n').encode())

    def observe(label, state=None):
        state = state or r.personal_effects()
        row = dict(label=label, personal=serial(state), play=r.read(PLAY, 0x80).hex())
        report['samples'].append(row)
        save()
        return state

    def wait(predicate, message, timeout=30):
        deadline = time.monotonic()+timeout
        while True:
            state = r.personal_effects()
            if predicate(state):
                return state
            require(time.monotonic() < deadline, message)
            require(not state['state']['failed'], 'Co-op reports failed')
            time.sleep(.015)

    def pad_actors():
        pool = r.read(0x800C6A10, 0x2900)
        return [dict(address=hex(0x800C6A10+i), body=pool[i:i+0xA4])
                for i in range(0, len(pool), 0xA4)
                if pool[i] and pool[i+1:i+3] == b'\x17\x02']

    def death_pool(state):
        raw = r.read(0x800D1F40, 0x3000)
        slots = [dict(slot=i//0x60, body=raw[i:i+0x60].hex())
                 for i in range(0, len(raw), 0x60) if raw[i] and raw[i+1] == 0x11]
        row = dict(world=state['state']['world_ticks'], life=state['life'], scene=state['scene'], slots=slots,
                   observation='Guest pool requested after the coherent body row; separate asynchronous observation')
        report['pool_samples'].append(row)
        return row

    trace = False
    try:
        r.resume_native_pause()
        r.release()
        r.fixture(11, hp=3)
        r.fixture(12, hp=0)
        previous = counter(r, 'mmx5.coop.enrolled')
        r.load_location(12, 0, 1)
        wait_ready(r, previous)
        wait(lambda s: s['life'][:2] == [0, 0] and not s['scene'][0] and
             all(b[4:6] == b'\x01\x02' for b in s['bodies']), 'Hub did not settle after native arrival')
        r.fixture(11, hp=0)
        r.fixture(2, seat=0, hp=13, ammo=121)
        r.fixture(2, seat=1, hp=17, ammo=73)
        before = observe('native-hub-idle-distinct-resources')
        require([b[2] for b in before['bodies']] == [1, 0], 'Expected native Zero/P1 and X/P2')
        report['retry_before'] = r.read(PLAY+0x45, 1).hex()
        report['p1_inventory_before'] = before['bodies'][0][0xA8:0xC8].hex()
        report['p1_hp_before'] = before['bodies'][0][0x5C]
        handoffs_before = counter(r, 'mmx5.coop.dead-scene-owner-handoffs')
        actors = pad_actors()
        if not actors:
            r.teleport(1, pad_entry['x']+64, pad_entry['y']-80)
            deadline = time.monotonic()+15
            while not actors:
                require(time.monotonic() < deadline, 'Original placement scan did not admit pad2')
                time.sleep(.025)
                actors = pad_actors()
        require(len(actors) == 1, 'Expected one original pad2 actor')
        require(r.read(0x800EF65C, 4) == bytes.fromhex('e8ffbd27'), 'Wrong original pad overlay')
        pad = actors[0]['body']
        body = r.players()[1]
        pad_box = r.read(struct.unpack_from('<I', pad, 0x68)[0], 4)
        body_box = r.read(struct.unpack_from('<I', body, 0x68)[0], 4)
        x, y = struct.unpack_from('<ii', pad, 8)
        y += (struct.unpack_from('<b', pad_box, 1)[0]-struct.unpack_from('<b', body_box, 1)[0]
              -pad_box[3]-body_box[3])*65536
        report['approach'] = dict(x=x/65536, y=y/65536, pad=serial(actors[0]),
                                  pad_box=pad_box.hex(), body_box=body_box.hex())
        r.request('fn_clear')
        r.request('fn_filter', lo='0x8002D27C', hi='0x8002D280')
        trace = True
        r.teleport(1, x/65536, y/65536)
        hidden = wait(lambda s: s['scene'] == [2, 3] and s['life'][:2] == [0, 0] and
                      s['bodies'][0][4] == 3 and not s['bodies'][0][3],
                      'Actual pad did not establish the living hidden P1 passenger', 20)
        observe('actual-native-pad-hidden-passenger-before-fatal', hidden)
        report['pad_before_fatal'] = serial(pad_actors())
        r.fixture(1, seat=1)
        start = time.monotonic()
        deadline = start+110
        last_key = None
        fatal_seen = False
        transitioned = False
        native_intro_seen = False
        captured_fatal = captured_particles = False
        last_pool_world = -100
        recovered = None
        while time.monotonic() < deadline:
            state = r.personal_effects()
            play = r.read(PLAY, 0x80)
            require(not state['state']['failed'], 'Co-op failed during native pad fatal recovery')
            require(state['life'][0] == 0, 'Native pad owner death lost the living P1 passenger')
            key = (play[0], play[1], play[0x1C], play[0x1D], *state['life'], *state['scene'],
                   *(b[4] for b in state['bodies']), *(b[5] for b in state['bodies']))
            if key != last_key:
                observe('fatal-native-state-change', state)
                last_key = key
            fatal_seen |= state['life'][1] in (4, 5)
            transitioned |= play[0] == 9 or play[0x1D] == 2
            native_intro_seen |= play[0x1D] == 2 and state['scene'][0] == 1
            if state['life'][1] == 4 and not captured_fatal:
                report['fatal_body_observation'] = serial(state)
                r.screenshot(args.out/'native-owner-fatal-composed.png')
                captured_fatal = True
            if state['state']['world_ticks'] >= last_pool_world+4:
                pool = death_pool(state)
                last_pool_world = state['state']['world_ticks']
                if pool['slots'] and not captured_particles:
                    report['particle_body_observation'] = serial(state)
                    r.request('capture_mark', op='mark')
                    try:
                        r.screenshot(args.out/'native-death-particles-composed.png')
                        report['particle_capture_mark'] = r.request('capture_mark', op='status')
                        frame = r.request('frame')['frame']
                        gpu = r.request('gpu_frame_dump', frame=frame, count=8192)
                        (args.out/'particle-gpu.json').write_bytes((json.dumps(gpu, indent=2)+'\n').encode())
                    finally:
                        r.request('capture_mark', op='release')
                    captured_particles = True
            body = state['bodies'][0]
            unlocked = body[4] == 1 and body[3] and not any(body[o] for o in (0x7A, 0xD0, 0xD4))
            # CP2 has a brief grounded/unlocked frame before the original
            # boss acquires stand14. Require its actual intro and full native
            # HP fill, rather than mistaking that intermediate frame for the
            # final input boundary. No boss/init/release fields are written.
            boss = r.read(0x80092090, 0x9C)
            full_hp = r.read(PLAY+0xA2, 1)[0]
            fight_ready = native_intro_seen and boss[:2] == b'\x41\x36' and boss[4] == 1 and \
                          full_hp > 0 and boss[0x5C] == full_hp
            if fatal_seen and state['life'][1] == 5 and play[0] == 10 and not play[1] and \
               not play[0x1C] and not state['scene'][0] and unlocked and fight_ready and (body[0x70] | body[0x71]) & 8:
                report['actual_native_boss_fill'] = dict(boss=boss.hex(), full_hp=full_hp)
                recovered = observe('actual-native-script-finished-survivor-grounded', state)
                break
            if play[0] == 10 and (play[0x1C] or state['scene'][0]):
                r.tap(0x4000, .025, max(0, state['scene'][0]-1))
            time.sleep(.015)
        require(recovered is not None, 'Original pad fatal/script/arrival did not recover the survivor')
        report['actual_native_transition_seen'] = transitioned
        report['elapsed_seconds_to_recovery'] = time.monotonic()-start
        report['owner_handoffs'] = counter(r, 'mmx5.coop.dead-scene-owner-handoffs')-handoffs_before
        require(report['owner_handoffs'] == 1, 'Expected exactly one real dead-owner recovery')
        body = recovered['bodies'][0]
        require(body[0x5C] == report['p1_hp_before'] and body[0xA8:0xC8].hex() == report['p1_inventory_before'],
                'Survivor resources changed during native fatal recovery')
        require(r.read(PLAY+0x45, 1).hex() == report['retry_before'], 'One owner death consumed a team retry')
        x_before = struct.unpack_from('<i', body, 8)[0]
        world = recovered['state']['world_ticks']
        r.input(0x20, 0)
        wait(lambda s: s['state']['world_ticks'] >= world+12, 'Survivor world did not advance', 10)
        r.release()
        moved = observe('survivor-native-right-input')
        report['survivor_right_pixels'] = (struct.unpack_from('<i', moved['bodies'][0], 8)[0]-x_before)/65536
        require(report['survivor_right_pixels'] > 1, 'Survivor did not respond to native movement')
        require(moved['life'][:2] == [0, 5], 'Movement resurrected the fallen owner or killed the survivor')
        r.screenshot(args.out/'actual-survivor-recovered-composed.png')
        report['render_stats'] = r.request('render_pass_stats')
        require(all(not report['render_stats'].get(key, 0) for key in
                    ('verify_mismatch', 'watchdog', 'device_reads', 'vram_leaks')), 'Private rendering fault')
        report['success'] = True
    except Exception:
        report['error'] = traceback.format_exc()
        print(report['error'], flush=True)
        try:
            observe('failure')
            r.screenshot(args.out/'failure-composed.png')
        except Exception:
            report['failure_capture_error'] = traceback.format_exc()
    finally:
        try:
            r.release()
            if trace:
                entries = r.request('fn_entry_dump', count=4096)
                exits = r.request('fn_exit_dump', count=4096)
                (args.out/'allocation-entries.json').write_bytes((json.dumps(entries, indent=2)+'\n').encode())
                (args.out/'allocation-exits.json').write_bytes((json.dumps(exits, indent=2)+'\n').encode())
                report['raw_allocation_callers'] = dict(Counter(row['ra'] for row in entries.get('entries', [])))
                report['trace_note'] = 'Raw native/wrapper calls may duplicate; normalize using retained entries/exits before exact allocation claims.'
                r.request('fn_disable')
            observe('final')
            r.tap(8, .15, 0)
        except Exception:
            report['cleanup_error'] = traceback.format_exc()
        save()
    print(json.dumps({key: report.get(key) for key in ('success', 'owner_handoffs', 'survivor_right_pixels', 'error')}), flush=True)
    return 0 if report['success'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

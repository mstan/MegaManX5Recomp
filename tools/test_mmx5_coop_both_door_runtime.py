"""Bounded Both-door check using a real Grizzly placement and native controls.

Position/checkpoint cheats bypass the route; admission, opening, collision,
movement, menus, closing and optional owner death must execute in the game.
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
from audit_mmx5_coop_locations import counter
from test_mmx5_coop_door_runtime import require, serial

CAMERA = 0x8009A1F8


def prepare_grizzly(r, report, save, wait, ticks):
    """Reach the authored checkpoint/door approach using native loader setup only."""
    r.release()
    r.resume_native_pause()
    r.fixture(11, hp=3)
    previous = counter(r, 'mmx5.coop.enrolled')
    play = r.read(PLAY,0x20)
    continuation = play[0] == 10 and play[0x0C:0x0E] == b'\x01\x00' and r.lifecycle()[:2] == (0,0)
    report['setup_continuation'] = continuation
    save()
    r.load_location(1, 0, 3, continuation=continuation)
    deadline = time.monotonic()+90
    while True:
        play = r.read(PLAY, 0x20)
        if play[0] == 10 and not play[1] and not play[0x1C] and r.lifecycle()[:2] == (0,0) and counter(r,'mmx5.coop.enrolled') > previous:
            break
        require(time.monotonic() < deadline, 'Checkpoint arrival did not complete')
        if play[0] in (4,5,6,20) or (play[0] == 10 and play[0x1C]):
            r.tap(0x4000, .05)
        time.sleep(.04)
    wait(lambda: all(b[4:6] == b'\x01\x02' for b in r.players()), 'Native idle absent')
    if r.players()[0][0xD5]:
        r.tap(0x4010, .2, 0)
        wait(lambda: not r.players()[0][0xD5] and r.players()[0][0x70]&8, 'Native dismount failed')
    for point in ((8928,264), (9112,None), (9328,None)):
        for seat in (0,1):
            body = r.players()[seat]
            r.teleport(seat, point[0], point[1] if point[1] is not None else (74 if body[2] else 75))
        ticks(3 if point[0] == 8928 else 30)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--owner-fatal', action='store_true',
                        help='After P1 reverses and crosses, enter its native death while P2 waits outside')
    args = parser.parse_args()
    session = json.loads(args.session.read_text(encoding='utf-8'))
    exe = Path(session['command'][0]).resolve()
    require(Path(psutil.Process(session['pid']).exe()).resolve() == exe, 'Not owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest() == session['executable_sha256'], 'Binary changed')
    require(session.get('door_players') == 'both', 'Boot with --door-players both')
    r = Runtime(session['port'])
    require(not r.request('netplay_status')['active'], 'Offline only')
    args.out.mkdir(parents=True, exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    report = dict(success=False, human_validation=False, session=str(args.session.resolve()),
                  executable_sha256=session['executable_sha256'], samples=[], checks=[],
                  harness_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  case='owner-fatal-after-open' if args.owner_fatal else 'healthy',
                  session_setup_success=session.get('success'), session_setup_error=session.get('error'),
                  limits='One requested Grizzly foot/closing flow; precise checkpoint timing, boss admission/defeat, mounted doors, pre-admission death/absence, other death timings/aborts, other doors and netplay remain separate checks.',
                  bypassed_prerequisites=['native checkpoint3 loader', 'position/history approach',
                                         'HP protection during setup; removed before opening',
                                         'distinct nonfull private HP/ammo fixture before admission'])

    def save():
        (args.out/'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')

    def observe(label):
        personal = r.personal_effects()
        door = r.read(r.diagnostic()+0xA4, 16)
        actor = struct.unpack_from('<I', door, 4)[0]
        item = dict(label=label, personal=serial(personal), play=r.read(PLAY, 0x80).hex(),
                    both_door=door.hex(), camera=r.read(CAMERA, 0xFC).hex(),
                    split_cameras=r.read(r.diagnostic()+0x710, 0x1F8).hex(),
                    actor=hex(actor), actor_body=r.read(actor, 0xA4).hex() if actor else None)
        report['samples'].append(item)
        save()
        return personal

    def wait(predicate, message, seconds=20):
        deadline = time.monotonic()+seconds
        while not predicate():
            require(time.monotonic() < deadline, message)
            require(not r.state()['failed'], 'Plugin failed')
            time.sleep(.03)

    def ticks(count):
        start = r.state()['world_ticks']
        wait(lambda: r.state()['world_ticks'] >= start+count, 'World stopped')

    def phase():
        return r.read(r.diagnostic()+0xA4, 1)[0]

    def x(seat):
        return struct.unpack_from('<i', r.players()[seat], 8)[0]/65536

    def foot(seat):
        body = r.players()[seat]
        box = struct.unpack_from('<I', body, 0x68)[0]
        require(0x80010000 <= box <= 0x801FFFFC, 'Actor has no real foot box')
        shape = r.read(box, 4)
        return struct.unpack_from('<i', body, 12)[0]+(struct.unpack_from('<b', shape, 1)[0]+shape[3])*65536

    animation_trace = False

    def owned_menu(seat, label):
        r.input(8, seat)
        wait(lambda: r.read(PLAY+1,1) == b'\x02', f'P{seat+1} menu did not open')
        r.input(0, seat)
        wait(lambda: r.read(0x800D4F44,1) == b'\x01', 'Menu not interactive')
        require(r.read(r.diagnostic()+0x48,1)[0] == seat+1, 'Wrong menu owner')
        frame = r.request('frame')['frame']
        r.input(8, seat^1)
        wait(lambda: r.request('frame')['frame'] >= frame+4, 'Menu stopped polling')
        r.input(0, seat^1)
        require(r.read(PLAY+1,1) == b'\x02', 'Other actor closed owned menu')
        observe(label)
        r.resume_native_pause()
        living = r.lifecycle()
        require(all(b[2] == characters[i] for i,b in enumerate(r.players()) if living[i] == 0),
                'Menu changed a living actor identity')

    def walk_until(seat, predicate, message):
        start = previous_tick = r.state()['world_ticks']
        previous_x = x(seat)
        deadline = time.monotonic()+60
        r.input(0x20, seat)
        try:
            while not predicate():
                tick = r.state()['world_ticks']
                require(tick-start < 220 and time.monotonic() < deadline, message)
                if tick-previous_tick >= 24:
                    current_x = x(seat)
                    require(current_x > previous_x+1, message+' (no movement for24 world ticks)')
                    previous_x,previous_tick = current_x,tick
                time.sleep(.03)
        finally:
            r.input(0, seat)

    try:
        prepare_grizzly(r, report, save, wait, ticks)
        r.teleport(0, 9280, 74 if r.players()[0][2] else 75)
        r.fixture(11, hp=0)
        wait(lambda: not any(b[0xA4] for b in r.players()), 'Native immunity did not expire')
        require(r.read(PLAY+0x1D,1) == b'\x03', 'Checkpoint advanced during setup')
        before = observe('approach')
        waits = counter(r, 'mmx5.coop.both-door-admission-waits')
        r.input(0x20, 1)
        ticks(35)
        r.release()
        observe('p2-waits-alone')
        require(phase() == 0 and counter(r,'mmx5.coop.both-door-admission-waits') > waits,
                'One early player opened the Both door or never reached its trigger')
        require(r.read(PLAY+0x1D,1) == b'\x03', 'Waiting player advanced checkpoint')
        report['checks'].append('single arrival waits at native admission')
        r.fixture(2, seat=0, hp=17, ammo=73)
        r.fixture(2, seat=1, hp=23, ammo=91)
        before = r.personal_effects()
        inventory = [b[0xA8:0xC8] for b in before['bodies']]
        hp = [b[0x5C] for b in before['bodies']]
        characters = [b[2] for b in before['bodies']]
        feet = [foot(seat) for seat in (0,1)]
        # Existing native call ring; include the menus and actual crossing,
        # after the setup arrival has completed. No new game instrumentation.
        r.request('fn_filter', lo='0x8003B33C', hi='0x8003B340')
        r.request('fn_clear')
        animation_trace = True
        r.teleport(0, 9328, 74 if r.players()[0][2] else 75)
        r.input(0x20, 0)
        wait(lambda: phase() > 0, 'Both players did not acquire the native door')
        r.release()
        wait(lambda: phase() == 2, 'Door did not open with free controls')
        r.release()
        wait(lambda: all(b[4:6] == b'\x01\x02' for b in r.players()), 'Opening did not settle into native idle')
        opened = observe('open-free-controls')
        require(opened['scene'] == [0,0], 'Paired door started passenger transport')
        require(all(b[4:6] == b'\x01\x02' for b in opened['bodies']), 'Opening did not restore both actors')
        require(not r.read(PLAY+0x1C,1)[0], 'Opening retained global control lock')
        report['checks'].append('native opening restores both actors without transport')
        for seat in (0,1):
            owned_menu(seat, f'p{seat+1}-menu-while-open')
            require(phase() == 2, 'Menu advanced door traversal')
        report['checks'].append('both owned menus open and return during traversal')
        initial = [x(seat) for seat in (0,1)]
        r.input(0x80, 1)
        ticks(18)
        r.release()
        observe('p2-reverses-away')
        require(x(1) < initial[1]-12 and abs(x(0)-initial[0]) < 3,
                'Reverse movement or independent control failed')
        report['checks'].append('partner reverses freely without carrying opener')
        walk_until(0, lambda: x(0) >= 9408, 'P1 could not enter the aperture before reversing')
        initial = [x(seat) for seat in (0,1)]
        r.input(0x80,0)
        ticks(18)
        r.release()
        observe('p1-reverses-from-inside')
        require(x(0) < initial[0]-12 and abs(x(1)-initial[1]) < 3,
                'P1-left trigger trapped the opener or carried P2')
        walk_until(0, lambda: x(0) >= 9632, 'Opener could not walk through open aperture')
        report['checks'].append('P1 reverses from inside and recovers native Right control')
        r.release()
        observe('opener-deep-partner-outside')
        actor = struct.unpack('<I',r.read(r.diagnostic()+0xA8,4))[0]
        require(phase() == 2 and r.read(actor+4,2) == b'\x01\x03', 'Door closed before full pair clearance')
        report['checkpoint_while_pair_waits'] = r.read(PLAY+0x1D,1)[0]
        report['checkpoint_timing_status'] = 'unverified: leader progress can advance checkpoint4 before pair clearance; this check qualifies door state/camera commitment separately'
        old_min = struct.unpack('<h',r.read(r.diagnostic()+0xAC,2))[0]
        require(struct.unpack('<h',r.read(CAMERA+0x26,2))[0] == old_min,
                'Door camera minimum committed before pair clearance')
        require(r.personal_effects()['scene'] == [0,0], 'Boss/story scene started before pair clearance')
        require(foot(0) == feet[0], 'Aperture crossing lost the native floor')
        cameras = r.read(r.diagnostic()+0x710, 0x1F8)
        require(abs(struct.unpack_from('<h', cameras, 10)[0]-struct.unpack_from('<h', cameras, 0xFC+10)[0]) > 100,
                'Waiting actors lost independent camera positions')
        report['checks'].append('deep opener waits for physical partner clearance')
        retries = r.read(PLAY+0x45,1)
        if args.owner_fatal:
            report['bypassed_prerequisites'].append('fatal HP for P1 after opening; native death/handoff required')
            require(r.read(r.diagnostic()+0xA5,1) == b'\x00', 'P1 did not own this paired door')
            handoffs = counter(r,'mmx5.coop.both-door-owner-handoffs')
            scene_handoffs = counter(r,'mmx5.coop.dead-scene-owner-handoffs')
            survivor_x = x(1)
            r.fixture(1,seat=0)
            native_fatal = False
            signature = None
            deadline = time.monotonic()+30
            while True:
                state = r.personal_effects()
                sig = (tuple(state['life']),state['bodies'][0][4],r.read(r.diagnostic()+0xA5,1)[0])
                if sig != signature:
                    observe('owner-death-state')
                    signature = sig
                native_fatal |= state['life'][0] in (4,5) and state['bodies'][0][4] in (2,3)
                require(state['life'][1] == 0 and state['bodies'][1][4] == 1,
                        'Owner death hid or killed the survivor')
                require(state['scene'] == [0,0], 'Owner death started ordinary passenger transport')
                require(abs(x(1)-survivor_x) < 1, 'Owner death relocated the waiting survivor')
                if state['life'][0] == 5 and r.read(r.diagnostic()+0xA5,1) == b'\x01':
                    break
                require(time.monotonic()<deadline, 'Native owner death/handoff did not finish')
                require(r.read(PLAY,1) == b'\x0a', 'Owner death restarted the stage')
                time.sleep(.03)
            require(native_fatal, 'Fatal HP did not execute native death')
            require(counter(r,'mmx5.coop.both-door-owner-handoffs') == handoffs+1,
                    'Wrong paired owner handoff count')
            require(counter(r,'mmx5.coop.dead-scene-owner-handoffs') == scene_handoffs,
                    'Paired handoff used the relocating scene recovery')
            require(r.read(PLAY+0x45,1) == retries, 'Single-owner death spent a shared retry')
            report['checks'].append('native P1 death hands ownership to stationary P2 without relocation or retry')
        crossings = counter(r, 'mmx5.coop.both-door-crossings')
        walk_until(1, lambda: counter(r,'mmx5.coop.both-door-crossings') > crossings,
                   'Native door did not close after both crossed')
        r.release()
        observe('pair-cleared-native-close')
        wait(lambda: r.read(actor+4,1) == b'\x02', 'Native closing animation did not complete')
        # Checkpoint ownership/timing has its own positional trigger. Closing
        # does not prove that the surviving actor has reached that trigger.
        report['checkpoint_after_closing'] = r.read(PLAY+0x1D,1)[0]
        after = observe('native-closed')
        living = (1,) if args.owner_fatal else (0,1)
        require(all(after['bodies'][seat][0xA8:0xC8] == inventory[seat] for seat in living), 'Door changed private ammo')
        require(all(after['bodies'][seat][0x5C] == hp[seat] for seat in living), 'Door changed living HP')
        require(all(foot(seat) == feet[seat] for seat in living), 'Crossing lost native floor collision')
        require(counter(r,'mmx5.coop.both-door-map-passages') > 0, 'Scoped aperture correction was not exercised')
        report['checks'].append('native closing completes after living full boxes clear; nonfull private resources preserved')
        if args.owner_fatal:
            require(after['life'][0] == 5, 'Door revived the fallen opener')
            owned_menu(1,'survivor-menu-after-door')
            start_x = x(1)
            r.input(0x20,1)
            ticks(12)
            r.release()
            require(x(1)>start_x+1, 'Survivor lost native control after its menu')
            require(r.lifecycle()[0] == 5 and r.read(PLAY+0x45,1) == retries,
                    'Survivor menu changed death/retry ownership')
            require(r.players()[1][0x5C] == hp[1] and r.players()[1][0xA8:0xC8] == inventory[1],
                    'Survivor menu changed private resources')
            report['checks'].append('P2 survivor menu and native movement recover; P1 stays fallen')
        report['success'] = True
    except Exception as error:
        report.update(success=False, error=f'{type(error).__name__}: {error}', traceback=traceback.format_exc())
        print(report['traceback'], flush=True)
    finally:
        try:
            r.release()
            if animation_trace:
                report['native_animation_calls'] = r.request('fn_entry_dump', count=256,
                    addr_lo='0x8003B33C', addr_hi='0x8003B340')
                r.request('fn_disable')
                beam_calls = [q for q in report['native_animation_calls']['entries']
                              if int(q['a0'],16) == 0x8009A0A0 and int(q['a1'],16) in (1,3)]
                report['transport_animation_calls'] = beam_calls
                require(not beam_calls, 'Both crossing started an incoming/outgoing transport animation')
            observe('final')
            report['render_stats'] = r.request('render_pass_stats')
            require(all(not report['render_stats'].get(k,0) for k in ('verify_mismatch','watchdog','device_reads','vram_leaks')),
                    'Private render restoration fault')
            require(counter(r,'mmx5.coop.local-view-state-leak') == 0, 'Private view mutated co-op state')
            r.screenshot(args.out/'final.png')
        except Exception as error:
            report.update(success=False, final_error=str(error))
        save()
    print(json.dumps({key: report.get(key) for key in ('success','checks','error','final_error')}), flush=True)
    return 0 if report['success'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

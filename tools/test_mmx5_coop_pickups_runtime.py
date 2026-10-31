"""Native nonblocking health/camera/ownership and immediate-ammo checks.

One separated Grizzly setup, three mechanic/ownership cases. Native pickups
must collect, apply and finish; fixtures only place actors/items and seed
nonfull resources/contact protection. No progression flag is written.
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
from test_mmx5_coop_door_runtime import require, serial


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--use-current-separation', action='store_true',
                        help='Reuse an owned Grizzly foot setup with already separated views')
    args = parser.parse_args()
    session = json.loads(args.session.read_text(encoding='utf-8'))
    exe = Path(session['command'][0]).resolve()
    require(Path(psutil.Process(session['pid']).exe()).resolve() == exe, 'Not owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest() == session['executable_sha256'], 'Binary changed')
    r = Runtime(session['port'])
    require(not r.request('netplay_status')['active'], 'Offline setup only')
    args.out.mkdir(parents=True, exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    report = dict(success=False, human_validation=False, session=str(args.session.resolve()),
                  executable_sha256=session['executable_sha256'], cases=[],
                  source_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  source_evidence='Native535C8 item-kind mapping;54000 collection;53E3C/53A4C delayed HP;53B68 instant ammo',
                  bypassed_prerequisites=['native Grizzly checkpoint2 then checkpoint1 setup',
            'P2 position/history on the visible cannon-approach platform, then native jump/gravity landing',
                                          'HP protection during setup, contact protection during item checks',
                                          'native item allocated at collector, rather than finding its stage placement',
                                          'nonfull HP/auxiliary energy/weapon ammo seeded, including locked weapon slots'],
                  limits='Three ordinary item cases; no incoming damage, stage acquisition, permanent upgrades, tank policy, unlocked-weapon/menu selection or campaign completion qualification. Displayed-camera evidence requires composed-image inspection.')

    def save():
        (args.out/'report.json').write_bytes((json.dumps(report, indent=2)+'\n').encode('utf-8'))

    def wait(predicate, message, seconds=30):
        deadline = time.monotonic()+seconds
        while not predicate():
            require(time.monotonic() < deadline, message)
            require(not r.state()['failed'], 'Plugin failed')
            time.sleep(.03)

    def ticks(count):
        start = r.state()['world_ticks']
        wait(lambda: r.state()['world_ticks'] >= start+count, 'Native world stopped')

    def cameras():
        data = r.read(r.diagnostic()+0x710,0x1F8)
        return [
            [struct.unpack_from('<h',data,seat*0xFC+10)[0],
             struct.unpack_from('<h',data,seat*0xFC+14)[0]] for seat in (0,1)]

    def steady_views():
        previous = cameras()
        start = r.state()['world_ticks']
        deadline = time.monotonic()+120
        while True:
            views = cameras()
            now = r.state()['world_ticks']
            if views != previous:
                previous, start = views, now
            if now-start >= 12:
                return views
            require(time.monotonic() < deadline, 'Native cameras did not settle')
            require(r.lifecycle()[:2] == (0,0), 'Party lost during setup')
            time.sleep(.04)

    def observe(case, label, actor=None):
        p = r.personal_effects()
        sample = dict(label=label, personal=serial(p), cameras=cameras(),
                      play=r.read(PLAY,0x80).hex(), item=r.read(actor,0x9C).hex() if actor else None)
        case['samples'].append(sample)
        save()
        return p, sample

    try:
        r.release()
        report['setup'] = dict(samples=[])
        observe(report['setup'],'before-native-pause-resume')
        try:
            r.resume_native_pause()
        except TimeoutError:
            # A submitted native close can outlast the helper's short fade
            # deadline on a slow host. Observe its completion, without
            # clearing the menu or submitting more Start edges.
            report['setup']['extended_native_close_wait'] = True
            wait(lambda: r.read(PLAY+1,1) == b'\x00', 'Native pause close remained pending')
        r.fixture(11,hp=3)
        r.fixture(12,hp=3)
        report['setup']['reused_current_separation'] = args.use_current_separation
        if args.use_current_separation:
            require(r.read(PLAY+0x0C,2) == b'\x01\x00', 'Current setup is not Grizzly')
            require(r.lifecycle()[:2] == (0,0) and all(b[4:6] == b'\x01\x02' and b[0x70]&8 and
                    not b[0xD5] for b in r.players()), 'Current setup is not two grounded foot actors')
        else:
            for cp in (2,1):
                previous = counter(r,'mmx5.coop.enrolled')
                r.load_location(1,0,cp,continuation=cp == 1)
                wait_ready(r,previous)
                r.release()
                observe(report['setup'],'checkpoint-'+str(cp)+'-loaded')
                wait(lambda: all(b[4:6] == b'\x01\x02' and b[0x70]&8 for b in r.players()),
                     'Native checkpoint landing did not settle')
                if cp == 2:
                    landing = struct.unpack_from('<ii',r.players()[1],8)
                    report['observed_checkpoint2_landing'] = list(landing)
            # CP2's incoming train lands beyond the still-intact CP1 cannon
            # stop. Reusing that spawn while its platform is absent leaves
            # an unsupported actor clamped against the boundary. Use the
            # visible approach platform before the cannon instead, and let
            # native jump/gravity establish a real grounded contact.
            r.teleport(1,4100,300)
            r.input(0x4000,1)
            ticks(3)
            r.input(0,1)
            wait(lambda: r.players()[1][4:6] == b'\x01\x02' and r.players()[1][0x70]&8,
                 'Native cannon-approach landing did not settle')
            report['observed_p2_grounded_approach'] = r.players()[1].hex()
        wait(lambda: abs(cameras()[0][0]-cameras()[1][0]) > 800, 'Views did not separate')
        steady_views()
        r.fixture(11,hp=0)
        x_seat = next(i for i,b in enumerate(r.players()) if b[2] == 0)
        for name,seat,kind,amount in (('p2-large-health',1,0x21,16),
                                      ('p1-small-health',0,0x20,4),
                                      ('x-large-ammo',x_seat,0x24,None)):
            case = dict(name=name,seat=seat,item_kind=kind,success=False,samples=[],checks=[])
            report['cases'].append(case)
            r.fixture(2,seat=0,hp=13,ammo=73)
            r.fixture(2,seat=1,hp=11,ammo=91)
            baseline = steady_views()
            before, sample = observe(case,'before-item')
            bodies = before['bodies']
            positions = [struct.unpack_from('<ii',b,8) for b in bodies]
            locks=r.read(PLAY+0x10,10)
            case['freeze_locks_before']=(locks[:8]+locks[9:]).hex()
            require(before['scene'] == [0,0], 'Item setup acquired a shared scene')
            r.screenshot(args.out/(name+'-before.png'))
            r.request('fn_filter',lo='0x80053A4C',hi='0x80054004')
            r.request('fn_clear')
            r.fixture(3,seat=seat,hp=kind)
            actor = struct.unpack('<I',r.read(r.diagnostic()+0x78,4))[0]
            require(0x80010000 <= actor <= 0x801FFF60, 'Original item allocation failed')
            case['actor'] = hex(actor)
            signature = None
            deadline = time.monotonic()+30
            captured = False
            while True:
                p = r.personal_effects()
                item = r.read(actor,0x9C)
                views = cameras()
                sig = (tuple(b[0x5C] for b in p['bodies']),item[4],item[6],item[0x83])
                if sig != signature:
                    observe(case,'native-item-state',actor)
                    signature = sig
                require(p['life'][:2] == [0,0] and p['scene'] == [0,0], 'Item changed party/scene ownership')
                require(p['bodies'][seat^1][0x5C] == bodies[seat^1][0x5C] and
                        p['bodies'][seat^1][0xA8:0xC8] == bodies[seat^1][0xA8:0xC8],
                        'Pickup changed the noncollector resources')
                # The partner jumps during healing. Its own camera may follow
                # that real movement; the stationary collector keeps its view.
                checked_seats=(seat,) if amount is not None else (0,1)
                require(all(abs(views[i][axis]-baseline[i][axis]) < 32
                            for i in checked_seats for axis in (0,1)), 'Pickup pulled a view away from its owner')
                require(all(abs(struct.unpack_from('<i',p['bodies'][i],offset)[0]-positions[i][axis]) <= 65536
                            for i in (0,1) for axis,offset in enumerate((8,12))
                            if amount is None or i==seat or axis==0), 'Pickup relocated an actor')
                if amount is not None and item[4] == 2:
                    locks=r.read(PLAY+0x10,10)
                    require((locks[:8]+locks[9:]).hex()==case['freeze_locks_before'],
                            'Health pickup froze the world')
                    if 'partner_motion' not in case:
                        case['partner_motion']=dict(seat=seat^1,
                            start_y=struct.unpack_from('<i',p['bodies'][seat^1],12)[0],
                            start_world=p['state']['world_ticks'],observed=False)
                        r.input(0x4000,seat^1)
                    elif struct.unpack_from('<i',p['bodies'][seat^1],12)[0] < case['partner_motion']['start_y']-65536:
                        case['partner_motion'].update(observed=True,healing_item=item.hex(),
                            world=p['state']['world_ticks'],body=p['bodies'][seat^1].hex())
                        r.input(0,seat^1)
                if amount is not None and item[4] == 2 and not captured and case.get('partner_motion',{}).get('observed'):
                    r.screenshot(args.out/(name+'-healing.png'))
                    case['during_heal_stereo'] = r.request('stereo_stats')
                    captured = True
                if item[4] != 1 and item[4] != 2:
                    break
                require(time.monotonic() < deadline, 'Native item did not finish')
                time.sleep(.03)
            require(r.read(r.diagnostic()+0x74,1)[0] == seat+1, 'Wrong native collector')
            after, _ = observe(case,'item-finished',actor)
            if amount is not None:
                require(case.get('partner_motion',{}).get('observed'),
                        'No independent partner jump observed during native health refill')
                maximum = r.read(PLAY+0x47+bodies[seat][2],1)[0]
                effective = (5 if amount == 4 else 20) if struct.unpack_from('<I',bodies[seat],0xFC)[0]&0x40 else amount
                case['expected_health_amount'] = effective
                require(after['bodies'][seat][0x5C] == min(maximum,bodies[seat][0x5C]+effective),
                        'Native health amount/owner mismatch')
                # Original53E3C also gives the collector amount*6 auxiliary
                # energy atA8, capped by148. Weapon bars begin atAA.
                auxiliary = min(struct.unpack_from('<H',bodies[seat],0x148)[0],
                                struct.unpack_from('<H',bodies[seat],0xA8)[0]+effective*6)
                case['expected_auxiliary_energy'] = auxiliary
                require(struct.unpack_from('<H',after['bodies'][seat],0xA8)[0] == auxiliary and
                        after['bodies'][seat][0xAA:0xC8] == bodies[seat][0xAA:0xC8],
                        'Native health auxiliary-energy/weapon-resource contract mismatch')
                if name == 'p2-large-health':
                    require(captured, 'No active large-heal capture; display check remains unverified')
                case['checks'].append('native delayed HP/auxiliary-energy recovery finishes without global freeze; partner jumps during refill and resources/cameras retain their owners')
            else:
                require(bodies[seat][2] == 0, 'Ammo case requires native X')
                original = list(struct.unpack_from('<9H',bodies[seat],0xA8))
                selected = bodies[seat][0x93]
                energy = (22 if struct.unpack_from('<I',bodies[seat],0xFC)[0]&0x40 else 16)*6
                maximum = struct.unpack_from('<H',bodies[seat],0x148)[0]
                expected = [original[0]]+[min(maximum,original[i]+(energy if i == selected else energy//2))
                                           for i in range(1,9)]
                case['expected_ammo'] = expected
                require(list(struct.unpack_from('<9H',after['bodies'][seat],0xA8)) == expected,
                        'Native X ammo amount/ownership mismatch')
                require(after['bodies'][seat][0x5C] == bodies[seat][0x5C], 'Ammo changed collector health')
                case['checks'].append('native immediate ammo refill uses the collector selected-weapon/parts contract and leaves the partner unchanged')
            case['native_calls'] = r.request('fn_entry_dump',count=512,addr_lo='0x80053A4C',addr_hi='0x80054004')
            root = '0x80053A4C' if amount is not None else '0x80053B68'
            require(any(e['func'].upper() == root.upper() for e in case['native_calls']['entries']),
                    'Expected native resource routine was not recorded')
            r.request('fn_disable')
            ticks(4)
            start = struct.unpack_from('<i',r.players()[seat],8)[0]
            other = struct.unpack_from('<i',r.players()[seat^1],8)[0]
            # Checkpoint2 is against the intact cannon's right-hand stop.
            # Walk away from that boundary; walking Right is legitimately
            # clamped until the cannon is destroyed, independently of healing.
            case['control_before'] = dict(collector_x=start,partner_x=other)
            r.input(0x80,seat)
            ticks(8)
            r.release()
            positions_after = [struct.unpack_from('<i',b,8)[0] for b in r.players()]
            case['control_after'] = dict(collector_x=positions_after[seat],partner_x=positions_after[seat^1])
            require(positions_after[seat] < start-65536 and positions_after[seat^1] == other,
                    'Collector did not recover independent native input')
            observe(case,'native-control-recovered',actor)
            r.screenshot(args.out/(name+'-complete.png'))
            case['checks'].append('collector recovers native Left away from the intact cannon boundary without moving the partner')
            case['success'] = True
            save()
        report['success'] = True
    except Exception as error:
        report.update(success=False,error=str(error),traceback=traceback.format_exc())
        print(report['traceback'],flush=True)
        try:
            report['failure'] = dict(personal=serial(r.personal_effects()),cameras=cameras(),
                                     play=r.read(PLAY,0x80).hex(),
                                     native_calls=r.request('fn_entry_dump',count=512,
                                                            addr_lo='0x80053A4C',addr_hi='0x80054004'))
            r.screenshot(args.out/'failure.png')
        except Exception as capture_error:
            report['failure_capture_error'] = str(capture_error)
    finally:
        try:
            r.release()
            r.request('fn_disable')
            r.fixture(12,hp=0)
            report['render_stats'] = r.request('render_pass_stats')
            require(all(not report['render_stats'].get(k,0) for k in ('verify_mismatch','watchdog','device_reads','vram_leaks')),
                    'Private redraw restoration fault')
            require(counter(r,'mmx5.coop.local-view-state-leak') == 0, 'Private draw changed co-op state')
        except Exception as error:
            report.update(success=False,cleanup_error=str(error))
        save()
    print(json.dumps({k:report.get(k) for k in ('success','error','cleanup_error')}),flush=True)
    return 0 if report['success'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

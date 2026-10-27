"""Both setting: unavailable teammates do not block original door admission.

One authored Grizzly door, first with P2 voluntarily withdrawn, then with P1
already fallen. Native inputs/death/door updates must establish each result.
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
from test_mmx5_coop_both_door_runtime import prepare_grizzly


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session', type=Path, required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    session = json.loads(args.session.read_text(encoding='utf-8'))
    exe = Path(session['command'][0]).resolve()
    require(Path(psutil.Process(session['pid']).exe()).resolve() == exe, 'Not owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest() == session['executable_sha256'], 'Binary changed')
    require(session.get('door_players') == 'both', 'Requires the selected Both setting')
    r = Runtime(session['port'])
    require(not r.request('netplay_status')['active'], 'Offline only')
    args.out.mkdir(parents=True, exist_ok=False)
    report = dict(success=False, human_validation=False, session=str(args.session.resolve()),
                  executable_sha256=session['executable_sha256'], cases=[],
                  limits='Grizzly availability fallback and return only; other doors, mounted admission, bosses and checkpoint timing remain separate.')
    report['source_sha256'] = {}
    for name in (Path(__file__).name, 'test_mmx5_coop_both_door_runtime.py'):
        source = Path(__file__).with_name(name)
        report['source_sha256'][name] = hashlib.sha256(source.read_bytes()).hexdigest()
        (args.out/name).write_bytes(source.read_bytes())

    def save():
        (args.out/'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')

    def wait(predicate, message, seconds=30):
        deadline = time.monotonic()+seconds
        while not predicate():
            require(time.monotonic() < deadline, message)
            require(not r.state()['failed'], 'Plugin failed')
            time.sleep(.03)

    def ticks(count):
        initial = r.state()['world_ticks']
        wait(lambda: r.state()['world_ticks'] >= initial+count, 'World stopped')

    def position(seat):
        return struct.unpack_from('<ii', r.players()[seat], 8)

    def foot(seat):
        b = r.players()[seat]
        box = struct.unpack_from('<I', b, 0x68)[0]
        require(0x80010000 <= box <= 0x801FFFFC, 'Missing original foot box')
        shape = r.read(box,4)
        return struct.unpack_from('<i',b,12)[0]+(struct.unpack_from('<b',shape,1)[0]+shape[3])*65536

    def door_actor():
        pool = r.read(0x800C6A10,0x2900)
        actors = [0x800C6A10+i for i in range(0,len(pool),0xA4)
                  if pool[i] and pool[i+1:i+3] == b'\x08\x0e' and
                  struct.unpack_from('<ii',pool,i+8) == (9360*65536,64*65536)]
        require(len(actors) == 1, 'Authored Grizzly door not uniquely loaded')
        return actors[0]

    def observe(case, label, actor=None):
        case['samples'].append(dict(label=label, personal=serial(r.personal_effects()),
                                    play=r.read(PLAY,0x80).hex(),
                                    both_door=r.read(r.diagnostic()+0xA4,16).hex(),
                                    door=r.read(actor,0xA4).hex() if actor else None))
        save()

    try:
        for unavailable in ('withdrawn-p2','fallen-p1'):
            case = dict(name=unavailable, samples=[], success=False, checks=[],
                        bypassed_prerequisites=['native checkpoint3 setup', 'position/history approach',
                                                'HP/contact protection during setup', 'distinct nonfull HP/ammo fixture'])
            report['cases'].append(case)
            save()
            prepare_grizzly(r,case,save,wait,ticks)
            r.fixture(12,hp=3)
            r.fixture(11,hp=0)
            r.fixture(2,seat=0,hp=17,ammo=73)
            r.fixture(2,seat=1,hp=23,ammo=91)
            expected = r.players()
            retry = r.read(PLAY+0x45,1)
            driver,other,status = (0,1,2) if unavailable == 'withdrawn-p2' else (1,0,5)
            r.teleport(other,9280,74 if expected[other][2] else 75)
            observe(case,'before-unavailability')
            if unavailable == 'withdrawn-p2':
                r.input(1,1)
                deadline = time.monotonic()+45
                signature = None
                min_y = position(1)[1]
                while True:
                    p = r.personal_effects()
                    min_y = min(min_y,struct.unpack_from('<i',p['bodies'][1],12)[0])
                    sig = (p['life'][1],p['bodies'][1][4:7])
                    if sig != signature:
                        observe(case,'native-select-withdrawal')
                        signature = sig
                    case['withdrawal_min_y'] = min_y/65536
                    if p['life'][1] == 2:
                        break
                    require(time.monotonic() < deadline, 'Native Select withdrawal did not finish')
                    require(not p['state']['failed'], 'Plugin failed during withdrawal')
                    time.sleep(.03)
                require(min_y < 0, 'Outgoing beam did not exercise the native Y=0 boundary')
                r.release()
            else:
                case['bypassed_prerequisites'].append('fatal HP for P1; native death must finish before door admission')
                r.fixture(12,hp=2)
                wait(lambda: not r.players()[0][0xA4], 'Primary setup immunity did not expire')
                r.fixture(1,seat=0)
                signature = None
                deadline = time.monotonic()+30
                dying = False
                while r.lifecycle()[0] != 5:
                    p = r.personal_effects()
                    dying |= p['life'][0] == 4 and p['bodies'][0][4] == 2
                    sig = (p['life'][0],p['bodies'][0][4])
                    if sig != signature:
                        observe(case,'native-primary-death')
                        signature = sig
                    require(time.monotonic() < deadline, 'Native primary death did not finish')
                    require(p['life'][1] == 0, 'Primary death lost P2')
                    time.sleep(.03)
                require(dying, 'Fatal setup did not observe native dying state')
            r.release()
            require(r.lifecycle()[other] == status, 'Wrong unavailable lifecycle')
            require(r.read(PLAY+0x45,1) == retry, 'Unavailability spent a shared retry')
            require(r.read(r.diagnostic()+0xA4,1) == b'\x00', 'Setup retained a paired traversal')
            r.fixture(12,hp=0)
            wait(lambda: not r.players()[driver][0xA4], 'Living driver immunity did not expire')
            actor = door_actor()
            require(r.read(actor+4,2) == b'\x01\x00', 'Door opened before the living driver arrived')
            starts = counter(r,'mmx5.coop.both-door-starts')
            observe(case,'unavailable-before-admission',actor)
            r.input(0x20,driver)
            wait(lambda: r.personal_effects()['scene'][0] == driver+1 and bool(r.players()[driver][0xD4]),
                 'Unavailable partner blocked original door admission')
            r.release()
            observe(case,'native-single-living-admission',actor)
            # Damage/pickups can occur while the driver waits for withdrawal
            # and approaches. Compare the actual admitted inventory across
            # the door; the unavailable teammate keeps its original fixture.
            admitted = r.players()
            case['driver_resources_from'] = 'native-single-living-admission'
            signature = None
            deadline = time.monotonic()+90
            while True:
                p = r.personal_effects()
                state = r.read(actor+4,2)
                sig = (state,tuple(p['scene']),p['bodies'][driver][5],p['bodies'][driver][0xD4])
                if sig != signature:
                    observe(case,'native-door-state',actor)
                    signature = sig
                require(p['life'][other] == status, 'Door revived or returned the unavailable teammate')
                require(p['life'][driver] == 0 and not p['state']['failed'], 'Door lost the living driver')
                require(p['scene'][1] == 0, 'Unavailable teammate entered passenger transport')
                if state[0] == 2 and p['scene'] == [0,0] and p['bodies'][driver][4:6] == b'\x01\x02':
                    break
                require(time.monotonic() < deadline, 'Original single-living door did not finish')
                time.sleep(.03)
            after = r.players()
            require(counter(r,'mmx5.coop.both-door-starts') == starts, 'Unavailable partner started a paired crossing')
            require(after[driver][0x5C] == admitted[driver][0x5C] and
                    after[driver][0xA8:0xC8] == admitted[driver][0xA8:0xC8], 'Door changed living resources')
            require(after[other][0xA8:0xC8] == expected[other][0xA8:0xC8], 'Door changed unavailable inventory')
            require(r.read(PLAY+0x45,1) == retry, 'Door spent a shared retry')
            require(foot(driver) == 96*65536, 'Original door returned the driver off the native floor')
            x = position(driver)[0]
            r.input(0x20,driver)
            ticks(12)
            r.release()
            require(position(driver)[0] > x+65536, 'Living driver did not recover native movement')
            case['checks'].append('unavailable partner stays unavailable; original door/closing/control and private resources pass')
            if unavailable == 'withdrawn-p2':
                location = r.read(PLAY+0x0C,2)
                enrollment = counter(r,'mmx5.coop.enrolled')
                case['return_location_before'] = location.hex()
                case['return_checkpoint_before'] = r.read(PLAY+0x1D,1)[0]
                observe(case,'before-native-return',actor)
                r.input(1,1)
                wait(lambda: r.lifecycle()[1] == 3, 'Absent P2 did not request native return')
                r.input(0,1)
                wait(lambda: r.lifecycle()[1] == 0, 'Native P2 return did not finish')
                observe(case,'native-return-alive',actor)
                ticks(4)
                b = r.players()[1]
                require(b[0x5C] == expected[1][0x5C] and b[0xA8:0xC8] == expected[1][0xA8:0xC8],
                        'Post-door return refilled or lost private resources')
                require(foot(1) == foot(0) and b[0x70]&8, 'Post-door return was not grounded at the live partner')
                case['return_checkpoint_after'] = r.read(PLAY+0x1D,1)[0]
                require(r.read(PLAY+0x0C,2) == location and
                        counter(r,'mmx5.coop.enrolled') == enrollment,
                        'Post-door return changed native location or re-enrolled the pair')
                case['checks'].append('voluntary post-door return retains nonfull inventory and grounded feet without reload')
            observe(case,'complete',actor)
            r.screenshot(args.out/(unavailable+'.png'))
            case['success'] = True
            save()
        report['success'] = True
    except Exception as error:
        report.update(success=False,error=f'{type(error).__name__}: {error}',traceback=traceback.format_exc())
        print(report['traceback'],flush=True)
    finally:
        try:
            r.release()
            report['render_stats'] = r.request('render_pass_stats')
            require(all(not report['render_stats'].get(k,0) for k in ('verify_mismatch','watchdog','device_reads','vram_leaks')),
                    'Private redraw restoration fault')
            require(counter(r,'mmx5.coop.local-view-state-leak') == 0, 'Private redraw changed co-op state')
        except Exception as error:
            report.update(success=False,final_error=str(error))
        save()
    print(json.dumps({k:report.get(k) for k in ('success','error','final_error')}),flush=True)
    return 0 if report['success'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

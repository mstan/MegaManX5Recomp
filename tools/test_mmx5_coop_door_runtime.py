"""Original Grizzly door acquisition, return and optional owner fatal check.

Checkpoint and position/history fixtures bypass the route to the door. Native
collision, door/controller updates, checkpoint advancement and pad movement
must establish the result; no door, contact or story success flag is written.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import sys
import time
import traceback

import psutil

sys.path.insert(0, str(Path(__file__).resolve().parents[1]/'tools'))
from coop_debug import Runtime, PLAY
from audit_mmx5_coop_locations import counter


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
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--session', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--leader', type=int, choices=(0, 1), default=0)
    p.add_argument('--fatal', action='store_true')
    p.add_argument('--fatal-after-hidden',action='store_true',help='Wait for the native hidden passenger before fatal HP')
    p.add_argument('--trace-fatal',action='store_true',help='Retain targeted native HP/contact write traces around owner death')
    a = p.parse_args()
    require(not a.trace_fatal or a.fatal,'--trace-fatal requires --fatal')
    require(not a.fatal_after_hidden or a.fatal,'--fatal-after-hidden requires --fatal')
    session = json.loads(a.session.read_text(encoding='utf-8'))
    exe = Path(session['command'][0]).resolve()
    require(Path(psutil.Process(session['pid']).exe()).resolve() == exe, 'Not owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest() == session['executable_sha256'], 'Binary changed')
    r = Runtime(session['port'])
    require(not r.request('netplay_status')['active'], 'Offline only')
    a.out.mkdir(parents=True, exist_ok=False)
    (a.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    report = dict(success=False, human_validation=False, leader=a.leader, fatal_requested=a.fatal, fatal_after_hidden=a.fatal_after_hidden,
                  session=str(a.session.resolve()), executable_sha256=session['executable_sha256'],
                  harness_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(), samples=[],
                  resident_loading=session.get('resident_loading',True),
                  limits='This door/controller only; route traversal, other doors, combat damage, boss defeat and rewards remain unverified.',
                  bypassed_prerequisites=['Grizzly checkpoint3 native loader', 'position/history before original door',
                                         'HP/contact protection during position setup; contact grace expires before door entry',
                                         'fatal HP when requested'])
    def save():
        (a.out/'report.json').write_text(json.dumps(report, indent=2)+'\n',encoding='utf-8')
    def observe(label):
        s = r.personal_effects()
        play = r.read(PLAY, 0x80)
        door_pool = r.read(0x800C6A10, 0x2900)
        doors = [dict(address=hex(0x800C6A10+i), body=door_pool[i:i+0xA4].hex())
                 for i in range(0, len(door_pool), 0xA4)
                 if door_pool[i] and door_pool[i+1:i+3] == b'\x08\x0e']
        item = dict(label=label, personal=serial(s), play=play.hex(), doors=doors, camera=r.read(0x8009A1F8,0xFC).hex())
        report['samples'].append(item)
        save()
        return s, play
    try:
        r.release()
        r.resume_native_pause()
        r.fixture(11, hp=3 ^ ((1 << a.leader) if a.fatal else 0))
        r.fixture(12, hp=3)
        previous = counter(r, 'mmx5.coop.enrolled')
        r.load_location(1, 0, 3)
        arrival_deadline=time.monotonic()+90
        arrival_signature=None
        report['arrival_observations']=[]
        while True:
            play=r.read(PLAY,0x20);state=r.personal_effects()
            signature=(play[0],play[1],play[0x1C],state['state']['enrolled'],*state['life'][:2])
            if signature != arrival_signature:
                report['arrival_observations'].append(dict(play=play.hex(),personal=serial(state),native_primary=r.read(0x8009A0A0,0x158).hex()))
                arrival_signature=signature;save()
            if play[0] == 10 and counter(r,'mmx5.coop.enrolled') > previous and state['life'][:2] == [0,0] and not play[1] and not play[0x1C]:break
            require(time.monotonic()<arrival_deadline,'Original checkpoint arrival did not finish')
            if play[0] in (4,5,6,20) or (play[0] == 10 and play[0x1C] and counter(r,'mmx5.coop.enrolled') == previous):r.tap(0x4000,.04)
            time.sleep(.04)
        r.release()
        idle_deadline = time.monotonic()+30
        while not all(b[4:6] == bytes((1,2)) for b in r.players()):
            require(time.monotonic() < idle_deadline, 'Native idle did not follow checkpoint arrival')
            time.sleep(.03)
        receipt=r.read(r.diagnostic()+0x9C,8)
        report['checkpoint_receipt']=receipt.hex()
        require(receipt == bytes.fromhex('01000300ac1c0780'),'Wrong original checkpoint initializer receipt')
        observe("checkpoint-arrival")
        if r.players()[0][0xD5]:
            r.tap(0x4010, .2, 0)
            dismount_deadline = time.monotonic()+15
            while r.players()[0][0xD5] or not (r.players()[0][0x70]&8):
                require(time.monotonic() < dismount_deadline, 'Native dismount/grounding did not complete')
                time.sleep(.03)
            observe('native-dismount')
        floor_y=74 if r.players()[a.leader][2] else 75
        for x, y in ((8928,264), (9112,floor_y), (9328,floor_y)):
            r.teleport(a.leader, x, y)
            start = r.state()['world_ticks']
            route_deadline = time.monotonic()+12
            while r.state()['world_ticks'] < start+(3 if x == 8928 else 30):
                require(time.monotonic() < route_deadline, 'Native camera route stopped')
                time.sleep(.02)
            observe(f'camera-route-{x}-{y}')
        r.fixture(12, hp=1 << (a.leader^1))
        deadline = time.monotonic()+20
        while r.players()[a.leader][0xA4]:
            require(time.monotonic() < deadline, 'Native invulnerability did not expire')
            time.sleep(.03)
        s, play = observe('door-approach')
        require(s['life'][:2] == [0, 0], 'Approach lost an actor')
        report['native_checkpoint_before_door']=play[0x1D]
        require(play[0x1D] == 3,'Approach advanced checkpoint before original door collision')
        scan_deadline = time.monotonic()+15
        while not report['samples'][-1]['doors']:
            require(time.monotonic() < scan_deadline, 'Original door was not activated by placement scan')
            time.sleep(.08)
            observe('await-native-door-scan')
        survivor=a.leader^1
        r.fixture(11,hp=0)
        r.fixture(2,seat=survivor,hp=17,ammo=73)
        before=r.personal_effects();inventory=before['bodies'][survivor][0xA8:0xC8]
        retries=r.read(PLAY+0x45,1);flags=r.read(PLAY+0x4C,8)
        handoffs=counter(r,'mmx5.coop.dead-scene-owner-handoffs')
        r.screenshot(a.out/'before.png')
        r.input(0x20, a.leader)
        acquired = fatal_submitted = native_fatal = False
        deadline = time.monotonic()+90
        signature = None
        while time.monotonic() < deadline:
            s = r.personal_effects()
            bodies = s['bodies']
            sig = (tuple(s['scene']), tuple(s['life']), *[tuple(b[o] for o in (4, 5, 0x7A, 0xD0, 0xD4)) for b in bodies])
            if sig != signature:
                s, play = observe('state-change')
                signature = sig
            else:
                play = r.read(PLAY, 0x80)
            bodies=s['bodies']
            require(a.fatal or s['life'][a.leader] == 0, 'Leader died before door completion')
            require(s['life'][survivor] == 0, 'Door lost its living passenger')
            if s['scene'][0] and not acquired:
                require(s['scene'][0] == a.leader+1, 'Other hero acquired door')
                require(bodies[a.leader][0xD4], 'Scene acquisition lacked a native door lock')
                acquired = True
                r.release()
            if a.fatal and acquired and not fatal_submitted and (not a.fatal_after_hidden or s['scene'] == [a.leader+1,3]):
                require(s['scene'][0] == a.leader+1,'Door released before requested native owner fatal')
                report['scene_at_fatal_submission']=serial(s)
                if a.trace_fatal:
                    r.request('wtrace_disarm_all')
                    for lo,hi in ((0x8009A0FC,0x8009A0FD),(0x8009A0A4,0x8009A0A7),
                                  (0x8009A108,0x8009A10C),(0x8009A110,0x8009A112),
                                  (0x8009A129,0x8009A12A),(0x8009A174,0x8009A175),
                                  (PLAY+0x10,PLAY+0x1C),(0x8009A0A8,0x8009A0B0)):
                        r.request('wtrace_arm',lo=hex(lo),hi=hex(hi))
                    r.request('wtrace_reset')
                r.fixture(1, seat=a.leader)
                report['fatal_ack_frame'] = r.request('frame')['frame']
                fatal_submitted = True
            if fatal_submitted and (bodies[a.leader][4] == 2 or s['life'][a.leader] in (4, 5)):
                if s['scene'][0]:
                    native_fatal = True
                    report['native_fatal_during_scene'] = True
            if acquired and not s['scene'][0] and not play[0x1C]:
                require(not s['state']['failed'], 'Plugin failed')
                report['native_fatal_during_scene'] = native_fatal
                if a.fatal and not native_fatal:
                    report['unverified'] = 'Native script deferred fatal entry until after scene; not P17 coverage'
                observe('scene-complete')
                restored_deadline=time.monotonic()+30
                while True:
                    after=r.personal_effects()
                    driver=survivor if a.fatal else a.leader
                    if (not a.fatal or after['life'][a.leader] == 5) and after['life'][driver] == 0 and after['bodies'][driver][4:6] == bytes((1,2)) and after['bodies'][survivor][4:6] == bytes((1,2)):break
                    require(time.monotonic()<restored_deadline,'Door return did not restore original control')
                    time.sleep(.03)
                require(after['bodies'][survivor][0x5C] == 17 and after['bodies'][survivor][0xA8:0xC8] == inventory,'Door changed passenger resources')
                require(r.read(PLAY+0x45,1) == retries and r.read(PLAY+0x4C,8) == flags,'Door changed retries or boss outcomes')
                report['handoffs']=counter(r,'mmx5.coop.dead-scene-owner-handoffs')-handoffs
                require(report['handoffs'] == int(a.fatal),'Wrong fatal-owner handoff count')
                play_after=r.read(PLAY,0x80)
                report['native_checkpoint_after_door']=play_after[0x1D]
                require(play_after[0x1D] == 4,'Original door did not advance checkpoint3 to4')
                anchors=[]
                for seat,body in enumerate(after['bodies']):
                    box=struct.unpack_from('<I',body,0x68)[0] or 0x800748D0+body[2]*4
                    shape=r.read(box,4)
                    foot=struct.unpack_from('<i',body,12)[0]+(struct.unpack_from('<b',shape,1)[0]+shape[3])*65536
                    anchors.append(dict(seat=seat,box=hex(box),shape=shape.hex(),foot_fixed=foot))
                report['foot_anchors']=anchors
                require(anchors[0]['foot_fixed'] == anchors[1]['foot_fixed'],'Door return mismatched native foot points')
                report['native_controls_after_return']=[]
                for driver in ((survivor,) if a.fatal else (a.leader,survivor)):
                    after=r.personal_effects();x_before=struct.unpack_from('<i',after['bodies'][driver],8)[0]
                    start=after['state']['world_ticks'];r.input(0x20,driver)
                    while r.state()['world_ticks']<start+12:
                        require(time.monotonic()<restored_deadline,'Door survivor world stopped');time.sleep(.02)
                    r.release();moved=r.personal_effects()
                    require(struct.unpack_from('<i',moved['bodies'][driver],8)[0]>x_before+65536,'Door return did not allow native Right movement')
                    while r.personal_effects()['bodies'][driver][4:6] != bytes((1,2)):
                        require(time.monotonic()<restored_deadline,'Door movement did not settle');time.sleep(.02)
                    report['native_controls_after_return'].append(driver)
                r.screenshot(a.out/'after.png')
                report['success'] = not a.fatal or native_fatal
                break
            # Native conversation confirmation only; never a story flag.
            if acquired and play[0x1C] and bodies[a.leader][0xD0]:
                r.tap(0x4000, .08, a.leader)
            time.sleep(.025)
        else:
            raise TimeoutError('Original door/scene did not finish')
        print(json.dumps({key: report.get(key) for key in ('success', 'native_fatal_during_scene', 'unverified')}), flush=True)
    except Exception as error:
        report.update(success=False,error=f'{type(error).__name__}: {error}', traceback=traceback.format_exc())
        print(report['traceback'], flush=True)
    finally:
        try:
            r.release()
            if a.trace_fatal:
                r.request('wtrace_disarm_all')
                for name,lo,hi in (('hp',0x8009A0FC,0x8009A0FD),('mode',0x8009A0A4,0x8009A0A7),
                                   ('box',0x8009A108,0x8009A10C),('contacts',0x8009A110,0x8009A112),
                                   ('contact-history',0x8009A129,0x8009A12A),('door',0x8009A174,0x8009A175)):
                    trace=r.request('wtrace_dump',addr_lo=hex(lo),addr_hi=hex(hi),count=2048,newest=1)
                    (a.out/('native-write-trace-'+name+'.json')).write_text(json.dumps(trace,indent=2)+'\n',encoding='utf-8')
            observe('final')
            report['render_stats'] = r.request('render_pass_stats')
            require(all(not report['render_stats'].get(k,0) for k in ('verify_mismatch','watchdog','device_reads','vram_leaks')),'Door private redraw restoration fault')
            r.screenshot(a.out/'final.png')
        except Exception as error:
            report['success']=False
            report['final_error'] = str(error)
        save()
    return 0 if report['success'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

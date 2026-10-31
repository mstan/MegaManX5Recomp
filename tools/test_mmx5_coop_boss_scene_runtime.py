"""Actual original Grizzly encounter/scene check; door traversal is bypassed."""
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
from test_mmx5_coop_scene_fatal_runtime import serial, require


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--session', type=Path, required=True)
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--leader', type=int, choices=(0,1), default=1)
    p.add_argument('--fatal', action='store_true')
    a = p.parse_args()
    parent = json.loads(a.session.read_text(encoding='utf-8'))
    exe = Path(parent['command'][0]).resolve()
    require(Path(psutil.Process(parent['pid']).exe()).resolve() == exe, 'Not owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest() == parent['executable_sha256'], 'Binary changed')
    r = Runtime(parent['port'])
    require(not r.request('netplay_status')['active'], 'Offline fixture only')
    a.out.mkdir(parents=True, exist_ok=False)
    (a.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    report = dict(success=False, human_validation=False, leader=a.leader, fatal_requested=a.fatal,
                  samples=[], session=str(a.session.resolve()), executable_sha256=parent['executable_sha256'],
                  resident_loading=parent.get('resident_loading', True),
                  harness_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                  bypassed_prerequisites=['Original stage1 checkpoint4 loader; preceding door/traversal bypassed',
                                         'HP protection for arrival, then waiting survivor HP17/ammo73 and contact protection',
                                         'Leader fatal HP when requested'],
                  limits='Native boss activation, scene ownership/return and control only; no door, boss defeat, reward or campaign outcome pass.')
    def save():
        (a.out/'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    def bosses():
        pool = r.read(0x80092090, 48*0x9C)
        return [dict(address=hex(0x80092090+i), body=pool[i:i+0x9C].hex())
                for i in range(0,len(pool),0x9C) if pool[i] and pool[i+1] == 0x36]
    def observe(label):
        s = r.personal_effects()
        report['samples'].append(dict(label=label, personal=serial(s), play=r.read(PLAY,0x80).hex(), bosses=bosses()))
        save()
        return s
    def wait(predicate, label, timeout=25):
        deadline = time.monotonic()+timeout
        while True:
            s = r.personal_effects()
            if predicate(s):return s
            require(time.monotonic() < deadline, label)
            time.sleep(.03)
    try:
        r.release();r.resume_native_pause();r.fixture(11,hp=3);r.fixture(12,hp=0)
        previous = counter(r,'mmx5.coop.enrolled')
        r.load_location(1,0,4)
        deadline = time.monotonic()+90
        while True:
            play = r.read(PLAY,0x20);s = r.personal_effects()
            if play[0] == 10 and counter(r,'mmx5.coop.enrolled') > previous and s['life'][:2] == [0,0]:
                require(not s['scene'][0], 'Boss acquired a scene before the controlled approach')
                if not play[1] and not play[0x1C]:break
            require(time.monotonic() < deadline, 'Original checkpoint arrival did not finish')
            # Original title/briefing confirmation only. Never consume a boss
            # conversation during setup, nor set its activation/success flags.
            if play[0] in (4,5,6,20):r.tap(0x4000,.04)
            time.sleep(.04)
        r.release()
        wait(lambda s: all(b[4:6] == bytes((1,2)) for b in s['bodies']), 'Arrival did not settle')
        require(r.read(0x800EE970,4) == bytes((5,0,0,0)), 'Wrong original boss overlay')
        survivor = a.leader^1
        r.fixture(11,hp=0);r.fixture(12,hp=1<<survivor)
        r.fixture(2,seat=survivor,hp=17,ammo=73)
        before = observe('original-corridor-control')
        inventory = before['bodies'][survivor][0xA8:0xC8]
        retries = r.read(PLAY+0x45,1);flags = r.read(PLAY+0x4C,8)
        handoffs = counter(r,'mmx5.coop.dead-scene-owner-handoffs')
        r.input(0x20,a.leader)
        started = wait(lambda s: bool(s['scene'][0]) or s['life'][a.leader] != 0,
                       'Native walk did not activate the boss scene', 35)
        r.release()
        require(started['life'][a.leader] == 0 and started['scene'][0] == a.leader+1,
                'Native boss scene did not belong to the approaching living hero')
        wait(lambda s: s['scene'] == [a.leader+1,3], 'Native passenger departure did not finish')
        locked = observe('original-boss-scene-passenger-hidden')
        require(report['samples'][-1]['bosses'], 'No original boss actor active')
        require(not locked['bodies'][survivor][3] and locked['bodies'][survivor][4] == 3,
                'Actual encounter did not establish a hidden living passenger')
        r.screenshot(a.out/'locked.png')
        r.fixture(12,hp=3)
        report['bypassed_prerequisites'].append('Both heroes have contact protection after original scene acquisition; combat damage is unqualified')
        if a.fatal:
            r.fixture(1,seat=a.leader)
            wait(lambda s: s['life'][a.leader] in (4,5), 'Original owner fatal entry did not run')
            observe('original-boss-owner-fatal')
        driver = survivor if a.fatal else a.leader
        deadline = time.monotonic()+90
        while True:
            s = r.personal_effects();play = r.read(PLAY,0x20)
            b = s['bodies'][driver]
            if not s['scene'][0] and not play[1] and not play[0x1C] and b[4:6] == bytes((1,2)) and \
               (not a.fatal or s['life'][a.leader] == 5):break
            require(time.monotonic() < deadline, 'Original boss controller did not finish the scene')
            require(s['life'][driver] == 0, 'Original scene lost its living hero')
            require(r.read(PLAY+0x45,1) == retries, 'Living passenger caused an unwanted team retry')
            if play[0x1C]:r.tap(0x4000,.08,driver)
            time.sleep(.03)
        after = observe('original-boss-controller-released')
        require(after['bodies'][survivor][0x5C] == 17 and after['bodies'][survivor][0xA8:0xC8] == inventory,
                'Actual scene return changed passenger resources')
        require(r.read(PLAY+0x45,1) == retries and r.read(PLAY+0x4C,8) == flags,
                'Scene altered retries or boss completion outcomes')
        report['handoffs'] = counter(r,'mmx5.coop.dead-scene-owner-handoffs')-handoffs
        require(report['handoffs'] == int(a.fatal), 'Actual scene did not hand ownership over exactly once')
        anchors = []
        for seat, body in enumerate(after['bodies']):
            box = struct.unpack_from('<I', body, 0x68)[0]
            if not box:
                # Native3CD08's original standing tables; a corpse has no
                # active box but retains the scene's committed center Y.
                box = 0x800748D0+body[2]*4
            shape = r.read(box,4)
            bottom = struct.unpack_from('<b',shape,1)[0]+shape[3]
            point = struct.unpack_from('<i',body,12)[0]+bottom*65536
            anchors.append(dict(seat=seat,box=hex(box),shape=shape.hex(),foot_fixed=point))
        report['foot_anchors'] = anchors
        require(anchors[0]['foot_fixed'] == anchors[1]['foot_fixed'],
                'Transport mismatched the original character foot anchors')
        body = after['bodies'][driver]
        x = struct.unpack_from('<i',body,8)[0]
        r.input(0x20,driver)
        # Resource/foot-box observations above issue several debug requests.
        # Count future world updates after submitting input; the old scene
        # snapshot can already be more than12 updates behind at this point.
        start = r.personal_effects()['state']['world_ticks']
        moved = wait(lambda s: s['state']['world_ticks'] >= start+12, 'Original world stopped')
        r.release()
        require(struct.unpack_from('<i',moved['bodies'][driver],8)[0] > x+65536, 'Scene hero could not move natively')
        wait(lambda s: s['bodies'][driver][4:6] == bytes((1,2)), 'Native movement did not settle')
        observe('original-boss-control-restored')
        r.screenshot(a.out/'after.png')
        report['render_stats'] = stats = r.request('render_pass_stats')
        require(all(not stats.get(k,0) for k in ('verify_mismatch','watchdog','device_reads','vram_leaks')),
                'Private rendering fault in actual encounter')
        report['success'] = True
        print('PASS original boss scene; leader',a.leader,'fatal',a.fatal,flush=True)
    except Exception as error:
        report.update(error=f'{type(error).__name__}: {error}',traceback=traceback.format_exc())
        print(report['traceback'],flush=True)
    finally:
        try:r.release();observe('final')
        except Exception as error:report['final_error'] = str(error)
        save()
    return 0 if report['success'] else 1


if __name__ == '__main__':
    raise SystemExit(main())

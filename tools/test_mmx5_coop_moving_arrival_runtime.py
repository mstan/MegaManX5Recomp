"""X4 feedback6 analogue: P1 walks during one P2 return, with native pose trace."""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time
import traceback

import psutil
from coop_debug import Runtime, PLAY, PLAYER
from audit_mmx5_coop_locations import counter, wait_ready


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--session',type=Path,required=True)
    p.add_argument('--out',type=Path,required=True)
    a=p.parse_args()
    session=json.loads(a.session.read_text(encoding='utf-8'))
    exe=Path(session['command'][0]).resolve()
    assert Path(psutil.Process(session['pid']).exe()).resolve()==exe,'Not owned process'
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==session['executable_sha256'],'Binary changed'
    r=Runtime(session['port'])
    assert not r.request('netplay_status')['active'],'Offline fixture only'
    a.out.mkdir(parents=True,exist_ok=False)
    source=Path(__file__).read_bytes();(a.out/'harness.py').write_bytes(source)
    report=dict(success=False,executable_sha256=session['executable_sha256'],
                session=str(a.session.resolve()),harness_sha256=hashlib.sha256(source).hexdigest(),samples=[],
                bypassed_prerequisites=['Native intro checkpoint0 setup bypasses selection/traversal',
                                       'Seeded P2 HP17/ammo73 and native contact protection'],
                limits='One moving-owner Select return on the intro floor; other geometry, incoming damage and network remain unverified.')
    def wait(predicate,seconds=15):
        deadline=time.monotonic()+seconds
        while not predicate():
            if time.monotonic()>deadline:raise TimeoutError('Native arrival prerequisite timed out')
            time.sleep(.02)
    try:
        r.release();r.resume_native_pause();r.fixture(11,hp=3);r.fixture(12,hp=0)
        previous=counter(r,'mmx5.coop.enrolled')
        r.load_location(0,0,0);wait_ready(r,previous);r.release()
        wait(lambda:all(b[4:6]==bytes((1,2)) and b[0x70]&8 for b in r.players()))
        r.fixture(11,hp=0);r.fixture(12,hp=3);r.fixture(2,seat=1,hp=17,ammo=73)
        r.input(1,1);wait(lambda:r.lifecycle()[1]==2);r.input(0,1)
        submitted=r.state()['world_ticks'];wait(lambda:r.state()['world_ticks']>=submitted+2)
        before=r.players();hp=before[1][0x5C];inventory=before[1][0xA8:0xC8]
        x0=struct.unpack_from('<i',before[0],8)[0]
        budget=r.read(PLAY+0x45,1);flags=r.read(PLAY+0x4C,8);checkpoint=r.read(PLAY+0x1D,1)
        applied=counter(r,'mmx5.coop.fixture-checkpoints-applied')
        enrolled=counter(r,'mmx5.coop.enrolled')
        r.request('fn_clear');r.request('fn_filter',lo='0x8003B33C',hi='0x8003B340')
        r.input(0x20,0);r.tap(1,.06,1)
        deadline=time.monotonic()+20;joining=False
        while True:
            s=r.personal_effects();b=s['bodies'][1]
            report['samples'].append(dict(world=s['state']['world_ticks'],life=s['life'],
                p1_x=struct.unpack_from('<i',s['bodies'][0],8)[0],
                p2_xy=list(struct.unpack_from('<ii',b,8)),state=list(b[3:7]),hp=b[0x5C]))
            joining|=s['life'][1]==3
            if s['life'][1] in (4,5):raise AssertionError('Return killed P2')
            if s['life'][1]==0:
                assert joining,'Arrival was not observed; single-beam check unqualified'
                break
            if time.monotonic()>deadline:raise TimeoutError('Moving owner stranded the native return')
            time.sleep(.01)
        # Stop the trace before releasing P1, whose idle transition also uses
        # the native pose setter. Pose1 is the one incoming start, not a draw.
        r.request('fn_disable');r.release()
        trace=r.request('fn_entry_dump',count=128)
        (a.out/'native-pose-starts.json').write_text(json.dumps(trace,indent=2)+'\n',encoding='utf-8')
        assert trace['seq_lo']==0 and trace['total']==len(trace['entries']), 'Pose trace truncated; exact-start count unqualified'
        starts=[e for e in trace['entries'] if int(e['a0'],16)==PLAYER and int(e['a1'],16)==1]
        assert len(starts)==1,f'Expected one native incoming start, observed{len(starts)}'
        after=r.players()
        report.update(native_incoming_starts=len(starts),owner_motion_fixed=struct.unpack_from('<i',after[0],8)[0]-x0,
                      return_hp=after[1][0x5C],return_inventory=after[1][0xA8:0xC8].hex())
        assert report['owner_motion_fixed']>8*65536,'Owner did not move during arrival'
        assert after[1][0x5C]==hp and after[1][0xA8:0xC8]==inventory,'Return changed P2 resources'
        assert after[0][0x5C]==before[0][0x5C],'Return changed P1 health'
        assert r.read(PLAY+0x45,1)==budget and r.read(PLAY+0x4C,8)==flags,'Return changed team outcomes'
        assert r.read(PLAY+0x1D,1)==checkpoint and counter(r,'mmx5.coop.enrolled')==enrolled,'Return reloaded the stage'
        assert counter(r,'mmx5.coop.fixture-checkpoints-applied')==applied,'Return reapplied a checkpoint'
        r.screenshot(a.out/'returned.png');report['success']=True
        print('PASS moving P1, one native P2 incoming start, retained HP/ammo and no reload',flush=True)
    except Exception:
        report['error']=traceback.format_exc();print(report['error'],flush=True)
    finally:
        try:
            r.request('fn_disable');r.release();r.fixture(12,hp=0)
            if r.lifecycle()[0]==0 and r.read(PLAY+1,1)==b'\x00':r.tap(8,.05)
        except Exception:report['cleanup_error']=traceback.format_exc()
        (a.out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    return 0 if report['success'] else 1


if __name__=='__main__':raise SystemExit(main())

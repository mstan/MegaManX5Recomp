"""One untouched CP0 timed-bomb expiry with separated native cameras.

Use an existing private Host session. Mirror --leader 0/1 in separate cases.
Only the earlier CP0 route and selected-body position/history are bypassed.
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


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--session',type=Path,required=True);p.add_argument('--out',type=Path,required=True)
    p.add_argument('--leader',type=int,choices=(0,1),required=True);a=p.parse_args()
    session=json.loads(a.session.read_text());exe=Path(session['command'][0]).resolve()
    assert Path(psutil.Process(session['pid']).exe()).resolve()==exe
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==session['executable_sha256']
    r=Runtime(session['port']);assert not r.request('netplay_status')['active']
    r.release=lambda:[r.input(0,seat) for seat in (0,1)]
    a.out.mkdir(parents=True,exist_ok=False)
    d={'success':False,'leader':a.leader,'session':str(a.session),'binary_sha256':session['executable_sha256'],
       'scope':'Native CP0 first bomb HP8, original171-update animation expiry, real +82=1/class5 flash/class8 blast; separated cameras with ordinary damage. CP0 loader bypasses preceding mission entry; the approach, grounding and timer wait use native input only. No position/timer/HP/flash/phase/PLAY/camera/bounds writes.', 'samples':[]}

    def observe(label):
        q=r.personal_effects();enemies=r.read(0x80092090,48*0x9c)
        bomb=next((enemies[i:i+0x9c]for i in range(0,len(enemies),0x9c)
                   if enemies[i]and enemies[i+1]==0x1f and struct.unpack_from('<h',enemies,i+10)[0]==576),None)
        stage=r.read(0x8009e7a0,0x1200);effects=r.read(0x80094398,0x2a00)
        flashes=[stage[i:i+0x30].hex()for i in range(0,len(stage),0x30)if stage[i]and stage[i+1]==5]
        blasts=[effects[i:i+0x70].hex()for i in range(0,len(effects),0x70)
                if effects[i]and effects[i+1]==8 and struct.unpack_from('<hh',effects,i+10)[0]==576]
        cams=r.read(r.diagnostic()+0x710,0x1f8)
        row={'label':label,'world':q['state']['world_ticks'],'life':q['life'][:2],
             'xy':[list(v/65536 for v in struct.unpack_from('<ii',b,8))for b in q['bodies']],
             'HP':[b[0x5c]for b in q['bodies']],'contacts':[[b[k]for k in(0x70,0x71,0x89,0xf9)]for b in q['bodies']],
             'cameras':[list(v/65536 for v in struct.unpack_from('<ii',cams,i+8))for i in(0,0xfc)],
             'camera_bounds':[cams[i+0x1c:i+0x2c].hex()for i in(0,0xfc)],
             'bomb':bomb.hex()if bomb else None,'flashes':flashes,'blasts':blasts,'play_locks':r.read(PLAY+0x10,0x10).hex()}
        d['samples'].append(row)
        assert not q['state']['failed']and q['life'][:2]==[0,0],'Lost native living pair'
        return row,q,bomb

    def until(label,test,seconds=35):
        end=time.monotonic()+seconds
        while True:
            row,q,b=observe(label)
            if test(row,q,b):return row,q,b
            assert time.monotonic()<end,label
            time.sleep(.005)

    try:
        r.release();r.resume_native_pause();r.fixture(11,hp=0);r.fixture(12,hp=0)
        old=counter(r,'mmx5.coop.enrolled');r.load_location(8,0,0);wait_ready(r,old)
        row,q,b=until('native-CP0',lambda row,q,b:all(x[4:6]==b'\x01\x02'and x[0x70]&8 for x in q['bodies']))
        d['entry']=row;r.input(0x20,a.leader)
        row,q,b=until('native-bomb-viewport-admission',lambda row,q,b:b is not None and b[4]==1 and b[0x84]!=0)
        r.release();assert b[0x5c]==8 and not struct.unpack_from('<H',b,0x82)[0]
        d['untouched_admission']=row
        # Real viewport admission already separates these cameras by >250px.
        # Stay on that authentic floor; CP3-derived map geometry is not a
        # valid reason to place a CP0 hero under its uncleared ceiling.
        start=r.state()['world_ticks']
        row,q,b=until('native-separated-ground',lambda row,q,b:row['world']>=start+12 and all(x[0x70]&8 for x in q['bodies']))
        d['native_input_only_setup']=row
        base=row;assert base['cameras'][a.leader][0]-base['cameras'][a.leader^1][0]>=200,'Native cameras did not separate'
        end=time.monotonic()+90
        while True:
            row,q,b=observe('native-timed-expiry-and-flash')
            if b is not None and b[4]==1:assert b[0x5c]==8,'Bomb was damaged before expiry'
            if b is not None and b[4]==2 and struct.unpack_from('<H',b,0x82)[0]==1:d.setdefault('native_timed_retirement',row)
            if any(bytes.fromhex(v)[4]in(1,2)for v in row['flashes']):d.setdefault('native_flash',row)
            if row['blasts']:d.setdefault('native_blast',row)
            assert all(abs(row['xy'][s][0]-base['xy'][s][0])<32 for s in(0,1)),'Unrelated native horizontal displacement'
            assert row['cameras'][a.leader][0]-row['cameras'][a.leader^1][0]>=200,'Timed flash collapsed separated views'
            if all(k in d for k in('native_timed_retirement','native_flash','native_blast'))and not row['flashes']:
                d['native_post_flash']=row;d['success']=True;break
            assert time.monotonic()<end,'Original timer/flash/blast/post-flash observations incomplete'
            time.sleep(.005)
        r.screenshot(a.out/'native-post-flash.png')
    except Exception:
        d['error']=traceback.format_exc()
        try:r.screenshot(a.out/'failure.png')
        except Exception:pass
    finally:
        r.release();(a.out/'report.json').write_text(json.dumps(d,indent=2)+'\n')
    print(json.dumps({'success':d['success'],'leader':a.leader,'error':d.get('error')}),flush=True)
    return int(not d['success'])


if __name__=='__main__':raise SystemExit(main())

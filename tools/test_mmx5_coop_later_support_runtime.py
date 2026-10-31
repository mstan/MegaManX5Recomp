"""Distinct stage8 support ownership, motion, camera and restore checks.

Original checkpoint setup skips the preceding route. Native approach/jump,
collision and callbacks admit the support; no contact or phase flag is set.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time
import traceback

import psutil
from coop_debug import Runtime,PLAY
from audit_mmx5_coop_locations import counter,wait_ready
from test_mmx5_coop_scene_fatal_runtime import serial
from test_mmx5_coop_snapshot_runtime import state_operation


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--checkpoint',type=int,choices=[2,3],required=True)
    args=parser.parse_args();cp=args.checkpoint;sub=cp-1;moving=6 if cp==2 else 7
    p=json.loads(args.session.read_text(encoding='utf-8'));exe=Path(p['command'][0]).resolve()
    assert Path(psutil.Process(p['pid']).exe()).resolve()==exe and hashlib.sha256(exe.read_bytes()).hexdigest()==p['executable_sha256']
    r=Runtime(p['port']);assert not r.request('netplay_status')['active']
    args.out.mkdir(parents=True,exist_ok=False);(args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    d=dict(success=False,session=str(args.session.resolve()),candidate_sha256=p['executable_sha256'],
           checkpoint=cp,subtype=sub,samples=[],checks={},operations=[],
           bypasses=['Original native8/0 checkpoint loader bypasses earlier traversal','HP refillboth only; native contact/hurt/pit/crush remain enabled'],
           limits='One distinct support subtype, living P1 behind/P2 leads. CP2 attempts a full authored stop after actual mid-motion save/load; any terrain/hero loss keeps the overall probe failed. CP3 bounded upward carry only; later cycle/exit remains unverified.')

    def save():
        (args.out/'report.json').write_bytes((json.dumps(d,indent=2)+'\n').encode())

    def xy(b):
        return tuple(v/65536 for v in struct.unpack_from('<ii',b,8))

    def obs(label):
        s=r.personal_effects();pool=r.read(0x800c6a10,64*0xa4)
        objs=[dict(address=hex(0x800c6a10+i),body=pool[i:i+0xa4].hex()) for i in range(0,len(pool),0xa4)
              if pool[i] and pool[i+1:i+3]==bytes([0x12,sub])]
        v=dict(label=label,personal=serial(s),parents=objs,play=r.read(PLAY,0x1c0).hex(),
               split_cameras=r.read(r.diagnostic()+0x710,0x1f8).hex(),frame=r.request('frame')['frame'])
        d['samples'].append(v);assert not s['state']['failed'],'Plugin failed'
        return v,s,bytes.fromhex(objs[0]['body']) if objs else None

    def until(label,test,seconds=20):
        end=time.monotonic()+seconds
        while True:
            v,s,o=obs(label)
            assert s['life'][:2]==[0,0],'Lost living pair'
            if test(s,o):return v,s,o
            assert time.monotonic()<end,label
            time.sleep(.035)

    try:
        r.input(0,0);r.input(0,1);r.resume_native_pause();r.fixture(11,hp=3);r.fixture(12,hp=0)
        previous=counter(r,'mmx5.coop.enrolled')
        r.load_location(8,0,cp,continuation=r.lifecycle()[:2]==(0,0) and r.read(PLAY+12,2)==b'\x08\0')
        wait_ready(r,previous)
        v,s,o=until('native-entry',lambda s,o:all(b[4:6]==b'\x01\x02' and b[0x70]&8 for b in s['bodies']))
        d['entry']=v;first_xy=xy(s['bodies'][0]);before=counter(r,'mmx5.coop.p2-later-support-admissions')
        r.screenshot(args.out/'native-entry.png');save();r.input(0x20,1)
        if cp==2:
            # Native jump from the last safe authored ledge, across the gap
            # onto the support. A naked walk instead slides below its top.
            v,s,o=until('native-ledge-approach',lambda s,o:xy(s['bodies'][1])[0]>=2544)
            r.input(0x4020,1);start=r.state()['world_ticks']
            until('native-gap-jump',lambda s,o:s['state']['world_ticks']>=start+12)
            r.input(0x20,1)
        v,s,o=until('p2-native-contact-admission',lambda s,o:o is not None and o[4]==1 and
                    o[5]==moving and s['bodies'][1][0x71]&8)
        r.input(0,1)
        assert xy(s['bodies'][0])[0]==first_xy[0],'P1 left its original X during admission'
        assert counter(r,'mmx5.coop.p2-later-support-admissions')==before+1,'P2 admission not exactly once'
        d['checks']['native_p2_admission_and_top_contact']=dict(status='passed',sample=v)
        start_xy=xy(o);r.screenshot(args.out/'p2-native-admission.png');save()
        v,s,o=until('actual-owned-motion',lambda s,o:o is not None and
                    (xy(o)[0]>=start_xy[0]+24 if cp==2 else xy(o)[1]<=start_xy[1]-64) and s['bodies'][1][0x71]&8,seconds=30)
        assert o[5]==moving and s['bodies'][1][0x71]&8 and xy(s['bodies'][0])[0]==first_xy[0]
        d['checks']['p2_owned_native_motion_and_carry']=dict(status='passed',sample=v)
        r.screenshot(args.out/'p2-actual-motion.png');save()
        if cp==2:
            d['operations'].append(state_operation(r,'save',6));v,s,o=obs('actual-moving-save');saved_x=xy(o)[0];save()
            until('native-advance-before-restore',lambda s,o:o is not None and xy(o)[0]>=saved_x+32)
            d['operations'].append(state_operation(r,'load',6));v,s,o=obs('actual-moving-restore')
            assert o is not None and o[5]==6 and xy(o)[0]<saved_x+16,'Original support position not restored'
            restored_x=xy(o)[0];save()
            v,s,o=until('restored-owner-native-motion',lambda s,o:o is not None and xy(o)[0]>=restored_x+24 and s['bodies'][1][0x71]&8)
            assert xy(s['bodies'][0])[0]==first_xy[0] and s['bodies'][1][0x71]&8
            d['checks']['mid_motion_owner_save_restore']=dict(status='passed',sample=v)
            v,s,o=until('original-diagonal-destination',lambda s,o:o is not None and o[5]==8 and s['bodies'][1][0x71]&8,seconds=70)
            assert xy(o)[0]>=3368 and s['bodies'][1][0x71]&8 and xy(s['bodies'][0])[0]==first_xy[0]
            d['checks']['native_diagonal_destination']=dict(status='passed',sample=v)
            r.screenshot(args.out/'native-destination.png')
        d['success']=True
    except Exception:
        d['error']=traceback.format_exc()
        try:d['failure']=obs('failure')[0];r.screenshot(args.out/'failure.png')
        except Exception:d['capture_error']=traceback.format_exc()
    finally:
        r.input(0,0);r.input(0,1);save()
    print(json.dumps({k:d.get(k)for k in ['success','checkpoint','error']}),flush=True)
    return 0 if d['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())

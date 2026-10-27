"""Complete a native mandatory Dynamo gate with actual entry, hit and retreat.

Original selector state determines first/second encounter. HP-only hero refill
and one positive boss-HP setup are allowed; native buster input must cause
death. No launch, script, defeat, reward or progression flag is fabricated.
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


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args();parent=json.loads(args.session.read_text(encoding='utf-8'))
    exe=Path(parent['command'][0]).resolve();assert Path(psutil.Process(parent['pid']).exe()).resolve()==exe
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==parent['executable_sha256']
    r=Runtime(parent['port']);assert not r.request('netplay_status')['active']
    args.out.mkdir(parents=True,exist_ok=False);(args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    d=dict(success=False,session=str(args.session.resolve()),candidate_sha256=parent['executable_sha256'],samples=[],inputs=[],text_pages=[],writes=[],
           limits='One actually selected mandatory Dynamo encounter. Native report/Save/selector qualified separately; no full attack-pattern, route, production or netplay pass.')
    actor=None

    def save():
        (args.out/'report.json').write_bytes((json.dumps(d,indent=2)+'\n').encode())

    def obs(label):
        nonlocal actor
        s=r.personal_effects();p=r.read(PLAY,0x1c0)
        if p[0]==10 and p[12]==stage:
            pool=r.read(0x80092090,48*0x9c)
            for i in range(0,len(pool),0x9c):
                if pool[i] and pool[i+1:i+3]==bytes((0x3b,subtype)):actor=0x80092090+i;break
        b=r.read(actor,0x9c) if actor else b''
        row=dict(label=label,frame=r.request('frame')['frame'],world=s['state']['world_ticks'],play=p.hex(),life=s['life'],scene=s['scene'],bodies=[b.hex() for b in s['bodies']],boss_address=hex(actor) if actor else None,boss=b.hex())
        d['samples'].append(row);assert not s['state']['failed'] and p[0x4c]==mask
        return row,p,s,b

    def frames(n):
        start=r.request('frame')['frame'];end=time.monotonic()+12
        while r.request('frame')['frame']<start+n:
            assert time.monotonic()<end,'Native frontend stopped'
            time.sleep(.02)

    def pulse(button,seat=0):
        d['inputs'].append(dict(seat=seat,button=hex(button),mode=r.read(PLAY,4).hex()))
        r.input(button,seat);frames(6);r.input(0,seat);frames(8)

    def ticks(n,button=0,seat=1):
        r.input(button,seat);d['inputs'].append(dict(seat=seat,button=hex(button),native_ticks=n))
        start=r.state()['world_ticks'];end=time.monotonic()+12
        while r.state()['world_ticks']<start+n:
            assert time.monotonic()<end,'Native world stopped'
            time.sleep(.02)

    try:
        r.input(0,0);r.input(0,1);p=r.read(PLAY,0x1c0)
        assert p[:3]==b'\x04\x04\0' and p[0x28]==8 and p[0x10f] in (2,4)
        mask=p[0x4c];campaign=p[0x44];launch=p[0x10f];stage=9 if launch==2 else 10;subtype=stage-9
        d.update(native_stage=stage,native_subtype=subtype,mask=mask,launch_progress=launch)
        d['selector']=obs('native-mandatory-selector')[0];r.screenshot(args.out/'mandatory-selector.png');pulse(0x4000)
        end=time.monotonic()+115;selected=False;protected=False;last=None
        while True:
            row,p,s,b=obs('native-entry')
            key=(p[:3].hex(),p[12],tuple(s['scene']),b[:7].hex(),b[0x5c] if b else None)
            if key!=last:print('Dynamo entry',key,flush=True);last=key;save()
            assert time.monotonic()<end,'Native mandatory entry did not finish'
            if p[:3]==b'\x04\x04\x05' and not selected:
                count=r.read(0x800f6dfb,1)[0];assert 0<count<=11;roster=r.read(0x800f6df0,count)
                for _ in range(count):
                    cursor=r.read(PLAY+0x30,1)[0];assert cursor<count
                    if roster[cursor]==5:break
                    pulse(0x20)
                else:raise AssertionError('Native Zero unavailable')
                pulse(0x4000);selected=True
            if p[0]==10 and p[12:14]==bytes((stage,0)):
                assert not any(v in (4,5) for v in s['life'][:2])
                if not protected and s['life'][:2]==[0,0]:r.fixture(11,hp=3);r.fixture(12,hp=0);protected=True
                if r.read(0x80091c44,2)==b'\xff\x03':d['text_pages'].append(r.confirm_native_text(timeout=4,seat=max(0,s['scene'][0]-1)))
                if b and b[4]==1 and b[0x5c]>0 and b[0x5c]==p[0xa2] and struct.unpack_from('<I',b,0x54)[0] and not p[1] and not p[0x1c] and s['scene']==[0,0] and s['life'][:2]==[0,0] and all(v[4]==1 and not v[0xd0] and v[0x70]&8 for v in s['bodies']):
                    assert p[0x10f]==launch+1
                    d['fight_ready']=row;r.screenshot(args.out/'native-fight-ready.png');save();break
            time.sleep(.04)
        finisher=next(i for i,v in enumerate(s['bodies']) if v[2]==0);assert s['bodies'][finisher][0x93]==0
        floor=struct.unpack_from('<i',s['bodies'][finisher],12)[0]/65536
        camera=r.read(0x8009a1f8,0xfc);left=struct.unpack_from('<h',camera,0x1e)[0]+16;right=struct.unpack_from('<h',camera,0x1c)[0]+304
        d['writes'].append(dict(address=hex(actor+0x5c),before=b[0x5c],value=1,purpose='positiveHP setup only'))
        r.request('write_ram',addr=hex(actor+0x5c),val='0x01');d['after_hp_setup']=obs('positive-health-only')[0];save()
        end=time.monotonic()+55
        for _ in range(24):
            row,p,s,b=obs('actual-buster-finish');save()
            if not b[0x5c]&127 and b[4]==2:d['actual_zero_hp']=row;break
            assert b[4]==1 and s['life'][:2]==[0,0] and time.monotonic()<end
            if s['bodies'][finisher][5]==0x11 or not struct.unpack_from('<I',b,0x54)[0]:ticks(8,seat=finisher);continue
            x=struct.unpack_from('<i',b,8)[0]/65536;on_left=x>(left+right)/2
            target=max(left,min(right,x+(-80 if on_left else 80)))
            try:r.teleport(finisher,target,floor)
            except RuntimeError:
                row,p,s,b=obs('finish-before-position-fixture')
                if not b[0x5c]&127 and b[4]==2:d['actual_zero_hp']=row;break
                raise
            ticks(2,0x20 if on_left else 0x80,finisher);ticks(4,0x8000,finisher);ticks(8,seat=finisher)
        else:raise AssertionError('No actual native Dynamo final hit')
        r.input(0,0);r.input(0,1);r.screenshot(args.out/'native-retreat.png');end=time.monotonic()+110
        while True:
            row,p,s,b=obs('native-retreat-report')
            if p[0]==12 and p[1]:
                assert not b[0] and p[0x26]==stage and p[12:14]==b'\x0f\0' and p[0x44]==campaign
                d['native_report']=row;r.screenshot(args.out/'native-report.png');d['success']=True;break
            assert p[0] in (10,11,12) and time.monotonic()<end,'Native retreat did not finish'
            if r.read(0x80091c44,2)==b'\xff\x03':d['text_pages'].append(r.confirm_native_text(timeout=4,seat=max(0,s['scene'][0]-1)));save()
            time.sleep(.04)
    except Exception:
        d['error']=traceback.format_exc()
        try:d['failure']=obs('failure')[0];r.screenshot(args.out/'failure.png')
        except Exception:d['capture_error']=traceback.format_exc()
    finally:
        r.input(0,0);r.input(0,1);save()
    print(json.dumps({k:d.get(k) for k in ['success','native_stage','error']}),flush=True)
    return 0 if d['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())

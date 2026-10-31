"""Reach an authored final Both door and native Maverick readiness.

Checkpoint setup bypasses the preceding route. Native controller input drives
admission, opening, paired passage, intro/dialogue and HP fill. This does not
qualify a boss defeat or its distinctive attack mechanics.

Raw4/1 CP1 starts beyond the subtype3 door that requests its alternate
background. This fixture does not qualify that trigger or room artwork.
Do not seed its completed flag or treat graphics from this bypass as a mod fault.
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
from test_mmx5_coop_scene_fatal_runtime import serial

# Actual native checkpoint/placement tables, not the highest checkpoint ID.
RECIPES={4:(1,1,4528,0x34),6:(1,4,3824,0x35),7:(0,6,8016,0x33)}


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--stage',type=int,choices=RECIPES,required=True)
    args=parser.parse_args();section,checkpoint,door_x,kind=RECIPES[args.stage]
    parent=json.loads(args.session.read_text(encoding='utf-8'));exe=Path(parent['command'][0]).resolve()
    assert Path(psutil.Process(parent['pid']).exe()).resolve()==exe
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==parent['executable_sha256']
    assert parent['door_players']=='both'
    r=Runtime(parent['port']);assert not r.request('netplay_status')['active']
    args.out.mkdir(parents=True,exist_ok=False);(args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    d=dict(success=False,stage=args.stage,section=section,checkpoint=checkpoint,boss_class=hex(kind),
           candidate_sha256=parent['executable_sha256'],session=str(args.session.resolve()),samples=[],
           bypasses=['Original authored checkpoint loader bypasses preceding stage/route. HP-only hero refill; native contacts, pit/crush and control recovery remain enabled.'] +
                    (['Raw4/1 CP1 skips the earlier subtype3 alternate-background upload trigger; room artwork is not qualified.'] if args.stage==4 else []),
           limits='Actual final Both door, native intro/HP fill and control release only. No defeat/reward, full stage, attack pattern or netplay pass.')

    def save():
        (args.out/'report.json').write_bytes((json.dumps(d,indent=2)+'\n').encode())

    def obs(label):
        s=r.personal_effects();p=r.read(PLAY,0x1c0);pool=r.read(0x80092090,48*0x9c)
        bosses=[dict(address=hex(0x80092090+i),body=pool[i:i+0x9c].hex()) for i in range(0,len(pool),0x9c) if pool[i] and pool[i+1]==kind]
        row=dict(label=label,personal=serial(s),play=p.hex(),bosses=bosses,door=r.read(r.diagnostic()+0xa4,16).hex(),camera=r.read(0x8009a1f8,0xfc).hex())
        d['samples'].append(row);return row

    def wait(test,label,seconds=35):
        end=time.monotonic()+seconds
        while not test():
            assert not r.state()['failed'] and r.lifecycle()[:2]==(0,0),'Hero lost or plugin failed: '+label
            assert time.monotonic()<end,label
            time.sleep(.02)

    def phase():return r.read(r.diagnostic()+0xa4,1)[0]
    def xs():return [struct.unpack_from('<i',b,8)[0]/65536 for b in r.players()]
    def frames(n):
        start=r.request('frame')['frame'];end=time.monotonic()+12
        while r.request('frame')['frame']<start+n:
            assert time.monotonic()<end,'Native frontend stopped'
            time.sleep(.02)
    def pulse(button):
        r.input(button,0);frames(6);r.input(0,0);frames(8)

    def enter_from_selector():
        assert r.read(PLAY,3)==b'\x04\x04\0','Need gameplay or native Maverick selector'
        table=r.read(0x800f5050,8);assert set(table)==set(range(1,9));target=table.index(args.stage)
        d['native_selector_start']=r.read(PLAY,0x1c0).hex()
        for _ in range(8):
            cursor=r.read(PLAY+0x28,1)[0];assert cursor<8
            if cursor==target:break
            if cursor//4!=target//4:pulse(0x20 if target>=4 else 0x80)
            else:pulse(0x40 if cursor%4<target%4 else 0x10)
        else:raise AssertionError('Original selector did not choose target stage')
        pulse(0x4000);end=time.monotonic()+90;selected=False
        while True:
            p=r.read(PLAY,0x20)
            if p[:3]==b'\x04\x04\x05' and not selected:
                count=r.read(0x800f6dfb,1)[0];assert 0<count<=11;roster=r.read(0x800f6df0,count)
                for _ in range(count):
                    cursor=r.read(PLAY+0x30,1)[0];assert cursor<count
                    if roster[cursor]==5:break
                    pulse(0x20)
                else:raise AssertionError('Native Zero roster unavailable')
                pulse(0x4000);selected=True
            elif p[0]==10:
                s=r.personal_effects()
                if s['life'][:2]==[0,0] and not p[1] and not p[0x1c] and all(b[4]==1 for b in s['bodies']):break
                if r.read(0x80091c44,2)==b'\xff\x03':r.confirm_native_text(timeout=3,seat=max(0,s['scene'][0]-1))
            assert time.monotonic()<end,'Native selected mission did not enter gameplay'
            time.sleep(.03)
        d['native_selector_entry']=obs('native-selected-mission-before-checkpoint')

    try:
        r.input(0,0);r.input(0,1);r.resume_native_pause()
        if r.read(PLAY,1)!=b'\x0a':enter_from_selector()
        r.fixture(11,hp=3);r.fixture(12,hp=0)
        mask=r.read(PLAY+0x4c,1)[0];assert not mask & (1 << (args.stage-1))
        previous=counter(r,'mmx5.coop.enrolled');r.load_location(args.stage,section,checkpoint);wait_ready(r,previous)
        wait(lambda:all(b[4:6]==b'\x01\x02' and b[0x70]&8 and not b[0xa4] for b in r.players()),'Original arrivals did not settle')
        assert r.read(PLAY+12,2)==bytes((args.stage,section)) and r.read(PLAY+0x4c,1)==bytes([mask])
        d['native_checkpoint_entry']=obs('settled-native-checkpoint');first_x=xs()[0]
        r.screenshot(args.out/'native-checkpoint.png');save()
        before=counter(r,'mmx5.coop.both-door-admission-waits');r.input(0x20,1)
        wait(lambda:xs()[1]>=door_x-28 and counter(r,'mmx5.coop.both-door-admission-waits')>before,'P2 did not reach forward Both admission')
        r.input(0,1);d['single_arrival']=obs('actual-p2-admission-wait')
        assert phase()==0 and abs(xs()[0]-first_x)<3
        r.input(0x20,0);r.input(0x20,1);wait(lambda:phase()>0,'Both door not acquired')
        d['door_acquired']=obs('door-acquired');r.input(0,0);r.input(0,1)
        raw=r.read(r.diagnostic()+0xa4,16);right=struct.unpack_from('<h',raw,14)[0]
        actor=struct.unpack_from('<I',raw,4)[0];door=r.read(actor,0xa4)
        assert struct.unpack_from('<h',door,10)[0]==door_x,'Wrong authored door acquired'
        wait(lambda:phase()==2,'Original opening did not release controls');d['door_open']=obs('door-open')
        r.input(0x20,0);r.input(0x20,1)
        wait(lambda:phase()==0 and min(xs())>right+12,'Original paired passage did not close',seconds=45)
        r.input(0,0);r.input(0,1);d['door_closed']=obs('door-closed');save()
        end=time.monotonic()+75;last=None
        while True:
            row=obs('native-boss-admission');p=bytes.fromhex(row['play']);s=row['personal']
            bosses=[bytes.fromhex(b['body']) for b in row['bosses']]
            key=(p[:2].hex(),tuple(s['scene']),[(b[4],b[5],b[0x5c],b[0x85],b[0x86],b[0x8c]) for b in bosses])
            if key!=last:print('Stage',args.stage,'native boss',key,flush=True);last=key;save()
            assert time.monotonic()<end and s['life'][:2]==[0,0] and not s['state']['failed'],'Native intro did not release'
            if r.read(0x80091c44,2)==b'\xff\x03':r.confirm_native_text(timeout=3,seat=max(0,s['scene'][0]-1))
            if bosses:
                b=bosses[0]
                # The first live phase can advance before a TCP sample. Use
                # native fill/release/vulnerability rather than requiring a
                # single transient action number to remain observable.
                if args.stage==4:gate=b[0x85]==1 and b[0x5c]==b[0x86]
                elif args.stage==6:gate=b[5]>=3 and b[0x5c]==p[0xa2] and struct.unpack_from('<I',b,0x54)[0]!=0
                else:gate=b[0x8c]==1 and b[0x5c]==p[0xa2]
                if gate and b[4]==1 and b[0x5c]&127 and not p[1] and not p[0x1c] and s['scene']==[0,0] and all(v[4]==1 and v[0x70]&8 for v in r.players()):
                    d['fight_ready']=row;r.screenshot(args.out/'native-fight-ready.png');break
            time.sleep(.04)
        assert r.read(PLAY+0x4c,1)==bytes([mask]);d['success']=True
    except Exception:
        d['error']=traceback.format_exc()
        try:d['failure']=obs('failure');r.screenshot(args.out/'failure.png')
        except Exception:d['capture_error']=traceback.format_exc()
    finally:
        r.input(0,0);r.input(0,1);save()
    print(json.dumps({k:d.get(k) for k in ['success','stage','error']}),flush=True)
    return 0 if d['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())

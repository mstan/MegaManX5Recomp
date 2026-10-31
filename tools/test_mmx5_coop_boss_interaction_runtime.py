"""Bounded native boss recovery or attack-child observation before a final hit.

Stage6 exercises real nonlethal P2 buster hits and original actor94 recovery.
Stage7 observes its authored class23/sub1 attack effect and native motion/art.
No boss state, health, vulnerability, attack phase or success flag is written.
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
from test_mmx5_coop_scene_fatal_runtime import serial


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--stage',type=int,choices=(6,7),required=True)
    args=parser.parse_args();parent=json.loads(args.session.read_text(encoding='utf-8'))
    exe=Path(parent['command'][0]).resolve();assert Path(psutil.Process(parent['pid']).exe()).resolve()==exe
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==parent['executable_sha256']
    r=Runtime(parent['port']);assert not r.request('netplay_status')['active']
    args.out.mkdir(parents=True,exist_ok=False);(args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    d=dict(success=False,stage=args.stage,candidate_sha256=parent['executable_sha256'],
           session=str(args.session.resolve()),samples=[],inputs=[],
           setup='HP-only hero refill. Stage6 positions only X/P2 within the native arena for ordinary buster shots. No boss writes.',
           limits='One distinct original recovery bank or native attack-effect family only; no optional scaling, weakness, full attack pattern or netplay pass.')

    def save():
        (args.out/'report.json').write_bytes((json.dumps(d,indent=2)+'\n').encode())

    def obs(label):
        s=r.personal_effects();p=r.read(PLAY,0x1c0);boss=r.read(actor,0x9c)
        row=dict(label=label,personal=serial(s),play=p.hex(),boss=boss.hex())
        d['samples'].append(row)
        assert p[0]==10 and p[12]==args.stage and not s['state']['failed'] and s['life'][:2]==[0,0]
        assert boss[1]==(0x35 if args.stage==6 else 0x33) and boss[4]==1 and boss[0x5c]&127
        return row,s,boss

    def ticks(n):
        start=r.state()['world_ticks'];end=time.monotonic()+12
        while r.state()['world_ticks']<start+n:
            assert time.monotonic()<end,'Native world stopped'
            time.sleep(.015)

    def shoot_until_hp_below(hp,label):
        end=time.monotonic()+25
        for attempt in range(24):
            row,s,b=obs(label)
            if b[0x5c]<hp:return row
            assert time.monotonic()<end,label+' timed out'
            if s['bodies'][1][5]==0x11:ticks(12);continue
            if not struct.unpack_from('<I',b,0x54)[0]:ticks(6);continue
            x=struct.unpack_from('<i',b,8)[0]/65536;left_side=x>(left+right)/2
            target=max(left,min(right,x+(-120 if left_side else 120)))
            # Original stage6 boss flies above the floor and turns its target
            # box off during parts of flight. Position-only setup aligns the
            # native muzzle with a currently vulnerable target; gravity,
            # contacts and projectile movement remain original.
            aim_y=min(floor,struct.unpack_from('<i',b,12)[0]/65536+16)
            r.teleport(1,target,aim_y);r.input(0x20 if left_side else 0x80,1);ticks(2)
            r.input(0x8000,1);ticks(4);r.input(0,1);ticks(8)
            d['inputs'].append(dict(attempt=attempt,target_x=target,button='native normal buster'))
        row,s,b=obs(label+'-last');assert b[0x5c]<hp,'No accepted native nonlethal P2 hit'
        return row

    try:
        r.input(0,0);r.input(0,1);r.resume_native_pause();r.fixture(11,hp=3);r.fixture(12,hp=0)
        actor=struct.unpack('<I',r.read(PLAY+0x20,4))[0]
        assert 0x80092090<=actor<0x80092090+48*0x9c and not (actor-0x80092090)%0x9c
        row,s,b=obs('native-fight-start');assert s['scene']==[0,0]
        if args.stage==6:
            assert s['bodies'][1][2]==0 and s['bodies'][1][0x93]==0
            floor=struct.unpack_from('<i',s['bodies'][1],12)[0]/65536
            camera=r.read(0x8009a1f8,0xfc);left=struct.unpack_from('<h',camera,0x1e)[0]+16;right=struct.unpack_from('<h',camera,0x1c)[0]+304
            hp=b[0x5c];assert hp>8 and b[0x94]==0
            r.request('wtrace_range',lo=hex(actor+0x58),hi=hex(actor+0x98));r.request('wtrace_clear')
            d['first_accepted_hit']=shoot_until_hp_below(hp,'first-real-hit');save()
            end=time.monotonic()+12
            while True:
                row,s,b=obs('native-recovery')
                if not b[0x94] and struct.unpack_from('<I',b,0x58)[0]==0x80075de0:break
                assert time.monotonic()<end,'Original recovery did not expire'
                time.sleep(.015)
            d['recovery_trace']=r.request('wtrace_dump',count=2048,addr_lo=hex(actor+0x58),addr_hi=hex(actor+0x98),pc_lo='0x800F2808',pc_hi='0x800F28C8')
            recovery=[e for e in d['recovery_trace']['entries'] if int(e['addr'],16)==(actor+0x94)&0x1fffffff]
            assert recovery and any(int(e['new'],16)==30 for e in recovery) and int(recovery[-1]['new'],16)==0
            assert any(int(e['new'],16)==0x80075de0 for e in d['recovery_trace']['entries'])
            d['second_accepted_hit']=shoot_until_hp_below(b[0x5c],'fresh-follow-up');r.screenshot(args.out/'actual-p2-follow-up.png')
        else:
            r.request('wtrace_range',lo='0x800D1F40',hi='0x800D4F40');r.request('wtrace_clear')
            end=time.monotonic()+30;found=None
            while found is None:
                row,s,b=obs('native-attack-emission');pool=r.read(0x800d1f40,0x3000)
                for i in range(0,len(pool),0x60):
                    if pool[i] and pool[i+1:i+3]==b'\x23\x01':found=(0x800d1f40+i,pool[i:i+0x60]);break
                assert time.monotonic()<end,'Native phase9 attack effect not observed'
                time.sleep(.02)
            address,body=found;d['native_child']=dict(address=hex(address),body=body.hex(),sample=row)
            r.screenshot(args.out/'native-attack-child.png');ticks(8)
            later=r.read(address,0x60);d['child_later']=later.hex()
            d['child_init_trace']=r.request('wtrace_dump',count=128,addr_lo=hex(address),addr_hi=hex(address+0x60),pc_lo='0x800F6120',pc_hi='0x800F61A8')
            assert d['child_init_trace']['entries'],'No actual native attack producer evidence'
            assert body!=later,'Native attack effect did not advance'
            r.screenshot(args.out/'native-attack-child-later.png')
        d['success']=True
    except Exception:
        d['error']=traceback.format_exc()
        try:d['failure']=obs('failure')[0];r.screenshot(args.out/'failure.png')
        except Exception:d['capture_error']=traceback.format_exc()
    finally:
        r.input(0,0);r.input(0,1);r.request('wtrace_disarm_all');save()
    print(json.dumps({k:d.get(k) for k in ['success','stage','error']}),flush=True)
    return 0 if d['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())

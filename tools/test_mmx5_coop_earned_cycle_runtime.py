"""Cycle actually earned X weapons and retain native deferred-upload evidence.

Attach after a successful native-card entry receipt. No RAM, card, inventory,
progression or position writes; existing function/write rings observe the
original effect upload, retirement and any unexpected frontend transition.
"""
import argparse
import hashlib
import json
from pathlib import Path
import time
import traceback

import psutil
from coop_debug import Runtime, PLAY
from test_mmx5_coop_scene_fatal_runtime import serial


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--entry',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    parent=json.loads(args.session.read_text(encoding='utf-8'))
    entry=json.loads(args.entry.read_text(encoding='utf-8'))
    exe=Path(parent['command'][0]).resolve()
    assert Path(psutil.Process(parent['pid']).exe()).resolve()==exe
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==parent['executable_sha256']
    assert entry['success'] and Path(entry['session']).resolve()==args.session.resolve()
    assert entry['executable_sha256']==parent['executable_sha256']
    r=Runtime(parent['port']);assert not r.request('netplay_status')['active']
    args.out.mkdir(parents=True,exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    d=dict(success=False,candidate_sha256=parent['executable_sha256'],
           session=str(args.session.resolve()),native_entry=str(args.entry.resolve()),
           inputs=[],samples=[],cycles=[],limits='Native earned selection/upload/retirement and guest menu only. No projectile mechanics, combat or broad save/story parity claim.')

    def save():
        (args.out/'report.json').write_bytes((json.dumps(d,indent=2)+'\n').encode())

    def observe(label):
        s=r.personal_effects();p=r.read(PLAY,0x1c0)
        v=dict(label=label,personal=serial(s),play=p.hex(),
               scratch=r.read(0x1f800000,0x30).hex(),frame=r.request('frame')['frame'])
        d['samples'].append(v)
        assert not s['state']['failed'] and s['life'][:2]==[0,0]
        assert p[0]==10 and p[0x4c]==mask,'Unexpected native title/progression transition'
        return s,p

    def frames(n):
        start=r.request('frame')['frame'];end=time.monotonic()+12
        while r.request('frame')['frame']<start+n:
            assert time.monotonic()<end,'Native frames stopped'
            time.sleep(.015)

    def traces():
        return dict(upload=r.request('fn_entry_dump',count=1024,addr_lo='0x80017348',addr_hi='0x8001734C'),
                    decompress=r.request('fn_entry_dump',count=1024,addr_lo='0x800182D8',addr_hi='0x800182DC'),
                    fx_writes=r.request('wtrace_dump',count=2048,addr_lo='0x80094398',addr_hi='0x80096D98',pc_lo='0x80050D04',pc_hi='0x80050F60'),
                    play_writes=r.request('wtrace_dump',count=128,addr_lo=hex(PLAY),addr_hi=hex(PLAY+4)))

    try:
        r.input(0,0);r.input(0,1)
        mask=bytes.fromhex(entry['native_entry']['play'])[0x4c]
        s,p=observe('native-entry-before-stability')
        x_seat=next(i for i,b in enumerate(s['bodies']) if b[2]==0)
        frames(120);s,p=observe('released-stable-native-entry')
        assert not p[1] and not p[0x1c] and r.read(0x8009a424,1)==b'\0'
        assert all(b[4:6]==b'\x01\x02' and b[0xc9]==mask for b in s['bodies'])
        order=list(r.read(0x80074a28,9));masks=list(r.read(0x80074a1c,9))
        assert order==[0,1,5,6,3,8,7,2,4] and masks==[0,1,16,32,4,128,64,2,8]
        expected=[w for w,m in zip(order,masks) if m & mask]+[0]
        d.update(x_seat=x_seat,mask=mask,expected=[0]+expected)
        assert s['bodies'][x_seat][0x93]==0
        d['actual']=[0];save()
        r.request('fn_filter',lo='0x80017348',hi='0x800182DC')
        r.request('wtrace_range',lo='0x80094398',hi='0x80096D98')
        r.request('wtrace_add',lo=hex(PLAY),hi=hex(PLAY+4))
        for weapon in expected:
            r.request('fn_clear');r.request('wtrace_clear')
            case=dict(expected=weapon);d['cycles'].append(case)
            r.input(0x800,x_seat);d['inputs'].append(dict(seat=x_seat,button='R1'))
            frames(6);r.input(0,x_seat);frames(8)
            case['trace']=traces();save()
            if weapon==5:
                pointers=[e for e in case['trace']['fx_writes']['entries'] if e['pc']=='0x80050EE8']
                assert len(pointers)==1 and int(pointers[0]['v1'],16)==0x29fc0,'Weapon5 did not use the original X graphics directory'
                base=int(pointers[0]['new'],16)-0x29fc0
                header=r.read(base,0x1c)
                assert header.hex()=='3c00000098890200988902005c8e02003c9602003c960200c09f0200'
                case['original_x_graphics']=dict(base=hex(base),header=header.hex(),offset='0x29FC0')
            s,p=observe('native-cycle-'+str(weapon))
            case['actual']=s['bodies'][x_seat][0x93];d['actual'].append(case['actual'])
            assert case['actual']==weapon,'Earned native cycle differs'
            assert not any(r.read(0x80094398+i*0x70,3)==b'\x01\x1b\x03' for i in range(96)), 'Native weapon5 upload did not retire'
            save()
        r.request('fn_disable');r.request('wtrace_disarm_all')
        r.input(8,1);frames(6);r.input(0,1)
        end=time.monotonic()+10
        while r.read(PLAY+1,1)!=b'\x02' or r.read(0x800d4f44,1)!=b'\x01':
            assert time.monotonic()<end,'Native guest menu did not open'
            time.sleep(.025)
        assert r.read(r.diagnostic()+0x48,1)==b'\x02'
        d['guest_owned_menu']=serial(r.personal_effects());r.screenshot(args.out/'earned-cycle-guest-menu.png')
        r.resume_native_pause();d['success']=True
    except Exception:
        d['error']=traceback.format_exc()
        try:d['failure']=serial(r.personal_effects());d['failure_play']=r.read(PLAY,0x1c0).hex();r.screenshot(args.out/'failure.png')
        except Exception:d['capture_error']=traceback.format_exc()
    finally:
        r.input(0,0);r.input(0,1);r.request('fn_disable');r.request('wtrace_disarm_all');save()
    print(json.dumps({k:d.get(k) for k in ['success','actual','expected','error']}),flush=True)
    return 0 if d['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())

"""Original story encounter must wait for actual Both-player door clearance.

The original raw18 checkpoint2 loader and HP refill bypass prior traversal.
All door motion, body clearance, scene admission, text and release are native.
No door, NPC, camera, pose or progression result is seeded by this observer.
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


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    session=json.loads(args.session.read_text(encoding='utf-8-sig'))
    exe=Path(session['command'][0]).resolve()
    assert Path(psutil.Process(session['pid']).exe()).resolve()==exe
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==session['executable_sha256']
    assert session['door_players']=='both' and session['character'] in ('x','zero')
    npc_kind=0x40 if session['character']=='zero' else 0x41
    r=Runtime(session['port']);assert not r.request('netplay_status')['active']
    out=args.out;out.mkdir(parents=True,exist_ok=False)
    (out/'harness.py').write_bytes(Path(__file__).read_bytes())
    d=dict(success=False,human_validation=False,campaign=session['character'],npc_kind=npc_kind,
           candidate_sha256=session['executable_sha256'],session=str(args.session.resolve()),
           harness_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),samples=[],pages=[],
           bypasses=['Original raw18/0 checkpoint2 loader bypasses campaign and preceding route.',
                     'HP-only refill during setup; contact/hurt and scene state stay native.'],
           limits='One requested native story encounter and Both-player crossing. Opponent art, boss defeat, other story branches and network remain separate.')

    def save():
        (out/'report.json').write_bytes((json.dumps(d,indent=2)+'\n').encode('utf-8'))

    def observe(label):
        s=r.personal_effects();p=r.read(PLAY,0x1c0);pool=r.read(0x80092090,48*0x9c)
        npcs=[dict(address=hex(0x80092090+i),body=pool[i:i+0x9c].hex()) for i in range(0,len(pool),0x9c)
              if pool[i] and pool[i+1]>=0x30]
        diagnostic=r.read(r.diagnostic(),0xE88);door=diagnostic[0xa4:0xb4]
        row=dict(label=label,personal=serial(s),play=p.hex(),npcs=npcs,door=door.hex(),
                 camera=r.read(0x8009a1f8,0xfc).hex(),views=diagnostic[0x710:0x908].hex())
        actor=struct.unpack_from('<I',door,4)[0]
        if actor:
            body=r.read(actor,0xa4);row['door_body']=body.hex()
            box=struct.unpack_from('<I',body,0x68)[0]
            if 0x80010000<=box<=0x801ffffc:row['door_shape']=r.read(box,4).hex()
        d['samples'].append(row)
        return row,s,p

    def until(predicate,label,seconds):
        deadline=time.monotonic()+seconds;last=None
        while True:
            row,s,p=observe(label)
            signature=(p[:4].hex(),tuple(s['scene']),tuple(s['life'][:2]),row['door'][:2],
                       tuple(n['body'][:16] for n in row['npcs']))
            if signature!=last:
                print(label,signature,flush=True);last=signature;save()
            if predicate(row,s,p):return row,s,p
            assert time.monotonic()<deadline,'Timed out: '+label
            assert not s['state']['failed'] and s['life'][:2]==[0,0],'Lost living hero/plugin'
            time.sleep(.05)

    try:
        r.release();r.resume_native_pause();r.fixture(11,hp=3);r.fixture(12,hp=0)
        enrolled=counter(r,'mmx5.coop.enrolled');r.load_location(18,0,2);wait_ready(r,enrolled)
        until(lambda row,s,p:not p[1] and not p[0x1c] and s['scene']==[0,0] and
              all(b[4:6]==b'\x01\x02' and ((b[0x70]|b[0x71])&8)for b in s['bodies']),
              'native-checkpoint-ready',30)
        d['checkpoint']=observe('checkpoint')[0];r.screenshot(out/'checkpoint.png')
        waits=counter(r,'mmx5.coop.both-door-admission-waits')
        d['crossings_before']=counter(r,'mmx5.coop.both-door-crossings')
        r.request('fn_clear');r.request('fn_filter',lo='0x8003D29C',hi='0x8003D2A0')
        r.input(0x20,1)
        row,s,p=until(lambda row,s,p:bool(s['scene'][0]) or
                      counter(r,'mmx5.coop.both-door-admission-waits')>waits,'p2-first-admission',35)
        r.release()
        assert not s['scene'][0],'One hero bypassed Both admission'
        r.input(0x20,0);r.input(0x20,1)
        row,s,p=until(lambda row,s,p:bool(s['scene'][0]) or
                      any(bytes.fromhex(n['body'])[1]==npc_kind and bytes.fromhex(n['body'])[4]for n in row['npcs']),
                      'both-native-crossing',90)
        r.release();d['admission']=row;r.screenshot(out/'scene-admission.png')
        d['native_stand_calls']=r.request('fn_entry_dump',count=512);r.request('fn_disable')
        d['crossings_after']=counter(r,'mmx5.coop.both-door-crossings')
        assert bytes.fromhex(row['door'])[0] in (0,4),'Native story starts before Both-player clearance'
        assert d['crossings_after']==d['crossings_before']+1,'Native Both crossing did not commit exactly once'
        aperture=next(sample for sample in reversed(d['samples']) if 'door_shape' in sample)
        door_body=bytes.fromhex(aperture['door_body']);shape=bytes.fromhex(aperture['door_shape'])
        right=struct.unpack_from('<i',door_body,8)[0]+(struct.unpack_from('<b',shape)[0]+shape[2])*65536
        clearance=[]
        for body in s['bodies']:
            assert body[3] and body[4]==1,'Story admission lost a living body'
            box=struct.unpack_from('<I',body,0x68)[0]
            assert 0x80010000<=box<=0x801ffffc,'No native body collision shape'
            shape=r.read(box,4)
            left=struct.unpack_from('<i',body,8)[0]+(struct.unpack_from('<b',shape)[0]-shape[2])*65536
            assert left>right,'Native body still overlaps the aperture at story admission'
            clearance.append((left-right)/65536)
        d['actual_clearance_pixels']=clearance
        deadline=time.monotonic()+180;seen=False
        while True:
            row,s,p=observe('native-text-and-release')
            seen|=any(bytes.fromhex(n['body'])[1]==npc_kind for n in row['npcs'])
            assert not s['state']['failed'] and s['life'][:2]==[0,0]
            if r.read(0x80091c44,2)==b'\xff\x03':
                if not d['pages']:r.screenshot(out/'dialogue.png')
                d['pages'].append(r.confirm_native_text(timeout=4,seat=max(0,s['scene'][0]-1)));save()
            elif seen and not p[1] and not p[0x1c] and not s['scene'][0] and all(
                    b[4]==1 and not b[0x7a] for b in s['bodies']) and any(
                    bytes.fromhex(n['body'])[1]==npc_kind and (npc_kind==0x41 or bytes.fromhex(n['body'])[0x87]==1) and
                    bytes.fromhex(n['body'])[0x5c]==p[0xa2]>0 for n in row['npcs']):
                d['released']=row;break
            assert time.monotonic()<deadline,'Native story did not release both heroes'
            time.sleep(.05)
        motions=[]
        for seat in (0,1):
            r.release();row,s,p=until(lambda row,s,p:s['bodies'][seat][4:6]==b'\x01\x02','native-idle',20)
            x=struct.unpack_from('<i',s['bodies'][seat],8)[0]
            r.input(0x20,seat);start=r.state()['world_ticks']
            until(lambda row,s,p:s['state']['world_ticks']>=start+12,'native-control',10)
            r.release();body=r.players()[seat];motions.append((struct.unpack_from('<i',body,8)[0]-x)/65536)
        assert all(delta>1 for delta in motions),'Story failed to restore both native controls'
        d['motions_right']=motions;r.screenshot(out/'native-controls-restored.png')
        d['render_stats']=r.request('render_pass_stats')
        assert not any(d['render_stats'][k] for k in ('verify_mismatch','watchdog','vram_leaks','device_reads'))
        d['success']=True
    except Exception:
        d['error']=traceback.format_exc();print(d['error'],flush=True)
        try:observe('failure');r.screenshot(out/'failure.png')
        except Exception:pass
    finally:
        r.release()
        try:r.request('fn_disable');r.tap(8,.1)
        except Exception:pass
        save()
    print(json.dumps({k:d.get(k)for k in ('success','error','motions_right','crossings_before','crossings_after')}),flush=True)
    return 0 if d['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())

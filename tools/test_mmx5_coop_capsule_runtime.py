"""One original capsule: P2 grant/return, then committed-grant owner fatal.

Native stage8 CP2 and bounded original placement rescan bypass access/traversal.
Position/history uses the actual initialized capsule collider. Never write a
scene, capsule phase, program, completion or release flag. The second case
restores the genuine pre-grant host snapshot and earns its grant again.
Native card persistence and completed-armor equipment remain separate checks.
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
from test_mmx5_coop_refight_runtime import require, serial
from test_mmx5_coop_snapshot_runtime import state_operation


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--original-dat',type=Path,required=True)
    args=parser.parse_args()
    parent=json.loads(args.session.read_text(encoding='utf-8'))
    exe=Path(parent['command'][0]).resolve()
    require(Path(psutil.Process(parent['pid']).exe()).resolve()==exe,'Not the owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest()==parent['executable_sha256'],'Candidate changed')
    r=Runtime(parent['port']);require(not r.request('netplay_status')['active'],'Offline only')
    args.out.mkdir(parents=True,exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    d=dict(success=False,human_validation=False,session=str(args.session.resolve()),
           executable_sha256=parent['executable_sha256'],cases=[],operations=[],samples=[],
           harness_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
           bypasses=['Original stage8/0/CP2 loader','HP-only setup protection, disabled before seeding',
                     'Original2B070 bounded rescan bypasses viewport-edge admission',
                     'Collider-derived P2 position/history; P1 stays at native checkpoint floor',
                     'Native fatalHP only after actual program grant in the second case'],
           limits='Regular capsule5 only. No route, ordinary viewport entry, native card persistence, completed armor or current netplay pass.')
    original=args.original_dat.read_bytes()
    d['original_dat_sha256']=hashlib.sha256(original).hexdigest()
    require(d['original_dat_sha256']=='5e0d1c49473ec502a7e517677ad0abdb6e9795e5dd6931ac8aa89fd4af0fa5bf',
            'Expected immutable original DAT')
    base=struct.unpack_from('<I',original,88*8)[0]*2048
    member=base+2048
    for index in range(15):member+=(struct.unpack_from('<I',original,base+12+index*8)[0]+2047)&~2047
    kind,length=struct.unpack_from('<II',original,base+8+15*8)
    require(kind==0x15 and length==69472,'Expected original X message member')
    original_messages=original[member:member+length]
    script_begin,script_end=struct.unpack_from('<II',original_messages,0x3F*4)
    original_script=original_messages[script_begin:script_end]
    require(hashlib.sha256(original_script).hexdigest()=='37ced8314ea217bfa55df30db0deeecac01403d10d9f6915ae4bb92cd0672ac5',
            'Expected complete original X capsule script')
    del original

    def save():
        (args.out/'report.json').write_bytes((json.dumps(d,indent=2)+'\n').encode())

    def capsules():
        raw=r.read(0x800C6A10,64*0xA4)
        return [dict(address=hex(0x800C6A10+i),body=raw[i:i+0xA4].hex())
                for i in range(0,len(raw),0xA4) if raw[i] and raw[i+1:i+3]==b'\x1A\x05']

    def obs(label):
        s=r.personal_effects();p=r.read(PLAY,0x1C0);caps=capsules()
        row=dict(label=label,personal=serial(s),play=p.hex(),capsules=caps,
                 text=r.read(0x80091C38,0x18).hex())
        d['samples'].append(row);save();return s,p,caps,row

    def wait(test,message,seconds=30):
        end=time.monotonic()+seconds
        while True:
            s=r.personal_effects()
            if test(s):return s
            require(time.monotonic()<end,message)
            require(not s['state']['failed'],'Co-op reported failure')
            time.sleep(.015)

    def move(seat,button,label):
        before=r.personal_effects();x=struct.unpack_from('<i',before['bodies'][seat],8)[0]
        world=before['state']['world_ticks'];r.input(button,seat)
        try:wait(lambda s:s['state']['world_ticks']>=world+10,'Native world stopped',10)
        finally:r.input(0,seat)
        s,p,c,row=obs(label)
        delta=(struct.unpack_from('<i',s['bodies'][seat],8)[0]-x)/65536
        require(delta>1 if button==0x20 else delta<-1,'Recovered hero did not move under native input')
        return dict(seat=seat,delta_x=delta,sample=row)

    def verify_recipient_text():
        cursor=struct.unpack('<I',r.read(0x80091C38,4))[0]
        fragment=r.read(cursor,24)
        position=original_script.find(fragment)
        require(position>=0 and original_script.find(fragment,position+1)<0,'Native cursor is not uniquely inside X capsule text')
        root=cursor-position-script_begin
        require(r.read(root,len(original_messages))==original_messages,'Private message bank differs from original X')
        canonical=struct.unpack('<I',r.read(0x1F800048,4))[0]
        require(canonical!=root,'Capsule replaced the shared campaign message root')
        return dict(cursor=hex(cursor),private_root=hex(root),canonical_root=hex(canonical),
                    member_bytes=len(original_messages),script_bytes=len(original_script),
                    script_sha256=hashlib.sha256(original_script).hexdigest())

    def run_case(fatal):
        case=dict(fatal_after_grant=fatal,status='unverified',pages=[],milestones=[],jump_attempts=0)
        d['cases'].append(case);save()
        s,p,c,row=obs('fatal-case-pregrant' if fatal else 'normal-case-pregrant')
        require(s['life'][:2]==[0,0] and not p[0xA1]&0x40 and len(c)==1,'Need genuine ungranted capsule and both heroes')
        before=s['bodies'];retries=p[0x45]
        handoffs=counter(r,'mmx5.coop.dead-scene-owner-handoffs')
        proximity=counter(r,'mmx5.coop.p2-capsule-proximity')
        contact=counter(r,'mmx5.coop.p2-capsule-contact')
        if fatal:
            r.request('fn_clear');r.request('fn_filter',lo='0x8002D27C',hi='0x8002D280')
            d['allocation_trace_armed']=True
        case['death_pool_samples']=[];last_pool_world=-100;particles_captured=False
        r.teleport(1,d['setup']['cap_x'],d['setup']['p2_center_y'])
        deadline=time.monotonic()+100
        acquired=granted=fatal_submitted=fatal_seen=False
        last=None;jump_until=next_jump=0
        case_name='fatal' if fatal else 'normal'
        while time.monotonic()<deadline:
            s=r.personal_effects();p=r.read(PLAY,0x1C0);caps=capsules()
            require(not s['state']['failed'] and s['life'][0]==0,'Capsule lost the living P1 partner')
            require(len(caps)==1,'Original capsule unexpectedly vanished')
            cap=bytes.fromhex(caps[0]['body']);b=s['bodies'][1];world=s['state']['world_ticks']
            key=(*s['life'],*s['scene'],cap[5],cap[6],b[4],b[5],b[6],b[0xF5],p[0xA1])
            if key!=last:
                row=obs(case_name+'-native-state')[3];case['milestones'].append(row);last=key
            acquired|=s['scene'][0]==2
            if not acquired:
                require(s['scene'][0]!=1,'P1 acquired the capsule despite being outside proximity')
            if r.read(0x80091C44,2)==b'\xFF\x03':
                require(acquired,'Text appeared before actual P2 capsule admission')
                require(len(case['pages'])<12,'Unexpected repeated capsule text')
                if not case.get('recipient_text'):
                    case['recipient_text']=verify_recipient_text()
                    if not fatal:
                        d['operations'].append(state_operation(r,'save',6));save()
                        case['snapshot_advance']=r.confirm_native_text(timeout=15,seat=1)
                        wait(lambda _:r.read(0x80091C44,2)==b'\xFF\x03','Next native text page did not become ready')
                        require(verify_recipient_text()['cursor']!=case['recipient_text']['cursor'],'Native text did not advance')
                        r.release();d['operations'].append(state_operation(r,'load',6))
                        case['restored_recipient_text']=verify_recipient_text()
                        require(case['restored_recipient_text']==case['recipient_text'],'Mid-text snapshot did not restore the native cursor/bank')
                        save()
                image=args.out/(case_name+'-light-text-'+str(len(case['pages']))+'.png')
                r.screenshot(image)
                case['pages'].append(dict(image=image.name,input=r.confirm_native_text(timeout=15,seat=1)))
                save()
            if cap[5]==6 and b[4]==1 and not any(b[o] for o in (0xD0,0xD4,0x7A)):
                cx=struct.unpack_from('<i',cap,8)[0];bx=struct.unpack_from('<i',b,8)[0]
                direction=0x20 if bx<cx-2*65536 else 0x80 if bx>cx+2*65536 else 0
                if world>=next_jump and case['jump_attempts']<3:
                    case['jump_attempts']+=1;jump_until=world+8;next_jump=world+45
                r.input(direction|(0x4000 if world<jump_until else 0),1)
            else:r.input(0,1)
            if p[0xA1]&0x40 and not granted:
                require(acquired and cap[5]==11,'Program changed without the original capsule grant boundary')
                case['actual_grant']=obs(case_name+'-actual-program-grant')[3];granted=True
                if fatal:
                    require(s['scene'][0]==2 and b[4:6]==b'\x01\x18' and b[0xF5]==4 and b[6] in (0,1),
                            'Missed the actual committed-grant return window')
                    case['fatal_submission']=serial(s)
                    r.fixture(1,seat=1);fatal_submitted=True
            if fatal_submitted:fatal_seen|=s['life'][1] in (4,5) and bool(s['scene'][0])
            if fatal_submitted and world>=last_pool_world+8:
                raw=r.read(0x800D1F40,0x3000)
                effects=[dict(slot=i//0x60,body=raw[i:i+0x60].hex()) for i in range(0,len(raw),0x60)
                         if raw[i] and raw[i+1]==0x11]
                case['death_pool_samples'].append(dict(world=world,life=s['life'],effects=effects,
                    note='Guest pool is a separate asynchronous read after the coherent body observation'))
                last_pool_world=world
                if effects and not particles_captured:
                    r.screenshot(args.out/'actual-return-owner-death-particles-composed.png')
                    particles_captured=True
            expected=[0,5] if fatal else [0,0]
            drivers=(0,) if fatal else (0,1)
            ready=all(s['bodies'][seat][4:6]==b'\x01\x02' and
                      not any(s['bodies'][seat][o] for o in (0xD0,0xD4,0x7A)) and
                      (s['bodies'][seat][0x70]|s['bodies'][seat][0x71])&8 for seat in drivers)
            if granted and s['life'][:2]==expected and not s['scene'][0] and not p[0x1C] and ready:
                case['native_return']=obs(case_name+'-native-return-controls')[3]
                require(s['bodies'][0][0x5C]==before[0][0x5C] and
                        s['bodies'][0][0xA8:0xC8]==before[0][0xA8:0xC8], 'Capsule changed passenger resources')
                require(p[0x45]==retries,'Capsule consumed a team retry')
                require(not fatal or fatal_seen,'Native fatal did not occur during the return scene')
                case['dead_owner_handoffs']=counter(r,'mmx5.coop.dead-scene-owner-handoffs')-handoffs
                require(case['dead_owner_handoffs']==int(fatal),'Wrong number of capsule owner recoveries')
                case['proximity_handoffs']=counter(r,'mmx5.coop.p2-capsule-proximity')-proximity
                case['contact_handoffs']=counter(r,'mmx5.coop.p2-capsule-contact')-contact
                require(case['proximity_handoffs']==1 and case['contact_handoffs']==1,'Native capsule admission was not once per gate')
                case['motion']=[move(seat,0x20 if seat==0 else 0x80,case_name+'-native-motion') for seat in drivers]
                r.screenshot(args.out/(case_name+'-native-return-composed.png'))
                case['status']='passed';save();return
            time.sleep(.015)
        raise TimeoutError('Original capsule grant/return did not finish')

    try:
        r.resume_native_pause();r.release();r.fixture(11,hp=3);r.fixture(12,hp=0)
        previous=counter(r,'mmx5.coop.enrolled');r.load_location(8,0,2);wait_ready(r,previous)
        wait(lambda s:all(b[4:6]==b'\x01\x02' for b in s['bodies']),'Native checkpoint idle unavailable')
        r.rescan_placements(2432,2448,1576,1592)
        wait(lambda s:len(capsules())==1 and bytes.fromhex(capsules()[0]['body'])[4]==1,'Original rescan did not initialize capsule')
        s,p,c,row=obs('native-capsule-loaded-before-setup')
        require([b[2] for b in s['bodies']]==[1,0] and not p[0xA1]&0x40,'Expected ungranted program and Zero/P1-X/P2')
        cap=bytes.fromhex(c[0]['body']);cb=r.read(struct.unpack_from('<I',cap,0x68)[0],4)
        hb=r.read(struct.unpack_from('<I',s['bodies'][1],0x68)[0],4)
        capx,capy=(v/65536 for v in struct.unpack_from('<ii',cap,8))
        d['setup']=dict(cap_x=capx,cap_y=capy,cap_box=cb.hex(),hero_box=hb.hex(),
                        p2_center_y=capy+struct.unpack_from('<b',cb,1)[0]-cb[3]-struct.unpack_from('<b',hb,1)[0]-hb[3]-1)
        r.fixture(11,hp=0);r.fixture(2,seat=0,hp=13,ammo=121);r.fixture(2,seat=1,hp=17,ammo=73)
        d['operations'].append(state_operation(r,'save',5));save()
        run_case(False)
        r.release();d['operations'].append(state_operation(r,'load',5));save()
        run_case(True)
        d['render_stats']=r.request('render_pass_stats')
        require(all(not d['render_stats'].get(k,0) for k in ('verify_mismatch','watchdog','vram_leaks','device_reads')),'Private rendering fault')
        d['success']=True
    except Exception:
        d['error']=traceback.format_exc();print(d['error'],flush=True)
        try:obs('failure');r.screenshot(args.out/'failure-composed.png')
        except Exception:d['failure_capture_error']=traceback.format_exc()
    finally:
        try:
            r.release()
            if d.get('allocation_trace_armed'):
                for kind in ('entry','exit'):
                    trace=r.request('fn_'+kind+'_dump',count=4096)
                    (args.out/('death-allocation-'+kind+'.json')).write_bytes((json.dumps(trace,indent=2)+'\n').encode())
                r.request('fn_disable')
            obs('final')
            if not r.personal_effects()['scene'][0]:r.tap(8,.15,0)
        except Exception:d['cleanup_error']=traceback.format_exc()
        save()
    print(json.dumps({key:d.get(key) for key in ('success','error')}),flush=True)
    return 0 if d['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())

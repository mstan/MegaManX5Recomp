"""Owned normal weapon8: menu, rising segments, original art and native contact.

Uses actually earned weapons and a read-only private-pool observer. Position
setup/HP protection are disclosed; original inputs, launch, child ownership,
enemy damage, ammo and retirement remain native. F174 beam is outside scope.
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
from coop_private_observer import PrivateObserver
from test_mmx5_coop_scene_fatal_runtime import serial


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--primary-button',type=lambda value:int(value,0),default=0x1000,
                        help='Physical configured primary special button; default Triangle in this native saved profile. Verify native mapped request8.')
    args=parser.parse_args();parent=json.loads(args.session.read_text(encoding='utf-8'))
    exe=Path(parent['command'][0]).resolve();assert Path(psutil.Process(parent['pid']).exe()).resolve()==exe
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==parent['executable_sha256']
    r=Runtime(parent['port']);assert not r.request('netplay_status')['active']
    args.out.mkdir(parents=True,exist_ok=False);(args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    observer=PrivateObserver(parent['pid'],exe)
    d=dict(success=False,candidate_sha256=parent['executable_sha256'],session=str(args.session.resolve()),samples=[],checks={},
           bypasses=['Actual saved earnedF9 card; native stage1 entry. P2 position220,379 for clear space, then position near an actual native vulnerable enemy. No target/phase/success writes.',
                     'HP-only refill. Incoming P2 contact protection only for the close target setup, so outgoing projectile contact is tested separately from incoming damage.'],
           limits='One normal rising-segmented weapon family. Exact original pixel proof is a separate analysis of captured frames/assets. No charged family, guided beamF174, all weapons, production or netplay claim.')

    def save():
        (args.out/'report.json').write_bytes((json.dumps(d,indent=2)+'\n').encode())

    def enemies():
        pool=r.read(0x80092090,48*0x9c)
        return [dict(address=hex(0x80092090+i),body=pool[i:i+0x9c].hex()) for i in range(0,len(pool),0x9c) if pool[i]]

    def obs(label):
        private=observer.second_shots();s=r.personal_effects()
        row=dict(label=label,private=private,personal=serial(s),enemies=enemies(),play=r.read(PLAY,0x1c0).hex())
        d['samples'].append(row);assert not s['state']['failed'] and s['life'][:2]==[0,0]
        return row

    def wait(test,label,seconds=20):
        end=time.monotonic()+seconds
        while not test():
            assert not r.state()['failed'] and time.monotonic()<end,label
            time.sleep(.015)

    def ticks(n):
        start=r.state()['world_ticks'];wait(lambda:r.state()['world_ticks']>=start+n,'Native world stopped')

    def shot8(row):return [a for a in row['private']['shots'] if bytes.fromhex(a['body'])[1]==8]

    def fire(label,capture=False):
        before=obs(label+'-before');body=bytes.fromhex(before['private']['body']);energy=struct.unpack_from('<H',body,0xb8)[0]
        result=dict(before=before,energy_before=energy,samples=[]);d.setdefault('attacks',[]).append(result)
        result['physical_primary_button']=hex(args.primary_button)
        r.input(args.primary_button,1);end=time.monotonic()+20;released=False;initialized=False;marked=False
        while True:
            row=obs(label+'-native');result['samples'].append(row);b=bytes.fromhex(row['private']['body']);ammo=struct.unpack_from('<H',b,0xb8)[0];shots=shot8(row)
            if b[0x96]==8:result['native_request8_observed']=True
            if ammo<energy and not released:r.input(0,1);released=True;result['native_ammo_debit']=[energy,ammo]
            primaries=[a for a in shots if bytes.fromhex(a['body'])[2]==0 and struct.unpack_from('<I',bytes.fromhex(a['body']),0x38)[0]]
            if primaries:
                initialized=True;result.setdefault('initialized_primary',primaries[0])
            if capture and not marked and len(shots)>=2 and any(bytes.fromhex(a['body'])[3] for a in shots):
                result['graphics_sample']=row;r.request('capture_mark',op='mark');marked=True
                stats=r.request('present_image_ring_stats');result['image_ring']=stats;result['captures']=[];result['gpu_frames']=[]
                for frame in range(stats['newest']-2,stats['newest']+1):
                    try:result['captures'].append(r.request('present_image_ring_get',frame=frame,path=str(args.out/f'{label}-frame-{frame}.png')))
                    except RuntimeError as e:result['captures'].append(dict(frame=frame,unavailable=str(e)))
                    gpu=r.request('gpu_frame_dump',frame=frame,count=8192);name=f'{label}-gpu-{frame}.json';(args.out/name).write_bytes((json.dumps(gpu,indent=2)+'\n').encode());result['gpu_frames'].append(dict(frame=frame,file=name))
                table=struct.unpack_from('<I',b,0x38)[0];base=table-0x3c
                result['x_resource_base']=hex(base);result['selected_palette']=r.read(base+0x50000+32,32).hex();result['original_bank_header']=r.read(base,0x3c).hex()
                r.screenshot(args.out/(label+'-composed.png'));r.request('capture_mark',op='release');save()
            if initialized and not shots and b[0x98]==body[0x98] and b[0x99]==body[0x99]:
                result['retired']=row;break
            assert time.monotonic()<end,'Native weapon8 launch/children/retirement did not finish'
            time.sleep(.01)
        r.input(0,1);assert released and initialized and result.get('native_request8_observed')
        result['energy_after']=struct.unpack_from('<H',bytes.fromhex(result['retired']['private']['body']),0xb8)[0]
        assert result['energy_after']<energy
        save();return result

    try:
        r.input(0,0);r.input(0,1);r.resume_native_pause();r.fixture(11,hp=3);r.fixture(12,hp=0)
        p=r.read(PLAY,0x1c0);assert p[0]==10 and p[12:14]==b'\x01\0' and p[0x4c]&0x80
        assert [b[2] for b in r.players()]==[1,0] and all(b[0xc9]&0x80 for b in r.players())
        wait(lambda:all(b[4:6]==b'\x01\x02' for b in r.players()),'Original loaded heroes not idle')
        for _ in range(9):
            if r.players()[1][0x93]==8:break
            r.input(0x800,1);ticks(6);r.input(0,1);ticks(8)
        else:raise AssertionError('Earned native cycle did not select weapon8')
        r.input(8,1);wait(lambda:r.read(PLAY+1,1)==b'\x02','P2 owned menu did not open');r.input(0,1)
        wait(lambda:r.read(0x800d4f44,1)==b'\x01' and r.read(0x8009a424,1)==b'\0','Native menu fade did not finish')
        assert r.read(r.diagnostic()+0x48,1)==b'\x02';d['owned_menu']=obs('native-owned-weapon8-menu');r.screenshot(args.out/'native-owned-weapon8-menu.png');save();r.resume_native_pause()
        r.teleport(1,220,379);r.input(0x20,1);ticks(2);r.input(0,1);ticks(3)
        p1_ammo=r.players()[0][0xa8:0xc8];first=fire('clear-space',capture=True)
        primaries=[bytes.fromhex(a['body']) for row in first['samples'] for a in shot8(row) if bytes.fromhex(a['body'])[2]==0 and bytes.fromhex(a['body'])[3]]
        assert len(primaries)>=2 and max(struct.unpack_from('<i',b,12)[0] for b in primaries)-min(struct.unpack_from('<i',b,12)[0] for b in primaries)>8*65536
        assert any(len(shot8(row))>=2 for row in first['samples']) and 'graphics_sample' in first
        d['checks']['owned_native_launch_rise_segments_retirement']='passed'
        # Source-valid ordinary targets remain native placements. Move only
        # the hero so the original camera admits them; never spawn/clone one.
        r.teleport(1,460,379);ticks(12)
        eligible=[a for a in enemies() if (b:=bytes.fromhex(a['body']))[4]==1 and 1<b[0x5c]<128 and struct.unpack_from('<I',b,0x54)[0] and 470<=struct.unpack_from('<i',b,8)[0]/65536<=700]
        assert eligible,'No native vulnerable ordinary target for vertical contact'
        target=eligible[0];t=bytes.fromhex(target['body']);address=int(target['address'],16);x=struct.unpack_from('<i',t,8)[0]/65536
        r.fixture(12,hp=2);wait(lambda:r.players()[1][4:6]==b'\x01\x02','P2 must finish existing hurt before protected setup')
        r.teleport(1,x-24,379);ticks(2);d['contact_target_before']=dict(address=hex(address),body=r.read(address,0x9c).hex())
        second=fire('native-contact');after=r.read(address,0x9c);d['contact_target_after']=after.hex();hp=bytes.fromhex(d['contact_target_before']['body'])[0x5c]
        assert (not after[0] or after[0x5c]<hp) and hp>0,'No actual native enemy damage/retirement from rising weapon'
        assert r.players()[0][0xa8:0xc8]==p1_ammo,'P2 special changed P1 ammo'
        d['checks']['actual_native_target_damage_and_private_ammo']='passed';r.fixture(12,hp=0)
        x=struct.unpack_from('<i',r.players()[1],8)[0];r.input(0x20,1);ticks(4);r.input(0,1)
        assert struct.unpack_from('<i',r.players()[1],8)[0]>x+65536
        d['checks']['native_control_after_retirement']='passed';d['success']=True
    except Exception:
        d['error']=traceback.format_exc()
        try:d['failure']=obs('failure');r.screenshot(args.out/'failure.png')
        except Exception:d['capture_error']=traceback.format_exc()
    finally:
        try:
            r.input(0,0);r.input(0,1);r.fixture(12,hp=0);r.request('capture_mark',op='release')
        finally:
            observer.close();save()
    print(json.dumps({k:d.get(k) for k in ['success','checks','error']}),flush=True)
    return 0 if d['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())

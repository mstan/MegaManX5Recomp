"""F147: native P1 pad contact during P2's actual Select withdrawal.

One original hub/HP/position setup; native Select, pad, loader and boss release
must do the work. No lifecycle/transition/success flags are authored.
"""
from pathlib import Path
import argparse,hashlib,json,psutil,struct,time,traceback
from coop_debug import Runtime,PLAY
from audit_mmx5_coop_locations import counter,wait_ready
from test_mmx5_coop_refight_runtime import serial

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    OUT=args.out;OUT.mkdir(parents=True,exist_ok=False)
    (OUT/'harness.py').write_bytes(Path(__file__).read_bytes())
    p=json.loads(args.session.read_text(encoding='utf-8'))
    exe=Path(p['command'][0]).resolve();process=psutil.Process(p['pid'])
    assert Path(process.exe()).resolve()==exe and process.cmdline()==p['command'],'Not the exact owned session'
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==p['executable_sha256']
    r=Runtime(p['port']);assert not r.request('netplay_status')['active']
    d=dict(success=False,target_row='F147',candidate_sha256=p['executable_sha256'],
           session=str(args.session.resolve()),harness_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),samples=[],checks={},inputs=[],
           bypasses=['Native12/0/CP1 hub loader bypasses preceding campaign.',
                     'P1 HP-only protection; P2 distinct17HP/73ammo setup, acquisition bypassed.',
                     'Prepared P1 position/history at actual pad top, sequence submitted during real P2 LEAVING.'],
           limits='One original pad2 race/localSplit/Both. No boss defeat/reward, other pad timings/rosters/network or mounted transfer claim.')

    def save():(OUT/'report.json').write_bytes((json.dumps(d,indent=2)+'\n').encode('utf-8'))

    def sample(label):
     s=r.personal_effects();play=r.read(PLAY,0x80);pool=r.read(0x80092090,48*0x9c)
     bosses=[dict(address=hex(0x80092090+i),body=pool[i:i+0x9c].hex())
             for i in range(0,len(pool),0x9c)if pool[i] and pool[i+1]==0x36]
     row=dict(label=label,personal=serial(s),play=play.hex(),bosses=bosses)
     d['samples'].append(row);save();return row,s,play

    try:
     r.release();r.resume_native_pause();r.fixture(11,hp=1);r.fixture(12,hp=0)
     previous=counter(r,'mmx5.coop.enrolled');r.load_location(12,0,1);wait_ready(r,previous)
     r.fixture(2,seat=1,hp=17,ammo=73)
     pool=r.read(0x800c6a10,0x2900)
     pads=[pool[i:i+0xa4]for i in range(0,len(pool),0xa4)if pool[i] and pool[i+1:i+3]==bytes([23,2])]
     assert len(pads)==1,'Authored pad2 not admitted from initial hub'
     pad=pads[0];body=r.players()[0]
     pb=r.read(struct.unpack_from('<I',pad,0x68)[0],4)
     bb=r.read(struct.unpack_from('<I',body,0x68)[0],4)
     x,y=struct.unpack_from('<ii',pad,8)
     y+=(struct.unpack_from('<b',pb,1)[0]-struct.unpack_from('<b',bb,1)[0]-pb[3]-bb[3])*65536
     d['actual_pad']=dict(body=pad.hex(),pad_box=pb.hex(),hero_box=bb.hex(),approach=[x/65536,y/65536])
     diagnostic=r.diagnostic();sequence=int.from_bytes(r.read(diagnostic+0x50,4),'little')+1
     assert sequence<=255
     fields={0x54:10,0x55:0,0x56:0,0x58:0,0x59:0}
     fields.update({0x90+i:v for i,v in enumerate(struct.pack('<ii',x,y))})
     for offset,value in fields.items():r.request('write_ram',addr=hex(diagnostic+offset),val=hex(value))
     before,s,play=sample('prepared-native-hub');d['before']=before
     assert s['life'][:2]==[0,0] and s['scene']==[0,0]
     assert s['bodies'][1][0x5c]==17
     r.screenshot(OUT/'native-hub-before-race.png')
     r.input(1,1);d['inputs'].append(dict(seat=1,button='Select',purpose='native90-update withdrawal hold'))
     deadline=time.monotonic()+15
     while True:
      # Keep this actual short race observation coherent; do not spend its window
      # on additional RAM reads, screenshots or instrumentation.
      s=r.personal_effects()
      if s['life'][1]==1:
       d['actual_leaving_before_position_commit']=serial(s)
       r.request('write_ram',addr=hex(diagnostic+0x50),val=hex(sequence))
       break
      assert s['life'][1]==0 and time.monotonic()<deadline,'Native withdrawal did not start'
      time.sleep(.01)
     r.input(0,1)
     row,s,play=sample('native-pad-contact-during-withdrawal');d['at_contact']=row
     assert int.from_bytes(r.read(diagnostic+0x64,4),'little')==1,'Position-only request rejected'
     assert s['life'][1]==1 and s['scene'][0]==1,'Actual scene/LEAVING overlap not observed'
     d['checks']['actual_native_scene_with_leaving_passenger']='passed'
     transitioned=False;deadline=time.monotonic()+90;last=None
     while True:
      row,s,play=sample('original-pad-loader-boss-intro')
      key=(play[:2].hex(),play[0x1d],s['life'][:2],s['scene'])
      if key!=last:print('Native withdrawal/pad',key,flush=True);last=key
      assert not s['state']['failed'] and s['life'][0]==0,'Native owner failed'
      transitioned|=play[0]==9 and play[0x1d]==2
      bosses=[bytes.fromhex(b['body'])for b in row['bosses']]
      ready=(transitioned and play[0]==10 and play[0x1d]==2 and not play[1] and not play[0x1c]
             and s['scene']==[0,0] and bosses and bosses[0][4]==1 and bosses[0][0x5c]==127
             and s['bodies'][0][4]==1 and not any(s['bodies'][0][i]for i in(0x7a,0xd0,0xd4)))
      if ready:
       d['active_fight']=row
       assert s['life'][1]==2,'Voluntarily withdrawing P2 was silently re-enrolled'
       assert s['bodies'][1][0x5c]==17 and list(struct.unpack_from('<9H',s['bodies'][1],0xa8))==[73]*9
       d['checks']['native_pad_loader_intro_release_absent_resources']='passed'
       start_x=struct.unpack_from('<i',s['bodies'][0],8)[0];start=s['state']['world_ticks']
       r.input(0x80,0)
       movement_deadline=time.monotonic()+5
       while r.state()['world_ticks']<start+6:
        assert time.monotonic()<movement_deadline,'Owner control check stopped advancing'
        time.sleep(.02)
       r.input(0,0);after,ss,pp=sample('actual-survivor-native-left')
       assert struct.unpack_from('<i',ss['bodies'][0],8)[0]<start_x
       assert ss['life'][1]==2 and ss['scene']==[0,0]
       d['checks']['actual_owner_controls_after_native_release']='passed'
       r.screenshot(OUT/'native-active-fight-absent-p2.png');d['success']=True;break
      assert time.monotonic()<deadline,'Native race did not reach finite active fight'
      if play[0]==10 and play[0x1c]:r.tap(0x4000,.025,max(0,s['scene'][0]-1))
      time.sleep(.04)
    except Exception:
     d['error']=traceback.format_exc()
     try:sample('failure');r.screenshot(OUT/'failure.png')
     except Exception:d['capture_error']=traceback.format_exc()
    finally:
     try:
      r.release()
      if r.read(PLAY+1,1)==b'\0':r.tap(8,.05,0)
     except Exception:d['cleanup_error']=traceback.format_exc()
     save();print(json.dumps({k:d.get(k)for k in('success','checks','error')},indent=2),flush=True)
    assert d['success']


if __name__=='__main__':
    main()

"""Native P2 death/view/glyph check on an owned Windows co-op intro session.

HP/contact and position/history are disclosed setup cheats. Native code must
create the death pose, particles, finite visibility and spectator transition.
Original common DAT1/2 provides the character palette. No effect, timer, pose,
scene or success field is written. Review the resulting images separately.
"""
from pathlib import Path
import sys,json,struct,time,traceback,subprocess,re,ctypes
from coop_debug import Runtime,PLAY
from coop_private_observer import PrivateObserver
from test_mmx5_coop_scene_fatal_runtime import serial


class CurrentCharacterAssets(ctypes.LittleEndianStructure):
    """Current co-op PE32+ descriptor; not an older-candidate ABI decoder.

    Mirror CharacterAssets in mmx5_coop_plugin.c. Native pointers and size_t
    are explicitly 64-bit here; guest resource addresses remain 32-bit.
    PrivateObserver independently rejects executables other than PE32+.
    """
    _fields_=[('graphics',ctypes.c_uint64),('graphics_bytes',ctypes.c_uint64),
        ('messages',ctypes.c_uint64),('message_bytes',ctypes.c_uint64),
        ('message_root',ctypes.c_uint32),('sprites',ctypes.c_uint32),
        ('assembly',ctypes.c_uint32),('colors',ctypes.c_uint32),
        ('palette',ctypes.c_uint32),('menu_colors',ctypes.c_uint32),
        ('menu_layout',ctypes.c_uint32),('ui_pixels',ctypes.c_uint64),
        ('hud_colors',ctypes.c_uint16*16),('loaded',ctypes.c_uint32),
        ('record',ctypes.c_uint32),('identity',ctypes.c_uint32)]


def current_character_palette(observer,assets,character):
    """Read one stable current descriptor without writing host/guest memory."""
    if character not in (0,1):raise RuntimeError('Expected X (0) or Zero (1) asset slot')
    stride=ctypes.sizeof(CurrentCharacterAssets)
    if stride!=120 or CurrentCharacterAssets.palette.offset!=48:
        raise RuntimeError('Current PE32+ CharacterAssets layout differs from the audited 120-byte ABI')
    address=assets+stride*character
    for attempt in range(64):
        raw=observer.read(address,stride)
        if raw==observer.read(address,stride):
            value=CurrentCharacterAssets.from_buffer_copy(raw)
            valid_record=value.record in ((88,93) if character==0 else (90,))
            resources=(('assembly',0x40000),('colors',0x4C000),
                ('palette',0x50000),('menu_colors',0x54000),('menu_layout',0x54400))
            if not (value.loaded==1 and valid_record and value.sprites and
                    all(getattr(value,name)==value.sprites+offset for name,offset in resources)):
                raise RuntimeError('Asset descriptor does not match the current loaded CharacterAssets ABI; no older-layout fallback')
            return dict(scope='current PE32+ CharacterAssets with record/identity; older candidate layouts unsupported',
                stride=stride,palette_offset=CurrentCharacterAssets.palette.offset,
                descriptor=hex(address),character=character,record=value.record,
                palette=hex(value.palette),attempts=attempt+1,
                observation='two equal read-only host descriptor copies')
        time.sleep(.001)
    raise RuntimeError('No stable current character asset descriptor within 64 read-only attempts')


def main():
    import argparse,hashlib,psutil
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--dat',type=Path,required=True,help='Original ROCK_X5.DAT for the native character palette')
    a=parser.parse_args();s=json.loads(a.session.read_text(encoding='utf-8-sig'));r=Runtime(s['port']);out=a.out
    exe=Path(s['command'][0]).resolve();assert Path(psutil.Process(s['pid']).exe()).resolve()==exe
    assert hashlib.sha256(exe.read_bytes()).hexdigest()==s['executable_sha256']
    assert not r.request('netplay_status')['active'];assert r.read(PLAY+12,2)==bytes((0,0)), 'Requires original intro'
    out.mkdir(parents=True,exist_ok=False);(out/'harness.py').write_bytes(Path(__file__).read_bytes())
    dat=a.dat.read_bytes()
    
    d={'success':False,'samples':[],'palette_samples':[],'candidate_sha256':s['executable_sha256'],'setup':['HP/contact setup and position/history only. P2 fatal HP fixture invokes original native death; no particle/animation/palette/timer/success writes.'],'scope':'Observe actual counterpart P2 death pose/particles/own palette/visibility and survivor control; native radials need not deallocate merely when no longer drawn.'}
    d.update(session=str(a.session.resolve()),harness_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),dat_sha256=hashlib.sha256(dat).hexdigest(),human_validation=False,limits='One current on-foot P2 death in the original intro. Captures require visual review; passing emission/color/state checks alone does not prove the composed pixels. No vehicle, boss approach, fatal contact, or broad network pass.')
    def save():(out/'report.json').write_bytes((json.dumps(d,indent=2)+'\n').encode())
    def observe(label):
     data=r.read(r.diagnostic(),0xE88);z={'state':dict(zip(('ticks','ready','enrolled','failed','counterpart','banks','world_ticks','native_timer'),struct.unpack_from('<8I',data))),'life':list(data[0x40:0x44]),'scene':list(data[0x4c:0x4e]),'bodies':[data[0x300:0x458],data[0x100:0x258]]};pool=r.read(0x800d1f40,0x3000);effects=[{'address':hex(0x800d1f40+i),'body':pool[i:i+0x60].hex()}for i in range(0,len(pool),0x60)if pool[i]and pool[i+1]==0x11];row={'label':label,'personal':serial(z),'effects':effects,'play':r.read(PLAY,0x30).hex(),'views':data[0x710:0x908].hex()};d['samples'].append(row);return row,z,effects
    try:
     r.release();r.resume_native_pause();r.fixture(11,hp=3);r.fixture(12,hp=3);r.teleport_pair(((160,1131),(400,1130)),(0,960));start=r.state()['world_ticks']
     while r.state()['world_ticks']<start+8:time.sleep(.01)
     d['before']=observe('before-native-fatal')[0];assert d['before']['personal']['life'][:2]==[0,0]
     o=PrivateObserver(s['pid'],s['command'][0]);symbols=subprocess.check_output(['C:/msys64/mingw64/bin/nm.exe','--defined-only',s['command'][0]],text=True)
     address=lambda name:int(re.findall(r'^([0-9a-fA-F]+) [bBdD] '+name+'$',symbols,re.M)[0],16)
     assets=o.second+address('assets')-address('second');char=r.state()['counterpart'];d['asset_layout_observation']=current_character_palette(o,assets,char);palette=int(d['asset_layout_observation']['palette'],16);d['p2_palette']=hex(palette)
     r.fixture(11,hp=1);r.request('fn_clear');r.request('fn_filter',lo='0x8002D27C',hi='0x8002D280');r.fixture(1,seat=1);start=r.state()['world_ticks'];end=time.monotonic()+30;captures=set();fell_at=None
     while time.monotonic()<end:
      row,z,effects=observe('native-death')
      assert z['life'][0]==0,'Fatal lost living partner'
      personal=z['bodies'][1];age=z['state']['world_ticks']-start
      views=bytes.fromhex(row['views']);camx=[struct.unpack_from('<h',views,i*0xfc+10)[0]for i in(0,1)]
      if z['life'][1]==4 and z['scene']==[0,0]:assert abs(camx[1]-camx[0])>100,'Dying seat lost its own camera'
      if z['life'][1]==5:
       if fell_at is None:fell_at=z['state']['world_ticks']
       if z['state']['world_ticks']>=fell_at+2:assert views[:0xfc]==views[0xfc:],'Finished death did not become spectator'
      if personal[4]==2 and personal[3] and 'pose'not in captures:
       r.screenshot(out/'native-fatal-pose.png');captures.add('pose')
      if effects and len(d['palette_samples'])<40:
       native=struct.unpack('<I',r.read(0x1f800028,4))[0];before=r.read(palette+64,32);gpu=r.request('vram_peek',x=32,y=480,w=16,h=1);after=r.read(palette+64,32)
       d['palette_samples'].append({'world':z['state']['world_ticks'],'p2_palette_before':before.hex(),'p2_palette_after':after.hex(),'primary_palette':r.read(native+64,32).hex(),'gpu':gpu,'effects':effects[:2]})
      if len(effects)>=8 and 'random'not in captures:r.screenshot(out/'native-particles-early.png');captures.add('random')
      if any(not bytes.fromhex(e['body'])[2]&128 for e in effects)and 'radial'not in captures:
       r.screenshot(out/'native-particles-radial.png');captures.add('radial');d['radial_first']=row
      if age>=360:break
      time.sleep(.01)
     d['after']=observe('native-death-after')[0];
     assert len(d['after']['effects'])==32 and all(not bytes.fromhex(e['body'])[2]&128 and not bytes.fromhex(e['body'])[3]for e in d['after']['effects']),'Native random retirement/radial visibility mismatch'
     d['allocations']=r.request('fn_entry_dump',count=512,addr_lo='0x8002D27C',addr_hi='0x8002D280');r.request('fn_disable');d['captures']=sorted(captures)
     assert d['after']['personal']['life'][:2]==[0,5]
     assert any(bytes.fromhex(e['body'])[2]&128 and bytes.fromhex(e['body'])[3] for row in d['samples'] for e in row['effects']),'Native random death glyph never survived culling'
     x=struct.unpack_from('<i',r.players()[0],8)[0];r.input(0x20,0);tick=r.state()['world_ticks']
     while r.state()['world_ticks']<tick+6:time.sleep(.01)
     r.input(0,0);d['survivor_right']=(struct.unpack_from('<i',r.players()[0],8)[0]-x)/65536;assert d['survivor_right']>1
     assert any(v['name']=='mmx5.coop.p2-death-glyphs-drawn'and v['count']>0 for v in r.request('mod_counters')['counters']),'No private death glyph draw'
     sector,size=struct.unpack_from('<II',dat,(char+1)*8);common=dat[sector*2048:sector*2048+size];assert struct.unpack_from('<I',common,4)[0]==size and struct.unpack_from('<II',common,8)[0]==5;reference=common[2048+64:2048+96].hex();assert all(p['p2_palette_before']==p['p2_palette_after']==reference for p in d['palette_samples']),'Private CLUT differs from original character common palette'
     r.screenshot(out/'native-survivor.png');d['stats']=r.request('render_pass_stats');entries=d['allocations']['entries'];assert len(entries)==152,'Death allocated beyond native76 attempts'
     for i in range(0,len(entries),2):assert {k:v for k,v in entries[i].items()if k not in('seq','exit_seq')}=={k:v for k,v in entries[i+1].items()if k not in('seq','exit_seq')}
     assert sum(e['ra']=='0x8003DABC'for e in entries[::2])==44 and sum(e['ra']=='0x8003DB3C'for e in entries[::2])==32
     d['success']=True
    except Exception:d['error']=traceback.format_exc();print(d['error'],flush=True)
    finally:
     r.release()
     try:r.request('fn_disable');r.tap(8,.15,0)
     except Exception:pass
     try:o.close()
     except Exception:pass
     save()
    print(json.dumps({'success':d['success'],'samples':len(d['samples']),'palettes':len(d['palette_samples']),'captures':d.get('captures'),'survivor_right':d.get('survivor_right'),'error':d.get('error')}),flush=True)
    
    return 0 if d['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())

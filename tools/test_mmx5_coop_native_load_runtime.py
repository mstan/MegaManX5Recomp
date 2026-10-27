"""Reload a real native X5 card with P1 input, then qualify the co-op roster.

Attach to the owned title-only card runner. Never write RAM, progression,
weapons or card contents. The source card must come from an actual native
save; this is not snapshot/rewind or broad campaign qualification.
"""
import argparse
import hashlib
import json
from pathlib import Path
import time
import traceback

import psutil
from coop_debug import Runtime, PLAY


def require(value, message):
    if not value:
        raise AssertionError(message)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--entry-only',action='store_true',
                        help='Stop after native card/roster entry; weapon cycle and guest menu remain untested in this receipt')
    parser.add_argument('--loaded-selector-receipt',type=Path,
                        help='Continue the same observed native load after a caller assertion, with exact receipt/session/hash guards')
    parser.add_argument('--expect-defeated-mask',type=lambda v:int(v,0),default=1)
    parser.add_argument('--stage',type=int,choices=range(1,9),default=1,
                        help='Earned Maverick to revisit through the native selector')
    parser.add_argument('--character',choices=('native','x','zero'),default='native',
                        help='Expected saved P1 hero; co-op armor selection preserves the established seats')
    args=parser.parse_args()
    parent=json.loads(args.session.read_text(encoding='utf-8'))
    exe=Path(parent['command'][0]).resolve()
    process=psutil.Process(parent['pid'])
    require(Path(process.exe()).resolve()==exe,'Not the owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest()==parent['executable_sha256'],'Binary changed')
    r=Runtime(parent['port'])
    require(not r.request('netplay_status')['active'],'Offline only')
    args.out.mkdir(parents=True,exist_ok=False)
    (args.out/'harness.py').write_bytes(Path(__file__).read_bytes())
    cards=Path(parent['command'][parent['command'].index('--memcard-dir')+1])
    card=cards/'card1.mcd'
    require(hashlib.sha256(card.read_bytes()).hexdigest()==parent['card_sha256'],'Private card differs from source copy')
    report=dict(success=False,native_load_passed=False,human_validation=False,
                session=str(args.session.resolve()),executable_sha256=parent['executable_sha256'],
                harness_sha256=hashlib.sha256(Path(__file__).read_bytes()).hexdigest(),
                source_card_sha256=parent['card_sha256'],inputs=[],trace=[],text_pages=[],
                limits='Actual card recognition/load, stored clear bits, native selection/re-entry, roster, acquired X weapon and guest menu only. No RAM writes; no snapshot/rewind, projectile behavior, original portrait-color or broader campaign pass.')

    def save():
        (args.out/'report.json').write_bytes((json.dumps(report,indent=2)+'\n').encode('utf-8'))

    def frames(n,seconds=15):
        start=r.request('frame')['frame'];deadline=time.monotonic()+seconds
        while r.request('frame')['frame']<start+n:
            require(time.monotonic()<deadline,'Native frontend frames stopped')
            time.sleep(.02)

    def pulse(button,seat=0):
        report['inputs'].append(dict(seat=seat,button=hex(button),play=r.read(PLAY,4).hex()))
        r.input(button,seat)
        try:frames(6)
        finally:r.input(0,seat)
        frames(8)

    def capture(label):
        r.screenshot(args.out/(label+'.png'))
        save()

    def state():
        s=r.personal_effects()
        return dict(state=s['state'],play=r.read(PLAY,0x1C0).hex(),life=s['life'],
                    scene=s['scene'],bodies=[b.hex() for b in s['bodies']])

    try:
        r.release()
        if args.loaded_selector_receipt:
            prior=json.loads(args.loaded_selector_receipt.read_text(encoding='utf-8'))
            require(Path(prior['session']).resolve()==args.session.resolve() and
                    prior['executable_sha256']==parent['executable_sha256'] and
                    prior['source_card_sha256']==parent['card_sha256'],'Prior native load identity differs')
            trace=[bytes.fromhex(row['play']) for row in prior['trace']]
            require(any(p[0]==0 and p[1]==4 and p[0x4C]==args.expect_defeated_mask for p in trace),
                    'Prior receipt does not observe native card decoding')
            require(any(not p[0x4C] for p in trace) and r.read(PLAY,3)==b'\x04\x04\0',
                    'Same native load is not parked at its selector')
            report['prior_load_receipt']=dict(path=str(args.loaded_selector_receipt.resolve()),
                sha256=hashlib.sha256(args.loaded_selector_receipt.read_bytes()).hexdigest())
            report['trace']=prior['trace']
            report['text_pages']=prior['text_pages']
        else:
            deadline=time.monotonic()+70
            while r.read(0x800D1F28,1)!=b'\x06':
                require(time.monotonic()<deadline,'Original title controller did not appear')
                pulse(8)
            while r.read(0x800D1F29,1)!=b'\x01' or r.read(0x8009A424,1)!=b'\0':
                require(time.monotonic()<deadline,'Original title fade did not finish')
                time.sleep(.03)
            capture('native-title')
            pulse(0x40)
            require(r.read(0x800D1F2A,1)==b'\x01','Continue cursor did not move')
            capture('native-continue-selected')
            pulse(0x4000)
            deadline=time.monotonic()+180
            signature=None
            while True:
                p=r.read(PLAY,0x1C0)
                if p[:4]!=signature:
                    signature=p[:4]
                    report['trace'].append(dict(play=p.hex()))
                    print('Native load',p[:4].hex(),list(p[12:14]),'mask',p[0x4C],flush=True)
                    save()
                if p[:3]==b'\x04\x04\0':break
                require(time.monotonic()<deadline,'Original card/story did not reach interactive selector')
                require(p[0] not in (7,8,9,10),'Unexpected stage entry before selector')
                if p[0]==20:
                    if r.read(0x80091C44,2)==b'\xff\x03':
                        report['text_pages'].append(r.confirm_native_text(timeout=3))
                    else:time.sleep(.03)
                elif p[0]==4 and p[1]==3:
                    time.sleep(.03)
                elif p[0]==0 and p[1]==3:
                    if p[3]!=4:time.sleep(.03);continue
                    if p[10]:pulse(0x80)
                    else:pulse(0x4000)
                else:pulse(0x4000)
        p=r.read(PLAY,0x1C0)
        require(p[0x4C]==args.expect_defeated_mask,'Native load changed stored defeated mask')
        card_bytes=card.read_bytes()
        require(card_bytes[0x80]==0x51 and card_bytes[0x2000:0x2002]==b'SC',
                'This bounded check expects the real native block1/slot1 card')
        saved_availability=card_bytes[0x2200+0x58]
        require(p[0x4A]==saved_availability,'Native load lost saved character availability')
        saved_hero_field=card_bytes[0x2200+0x5F]
        native_default=0 if saved_hero_field else 1
        selected_hero=native_default if args.character=='native' else int(args.character=='zero')
        require(selected_hero==native_default,
                'Co-op preserves the saved P1 hero. Start a new campaign or load a native save for the requested seat assignment.')
        require(p[0x44]==native_default,'Native card restore differs from original slot5F rule')
        report['native_default_hero']=dict(saved_availability=saved_availability,
            saved_slot5f=saved_hero_field,expected=native_default,actual=p[0x44],
            source='Actual card-buffer1C904 restores slot5F to PLAY1BA, then1CBDC..1CBF4 maps nonzero to X0. Separate current-data1CC34 uses slot58 availability. Co-op armor confirmation preserves that hero and assigns the chosen X form to its established seat.')
        report['loaded_selector']=p.hex()
        table=r.read(0x800F5050,8)
        require(set(table)==set(range(1,9)),'Unexpected native selector table')
        report['stage_table']=list(table)
        capture('native-loaded-selector')
        require(args.expect_defeated_mask & (1 << (args.stage-1)),
                'Revisit must use a boss actually earned by this native card')
        target=table.index(args.stage)
        for _ in range(8):
            cursor=r.read(PLAY+0x28,1)[0]
            require(cursor<8,'Invalid native portrait cursor')
            if cursor==target:break
            if cursor//4!=target//4:pulse(0x20 if target>=4 else 0x80)
            else:pulse(0x40 if cursor%4<target%4 else 0x10)
        else:raise AssertionError('Native selector did not choose requested earned stage')
        pulse(0x4000)
        deadline=time.monotonic()+90
        selected=False
        while True:
            p=r.read(PLAY,0x20)
            # Loaded-game character confirmation has its own native handler.
            # 04/04/02 is the fade; only 04/04/05 consumes roster input.
            if p[:3]==b'\x04\x04\x05' and not selected:
                capture('native-character-selection')
                count=r.read(0x800F6DFB,1)[0]
                require(0<count<=5,'Invalid co-op X armor roster count')
                roster=r.read(0x800F6DF0,count)
                require(5 not in roster and 1 in roster,'Co-op must offer X-only forms with Fourth always available')
                cursor=r.read(PLAY+0x30,1)[0]
                require(cursor<count,'Invalid original armor cursor')
                report['character_choice']=dict(request=args.character,hero=selected_hero,
                    x_seat=selected_hero,armor=roster[cursor],roster=list(roster),cursor=cursor)
                pulse(0x4000,selected_hero);selected=True
            elif p[0]==10:
                s=r.personal_effects()
                if s['life'][:2]==[0,0] and not p[1] and not p[0x1C] and all(b[4:6]==b'\x01\x02' for b in s['bodies']):break
                if p[0x1C]:pulse(0x4000)
                else:time.sleep(.03)
            else:time.sleep(.03)
            require(time.monotonic()<deadline,'Native loaded roster did not enter/control requested stage')
        report['native_entry']=state()
        p=bytes.fromhex(report['native_entry']['play'])
        bodies=[bytes.fromhex(b) for b in report['native_entry']['bodies']]
        require(p[12:14]==bytes((args.stage,0)),'Wrong native stage')
        # PLAY44 temporarily names the projected P2 character during native
        # gameplay. The original frontend choice and coherent primary body
        # establish the roster; an unbracketed live PLAY read cannot do so.
        require([b[2] for b in bodies]==[selected_hero,selected_hero^1],'Loaded co-op roster differs')
        require(p[0x4C]==args.expect_defeated_mask,'Native entry lost saved clear mask')
        capture('native-loaded-both-heroes')
        report['entry_only']=args.entry_only
        if args.entry_only:
            report['limits']='Actual native card recognition/load, stored clear bits, character selection and co-op stage entry only. Weapon cycle, guest menu, route, combat and transitions untested in this receipt.'
        else:
            x_seat=selected_hero
            require(all(b[0xC9]==args.expect_defeated_mask for b in bodies),
                    'Native load lost a hero acquired-weapon mask')
            # Original3DE28..3DEF4 uses this authored cycle, not ascending IDs.
            # In particular, actual earned mask06 cycles0->3->2->0.
            order=list(r.read(0x80074A28,9))
            masks=list(r.read(0x80074A1C,9))
            require(order==[0,1,5,6,3,8,7,2,4] and
                    masks==[0,1,16,32,4,128,64,2,8],'Unexpected original weapon cycle tables')
            expected=[weapon for weapon,mask in zip(order,masks)
                      if mask & args.expect_defeated_mask]+[0]
            actual=[bodies[x_seat][0x93]]
            require(actual==[0],'Loaded X must begin with the normal buster')
            for weapon in expected:
                r.input(0x800,x_seat);frames(6);r.input(0,x_seat);frames(8)
                actual.append(r.personal_effects()['bodies'][x_seat][0x93])
                require(actual[-1]==weapon,'Native load lost an earned X selection or cycle wrap')
            report['earned_x_cycle']=dict(seat=x_seat,actual=actual,expected=[0]+expected,
                                          native_order=order,native_masks=masks)
            r.input(8,1);frames(6);r.input(0,1)
            deadline=time.monotonic()+10
            while r.read(PLAY+1,1)!=b'\x02' or r.read(0x800D4F44,1)!=b'\x01':
                require(time.monotonic()<deadline,'Loaded guest menu did not open')
                time.sleep(.03)
            require(r.read(r.diagnostic()+0x48,1)==b'\x02','Loaded guest did not own its menu')
            report['parked_owned_menu']=state()
            capture('native-loaded-guest-menu')
        report['native_load_passed']=report['success']=True
    except Exception:
        report['error']=traceback.format_exc();print(report['error'],flush=True)
        try:report['failure']=state();capture('failure')
        except Exception:report['capture_error']=traceback.format_exc()
    finally:
        try:r.release()
        except Exception:report['cleanup_error']=traceback.format_exc()
        report['private_card_after_sha256']=hashlib.sha256(card.read_bytes()).hexdigest()
        save()
    print(json.dumps({k:report.get(k) for k in ['success','error','native_load_passed','earned_x_cycle']},ensure_ascii=True),flush=True)
    return 0 if report['success'] else 1


if __name__=='__main__':
    raise SystemExit(main())

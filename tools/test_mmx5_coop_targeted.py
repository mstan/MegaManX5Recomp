"""Small, cheat-assisted alpha check on private synchronized Split peers.

Requires MMX5_COOP_TEST_CHEATS=ON. All mutations arrive as logical P1 input;
debug TCP only observes state. Real Internet/Host-Join testing is owner QA.
"""
import argparse
from concurrent.futures import ThreadPoolExecutor
import json
import os
from pathlib import Path
import shutil
import struct
import time
import traceback

from coop_debug import Runtime, PLAY
from test_mmx5_coop_netplay import Peer, prepare_peer, reserve_ports, parse_crc_log, require, sha256_file


def main():
    root = Path(__file__).resolve().parents[1]
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--exe', type=Path, default=root/'build-coop/qualification-cheats/mmx5-runtime.exe')
    p.add_argument('--out', type=Path, required=True)
    p.add_argument('--save-card', type=Path, required=True, help='Unmodified downloaded 128KB PS1 card')
    p.add_argument('--save-source-url', required=True, help='Online source of the downloaded card')
    p.add_argument('--keep-running', action='store_true', help='Keep only these owned peers for failed-case research')
    p.add_argument('--only',choices=('all','boss','vehicles'),default='all',help='Run only the boss or vehicle cases when isolating a failure')
    p.add_argument('--resume-after-boss',type=Path,help='Resume these still-running, owned peers after a retained save-menu driver failure')
    p.add_argument('--human-return',action='store_true',help='Leave a visible pair at the native post-boss save prompt for owner validation')
    args = p.parse_args()
    if args.human_return:
        args.only='boss';args.keep_running=True
    args.exe = args.exe.resolve(); args.out = args.out.resolve()
    args.out.mkdir(parents=True, exist_ok=False)
    args.game_root = root; args.config = root/'game.toml'
    args.disc = root/'mmx5/Mega Man X5 (USA).cue'
    args.bios = root/'build-coop/bios/openbios.bin'
    args.cameras = 'split'; args.frontend = 'hidden-window'; args.delay = 3
    args.seed = 1296914485; args.latency = args.jitter = 0
    args.bios_hle = True; args.render_verify = not args.human_return
    args.card_seed = args.out/'seed'; args.card_seed.mkdir()
    card = args.save_card.resolve()
    require(args.save_source_url.startswith(('https://','http://')), 'Downloaded save source URL required')
    require(card.is_file(), 'Downloaded save card not found')
    require(len(card.read_bytes())==131072 and card.read_bytes()[:2]==b'MC', 'Use a downloaded native 128KB PS1 card')
    require(sha256_file(card)!='4a173ef3b348f61073a89b8e416687f556b44b7475ef60c0e3564a0ac093f3e9',
            'Retired synthetic stage-access card cannot be reused')
    shutil.copy2(card, args.card_seed/'card1.mcd')
    prior=None
    if args.resume_after_boss:
        import psutil
        import subprocess
        case=args.resume_after_boss.resolve()
        require(case.is_relative_to(root/'build-coop'),'Resume case must be an owned QA directory')
        prior=json.loads((case/'report.json').read_text())
        require(prior['executable_sha256']==sha256_file(args.exe),'Resume binary differs from the retained case')
        require(any(c['name']=='grizzly-fight' for c in prior['checks']),'No validated boss encounter to resume')
        class OwnedProcess:
            def __init__(self,pid,expected):
                self.handle=psutil.Process(pid);self.pid=pid
                require(Path(self.handle.exe()).resolve()==expected.resolve(),'Refusing to adopt an unrelated process')
            def poll(self):return None if self.handle.is_running() else 0
            def terminate(self):self.handle.terminate()
            def kill(self):self.handle.kill()
            def wait(self,timeout):
                try:return self.handle.wait(timeout=timeout)
                except psutil.TimeoutExpired:raise subprocess.TimeoutExpired(str(self.pid),timeout)
        ports=prior['ports'];reservations=[]
        peers=[Peer(s,case/f'peer-{s}',ports[2+s],ports[s],[],{}) for s in (0,1)]
        for peer,pid in zip(peers,prior['pids']):
            peer.process=OwnedProcess(pid,peer.directory/args.exe.name)
    else:
        ports, reservations = reserve_ports()
        peers = [prepare_peer(args,args.out,s,ports,False,args.seed) for s in (0,1)]
    for peer in peers:
        # Native disc/graphics loads can delay servicing the debug queue;
        # avoid turning a slow observer response into a gameplay failure.
        peer.debug_timeout = 12
        peer.environment.update(PATH='C:/msys64/mingw64/bin;'+os.environ['PATH'],
                                SDL_AUDIODRIVER='dummy',PSX_DEV_INPUT='0')
        if not prior:
            if args.human_return:peer.argv.remove('--hidden-window')
            cards = peer.directory/'saves/openbios'; cards.mkdir()
            shutil.copy2(args.card_seed/'card1.mcd',cards/'card1.mcd')
    report = dict(success=False,human_validation=False,internet_validation=False,
                  executable_sha256=sha256_file(args.exe),scope=args.only,checks=[],commands=[])
    if prior:
        report.update(resumed_from=str(args.resume_after_boss.resolve()),prior_driver_error=prior.get('error'),
                      checks=prior['checks'],commands=prior['commands'])
    pool = ThreadPoolExecutor(max_workers=2)
    def both(method,*values,**fields):
        futures = [pool.submit(getattr(peer,method),*values,**fields) for peer in peers]
        return [future.result() for future in futures]
    def read(addr,size=1): return peers[0].read(addr,size)
    def ticks(n):
        start = min(s['tick'] for s in both('status')); deadline = time.monotonic()+90
        while min(s['tick'] for s in both('status')) < start+n:
            require(time.monotonic()<deadline,'Synchronized peers stopped advancing')
            time.sleep(.02)
    def pulse(button,n=2,seat=0):
        peers[seat].input(button); ticks(n); peers[seat].input(0); ticks(9)
    def wait(predicate,label,seconds=90,action=None):
        deadline = time.monotonic()+seconds
        while not predicate():
            require(time.monotonic()<deadline,label+' timeout')
            if action: action()
            else: ticks(3)
    def command(number):
        diagnostics = [Runtime(peer.tcp).diagnostic() for peer in peers]
        before = [struct.unpack('<I',peer.read(addr+0x80,4))[0] for peer,addr in zip(peers,diagnostics)]
        pulse(6|(number<<8),n=6)
        wait(lambda: all(struct.unpack('<I',peer.read(addr+0x80,4))[0]==seq+1
                        for peer,addr,seq in zip(peers,diagnostics,before)),f'Cheat {number}',15)
        receipts = [struct.unpack('<III',peer.read(addr+0x80,12)) for peer,addr in zip(peers,diagnostics)]
        require(receipts[0]==receipts[1] and receipts[0][1]==number and receipts[0][2],
                f'Cheat {number} rejected or disagreed: {receipts}')
        report['commands'].append(dict(command=number,receipts=receipts))
    def checkpoint(name):
        ticks(70)
        logs = [parse_crc_log(peer.log) for peer in peers]
        common = sorted(set(logs[0])&set(logs[1])); require(len(common)>2,'Missing peer digests')
        for tick in common: require(logs[0][tick]==logs[1][tick],f'{name}: CRC mismatch at {tick}')
        probes=[]
        for peer in peers:
            counters={r['name']:r['count'] for r in peer.request('mod_counters')['counters']}
            stats=peer.request('render_pass_stats')
            require(counters.get('mmx5.coop.test-cheats-enabled')==1,'Not a qualification build')
            require(stats.get('local_views',0)>0,'Independent views were not exercised')
            if args.render_verify:
                require(stats.get('verify_checks',0)>0,'Restoration verifier was not exercised')
            require(not counters.get('mmx5.coop.local-view-state-leak',0),'Private state leaked from local draw')
            require(all(stats.get(k,0)==0 for k in ('verify_mismatch','watchdog','device_reads','vram_leaks')),
                    f'{name}: sandbox fault {stats}')
            probes.append(dict(state=peer.observe(),stats=stats))
            Runtime(peer.tcp).screenshot(args.out/f'peer-{peer.seat}-{name}.png')
        report['checks'].append(dict(name=name,matched_samples=len(common),through=common[-1],peers=probes))
        print('PASS',name,'matched core/mod through',common[-1],flush=True)
    def open_stage(stage):
        wait(lambda: all(s['mode']==10 and s['minor']==0 and s['stage']==stage and
                         s['enrolled'] and not s['gate'] and s['lifecycle'][:2]==[0,0]
                         for s in both('observe')),'Native stage arrival',action=lambda:pulse(0x4000))
        both('input',0)
    def finish_victory():
        def return_input():
            play=read(PLAY,4)
            if play[0]==18:
                # The original save browser labels Triangle "Save cancel".
                # Circle backs out of the confirmation but leaves the browser.
                pulse(0x1000,6)
            elif play[0]==14 and play[1]==1:
                # The original prompt defaults to its upper Save row. Choose
                # the lower Cancel row once, after the fade, then confirm.
                if read(0x8009A424)!=b'\x00':ticks(3)
                else:
                    both('input',0);ticks(20)
                    Runtime(peers[0].tcp).screenshot(args.out/'save-prompt.png')
                    pulse(0x40,6);pulse(0x4000,6)
            elif play[0]==4 and play[1]==3:
                both('input',0);ticks(120)
            else:pulse(0x4000,6)
        wait(lambda:read(PLAY,3)==b'\x04\x04\x00','Native victory/reward/selector return',
             seconds=180,action=return_input)
        require(read(PLAY+0x4C)[0]==1,'Victory changed additional boss flags')
        checkpoint('grizzly-victory-selector')
    def boss_case():
        command(2);open_stage(1)
        # CP4 is the authored corridor before the final boss gate. Walk both
        # original bodies through it; the cheat does not open doors/spawn bosses.
        both('input',0x20)
        wait(lambda:read(0x80092091)==b'\x36','Native Grizzly activation')
        both('input',0)
        wait(lambda:read(0x80092095)!=b'\x09' and 0<read(0x800920EC)[0]<128,
             'Native boss conversation and health fill',action=lambda:pulse(0x4000,6))
        checkpoint('grizzly-fight')
        command(3)
        wait(lambda:read(PLAY+0x4C)[0]&1,'Native boss defeat flag')
        if args.human_return:
            wait(lambda:read(PLAY,2)==b'\x0e\x01' and read(0x8009A424)==b'\x00',
                 'Human save prompt',seconds=120,action=lambda:pulse(0x4000,2))
            both('input',0)
            checkpoint('grizzly-reward-human-handoff')
            report['human_return_pending']=True
            report['success']=False
            print('HANDOFF native save prompt; return remains pending human validation',flush=True)
            return
        finish_victory()
    try:
        for sock in reservations:sock.close()
        if not prior:
            for peer in peers:peer.launch()
        report['pids']=[peer.process.pid for peer in peers]; report['ports']=ports
        print('Owned targeted peers',report['pids'],'ports',ports,flush=True)
        deadline=time.monotonic()+60
        for peer in peers:
            while True:
                try:peer.status();peer.input(0);break
                except OSError:
                    require(peer.running() and time.monotonic()<deadline,'Boot failed');time.sleep(.1)
        if prior:
            finish_victory()
            if args.only=='boss':
                report['success']=True
                return 0
        else:
            wait(lambda:read(0x800D1F28)==b'\x06','Title',action=lambda:pulse(8,2))
            wait(lambda:read(0x800D1F29)==b'\x01' and read(0x8009A424)==b'\x00','Title fade')
            pulse(0x40);require(read(0x800D1F2A)==b'\x01','Continue selection failed');pulse(0x4000)
            deadline=time.monotonic()+120
            while read(PLAY,3)!=b'\x04\x04\x00':
                play=read(PLAY,16);require(time.monotonic()<deadline,'Continue timeout')
                if play[0]==4 and read(PLAY+0x47)[0]:
                    # Skip the loaded campaign's briefing through the same native
                    # mission-title/loader path used by the debug stage control.
                    break
                if play[0]==4 and play[1]==3:both('input',0);ticks(120);continue
                if play[:2]==b'\x00\x03':
                    if play[3]!=4:ticks(3);continue
                    if play[10]:pulse(0x80);continue
                pulse(0x4000);ticks(12)
            if args.only=='vehicles':
                command(8);open_stage(4);command(4)
            else:
                command(12);open_stage(1);command(4);checkpoint('grizzly-entry')
            if args.only=='boss':
                boss_case()
                if not args.human_return:report['success']=True
                return 0
            if args.only!='vehicles':
                command(1);open_stage(1)
                require(read(PLAY+0x1D)==b'\x01','Native checkpoint1 load failed')
                lives=read(PLAY+0x45)[0];require(0<lives<128,'No shared retries')
                command(6)
                wait(lambda:all(s['lifecycle'][1] in (4,5) for s in both('observe')),'P2 death')
                require(all(s['lifecycle'][0]==0 for s in both('observe')),'P2 death killed survivor')
                require(read(PLAY+0x45)[0]==lives,'One death consumed a shared retry')
                command(5)
                wait(lambda:read(PLAY+0x45)[0]!=lives,'Native team retry')
                require(read(PLAY+0x45)[0]==lives-1,'Team wipe spent incorrect retry count')
                open_stage(1);require(read(PLAY+0x1D)==b'\x01','Retry lost native checkpoint1')
                command(4);checkpoint('checkpoint-team-retry')
                boss_case()
        if args.only!='vehicles':
            command(8);open_stage(4)
        require(read(0x800EE970,4)==b'\x0a\x00\x00\x00','Wrong native Armor overlay')
        wait(lambda:read(0x800D1DC0)[0]!=0,'Authored Ride Armor')
        command(10);ticks(25)
        require(all(not Runtime(peer.tcp).players()[1][0xD5] for peer in peers),'P2 mounted P1 Armor')
        require(not read(0x800D1E57)[0]&0x40,'P2 occupied world Armor')
        command(9)
        wait(lambda:all(Runtime(peer.tcp).players()[0][0xD5]==255 for peer in peers)
             and read(0x800D1DC5)==b'\x01','P1 native Armor mount')
        pulse(0x4010,6)
        wait(lambda:all(not Runtime(peer.tcp).players()[0][0xD5] for peer in peers),'P1 native dismount')
        checkpoint('p1-armor-policy')
        command(13)
        command(7);both('input',0x20)
        wait(lambda:all(s['mode']==10 and s['stage']==5 and s['enrolled'] and
                       s['lifecycle'][:2]==[0,0] for s in both('observe')),'Native car arrival')
        wait(lambda:all(c[4:6]==b'\x01\x01' and c[0x70]&8
                       for peer in peers for c in Runtime(peer.tcp).cars()),'Grounded native cars')
        for peer in peers:
            for body,car in zip(Runtime(peer.tcp).players(),Runtime(peer.tcp).cars()):
                require(body[0x15]==car[0x15] and body[0x47]!=255,'Mounted rider pose/facing invalid')
        command(13)
        select_lives=read(PLAY+0x45)[0]
        peers[1].input(0x21)
        for _ in range(24):
            ticks(8)
            # Authored gaps can kill a rider on this automated straight trip.
            # The policy forbids voluntary beams, and permits native deaths.
            require(all(s['lifecycle'][1] not in (1,2,3,6) for s in both('observe')),
                    'Car rider voluntarily despawned during Select hold')
        peers[1].input(0x20)
        report['select_native_retries']=[select_lives,read(PLAY+0x45)[0]]
        for peer in peers:
            counters={r['name']:r['count'] for r in peer.request('mod_counters')['counters']}
            require(counters.get('mmx5.coop.car-departure-blocked',0)>0,'Sequence departure lock not exercised')
        both('input',0x20)
        wait(lambda:read(PLAY)==b'\x0d' or all(s['mode']==10 and s['enrolled'] and
             s['lifecycle'][:2]==[0,0] for s in both('observe')),'Native car death/retry boundary')
        # Restart the original road for one separate survivor/wipe scenario.
        command(7);both('input',0x20)
        wait(lambda:all(c[4:6]==b'\x01\x01' and c[0x70]&8
                       for peer in peers for c in Runtime(peer.tcp).cars()) and read(PLAY)==b'\x0a',
             'Fresh car driving')
        command(13)
        lives=read(PLAY+0x45)[0];command(6);peers[0].input(0x20)
        wait(lambda:all(s['lifecycle'][1] in (4,5) and s['lifecycle'][0]==0 for s in both('observe')),
             'Car P1 survivor')
        require(read(PLAY+0x45)[0]==lives,'Single car death spent retry')
        car=Runtime(peers[0].tcp).cars()[0];x,y=struct.unpack_from('<ii',car,8)
        peers[0].input(0x4020);ticks(6);peers[0].input(0x20)
        require(struct.unpack_from('<i',Runtime(peers[0].tcp).cars()[0],8)[0]>x,'Survivor car stopped')
        require(struct.unpack_from('<i',Runtime(peers[0].tcp).cars()[0],12)[0]<y,'Survivor lost native jump control')
        command(5)
        wait(lambda:read(PLAY+0x45)[0]!=lives,'Car shared retry')
        require(read(PLAY+0x45)[0]==((lives-1)&255),'Two car deaths spent incorrect retries')
        both('input',0x20)
        wait(lambda:all(s['mode']==10 and s['stage']==5 and s['enrolled'] and
                       s['lifecycle'][:2]==[0,0] for s in both('observe')),'Car team reload')
        checkpoint('car-select-survivor-retry')
        report['success']=True
    except BaseException:
        report['error']=traceback.format_exc(); print(report['error'],flush=True)
        for peer in peers:
            try:
                report.setdefault('failure_states',[]).append(peer.observe())
                Runtime(peer.tcp).screenshot(peer.directory/'failure.png')
            except Exception:pass
    finally:
        for peer in peers:
            if peer.running():
                try:peer.input(0)
                except Exception:pass
            if not args.keep_running:peer.stop()
        pool.shutdown()
        (args.out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8',newline='\n')
    return 0 if report['success'] else 1


if __name__=='__main__':raise SystemExit(main())

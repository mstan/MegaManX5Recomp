"""Check original jump-tone identity, independent SFX slots and muted PCM.

Attach only to an owned offline session. Native input makes sounds; setup
uses a native Grizzly checkpoint, HP refill and contact grace. Capture emulated
audio without routing it to speakers. This is not an audible quality review.
"""
import argparse
import hashlib
import json
from pathlib import Path
import struct
import time
import traceback
import wave

import psutil
from coop_debug import Runtime,PLAY
from audit_mmx5_coop_locations import counter,snapshot,wait_ready
from qualify_mmx5_audio_native import asset


def require(value,message):
    if not value:raise AssertionError(message)


def wait(predicate,label,seconds=15):
    deadline=time.monotonic()+seconds
    while not predicate():
        require(time.monotonic()<deadline,label);time.sleep(.02)


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--session',type=Path,required=True)
    parser.add_argument('--native-sdk',type=Path,required=True)
    parser.add_argument('--dat',type=Path,required=True)
    parser.add_argument('--out',type=Path,required=True)
    args=parser.parse_args()
    session=json.loads(args.session.read_text());exe=Path(session['command'][0]).resolve()
    require(Path(psutil.Process(session['pid']).exe()).resolve()==exe,'Not the owned process')
    require(hashlib.sha256(exe.read_bytes()).hexdigest()==session['executable_sha256'],'Owned binary changed')
    sdk=json.loads(args.native_sdk.read_text());dat=args.dat.read_bytes()
    require(sdk['success'] and hashlib.sha256(dat).hexdigest()==sdk['dat_sha256'],'Original SDK assets differ')
    args.out.mkdir(parents=True,exist_ok=False)
    r=Runtime(session['port']);require(not r.request('netplay_status')['active'],'Offline audio fixture required')
    report=dict(success=False,human_validation=False,executable_sha256=session['executable_sha256'],cases=[],
                session=str(args.session.resolve()),resident_loading=session.get('resident_loading',True),
                bypassed_prerequisites=['native Grizzly checkpoint1 load','HP refill/contact grace held'],
                limits='Voice identity/registers/slot overlap and retained muted PCM; no ear review or full DSP comparison.')
    def grounded():
        # Native action2 is grounded idle. Contact70 is an intermediate terrain
        # result and need not have the same phase in both captured contexts.
        # Assert actual lift-off below, rather than inferring a jump from SFX.
        return all(body[4]==1 and body[5]==2 for body in r.players())
    def capture(label,seats,button=0x4000):
        wait(grounded,'Heroes did not settle to native grounded idle')
        r.request('spu_events_reset')
        before=r.request('audio_stats')['taps'][0]['frames'];state=snapshot(r)
        for seat in seats:r.input(button,seat)
        time.sleep(.12)
        motion=snapshot(r)
        for seat in seats:r.input(0,seat)
        time.sleep(.45)
        events=r.request('spu_events',count=1024)['events']
        end=r.request('audio_stats')['taps'][0]['frames']
        response=r.request('audio_wav',tap=0,path=str((args.out/(label+'.wav')).resolve()),
                           start=str(before),count=str(end-before))
        with wave.open(str(args.out/(label+'.wav')),'rb') as audio:
            require(audio.getsampwidth()==2 and audio.getnchannels()==2,'Unexpected PCM capture format')
            pcm=audio.readframes(audio.getnframes());samples=struct.unpack('<'+'h'*(len(pcm)//2),pcm)
        result=dict(name=label,seats=seats,button=button,before=state,motion=motion,after=snapshot(r),events=events,
                    capture=response,pcm_peak=max(map(abs,samples)),pcm_nonzero=sum(value!=0 for value in samples))
        report['cases'].append(result)
        if button==0x4000:
            require(all(motion['players'][seat]['y']<state['players'][seat]['y']-4 for seat in seats),
                    'Native jump input did not lift the requested hero from the ground')
        require(result['pcm_nonzero']>0,'SPU emulation produced silent PCM')
        private=[event for event in events if event['kind']=='KEYON' and 24<=event['v']<48]
        hardware=[event for event in events if event['kind']=='KEYON' and event['v']<24]
        if 1 in seats:require(private,'Native P2 input did not start an independent private voice')
        if 0 in seats:require(hardware,'Native P1 input did not retain hardware voices')
        character=result['before']['players'][1]['character']
        if 1 in seats and (button==0x4000 or (button==0x8000 and character==0)):
            _,head=asset(dat,(88,90)[character],10)
            # Original35630..3563C requests group1/record1 on jump entry.
            # Original4F348 requests group1/record8 for X's deferred buster.
            sound=1 if button==0x4000 else 8
            record=head[8+sound*4:8+(sound+1)*4]
            program=record[1]&127;tone=record[2]>>4;voice=record[3]&31;count=(record[3]>>5&3)+1
            original=next(case for case in sdk['cases'] if case['character']==character and case['group']==1)
            expected=[]
            for index in range(count):
                native=next(row for row in original['tones'] if row['program']==program and row['tone']==tone+index)
                regs=native['regs'];slot=24+voice+index
                expected.append(dict(voice=slot,pitch=regs[2],addr=(regs[3]&~1)*8,adsr=regs[4:6]))
                require(any(event['v']==slot and int(event['pitch'],16)==regs[2] and
                            int(event['addr'],16)==(regs[3]&~1)*8 and
                            [int(event['adsr_lo'],16),int(event['adsr_hi'],16)]==regs[4:6] for event in private),
                        f'P2 group1/record{sound} tone differs from its original character bank: {expected[-1]}')
            result['original_sound_record']=sound
            result['original_expected_tones']=expected
            if button==0x4000:result['original_expected_jump_tones']=expected
        if len(seats)==2:
            # Observe matching hardware/private slots with intersecting event
            # lifetimes. End-of-sample/key-off terminates an interval; a later
            # KEYON cannot count as the earlier sound still playing.
            intervals=[];active={}
            for event in events:
                slot=event['v']
                if event['kind']=='KEYON':
                    if slot in active:intervals.append((slot,active[slot],event['frame']))
                    active[slot]=event['frame']
                elif event['kind'] in ('KEYOFF','END_STOP') and slot in active:
                    intervals.append((slot,active.pop(slot),event['frame']))
            intervals.extend((slot,start,end) for slot,start in active.items())
            overlaps=[dict(hardware=slot,private=p,begin=max(a,c),end=min(b,d))
                      for slot,a,b in intervals for p,c,d in intervals
                      if slot<24 and 24<=p<48 and (button!=0x4000 or p==slot+24) and max(a,c)<min(b,d)]
            result['overlapping_slot_intervals']=overlaps
            require(overlaps,'Simultaneous sounds did not overlap in independent copies of an original voice slot')
        require(not r.state()['failed'],'Co-op failed during native audio check')
        return result
    try:
        r.resume_native_pause();r.release();r.fixture(11,hp=3);r.fixture(12,hp=3)
        previous=counter(r,'mmx5.coop.enrolled');r.load_location(1,0,1);wait_ready(r,previous)
        for label,seats,button in [('p1-jump',[0],0x4000),('p2-jump',[1],0x4000),
                                   ('both-jump',[0,1],0x4000),('p1-attack',[0],0x8000),
                                   ('p2-attack',[1],0x8000),('both-attack',[0,1],0x8000)]:
            capture(label,seats,button)
        for seat in (1,0):
            wait(grounded,'Heroes did not settle before native menu')
            r.tap(8,.10,seat);wait(lambda:r.read(PLAY+1,1)==b'\x02','Native menu did not open')
            r.resume_native_pause();capture(f'after-p{seat+1}-menu-jump',[0,1])
        report['counters']=r.request('mod_counters')['counters']
        require(not any(row['count'] for row in report['counters'] if row['name'] in
                        ('mmx5.coop.audio-key-failed','mmx5.coop.audio-bank-open-failed','mmx5.coop.private-voice-failed')),
                'Native audio routing reported a failure')
        r.screenshot(args.out/'after.png');report['success']=True
        print('PASS original jump identity, independent voice overlap, attacks and both menu returns',flush=True)
    except Exception:
        report['error']=traceback.format_exc();report['failure']=snapshot(r)
        r.screenshot(args.out/'failure.png');raise
    finally:
        try:
            r.release();r.resume_native_pause();r.fixture(12,hp=0)
            if r.read(PLAY+1,1)==b'\x00':r.tap(8,.05)
        finally:(args.out/'report.json').write_text(json.dumps(report,indent=2)+'\n')


if __name__=='__main__':main()

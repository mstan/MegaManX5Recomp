"""Native death/survivor/retry checks on an owned, already booted QA process.

Fatal fixtures are acknowledged at the original world boundary; all death
animation, effects, native retry consumption and checkpoint reload stay native.
They do not constitute a stage playthrough or human acceptance.
"""
import argparse
import json
from pathlib import Path
import struct
import time
import traceback
from coop_debug import Runtime, PLAY


def require(condition, message):
    if not condition:
        raise AssertionError(message)


def wait_for(runtime, predicate, timeout=12, dialogue=False):
    deadline = time.monotonic()+timeout
    while not predicate():
        require(time.monotonic()<deadline, 'Native lifecycle boundary timed out')
        if dialogue and runtime.read(PLAY+0x1C, 1)!=b'\x00':
            runtime.tap(0x4000, .05)
        time.sleep(.025)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, required=True)
    parser.add_argument('--out', type=Path, required=True)
    parser.add_argument('--contact-protection',action='store_true',
                        help='Hold native contact grace for survivor setup; does not qualify damage')
    parser.add_argument('--retry-budget',action='store_true',
                        help='Qualification input chord seeds three retries; does not qualify Game Over')
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    runtime = Runtime(args.port)
    report = dict(human_validation=False, cases=[])
    try:
        runtime.release()
        runtime.resume_native_pause()
        if args.retry_budget:
            before=int.from_bytes(runtime.read(runtime.diagnostic()+0x80,4),'little')
            runtime.tap(6|(13<<8),.12)
            wait_for(runtime,lambda:int.from_bytes(runtime.read(runtime.diagnostic()+0x80,4),'little')==before+1)
            require(runtime.read(PLAY+0x45,1)==b'\x03','Qualification retry budget was not accepted')
            report['bypassed_prerequisites']=['three shared retries seeded through qualification input; Game Over unverified']
        if args.contact_protection:
            runtime.fixture(12,hp=3)
            require(int.from_bytes(runtime.read(runtime.diagnostic()+0x64,4),'little')==1,
                    'Native contact-protection fixture unavailable')
            report.setdefault('bypassed_prerequisites',[]).append('native incoming-contact grace held; fatal HP entry remains explicit')
        for first_dead in (0, 1):
            wait_for(runtime, lambda: runtime.state()['enrolled'] and runtime.lifecycle()[:2]==(0,0),
                     timeout=20, dialogue=True)
            lives = runtime.read(PLAY+0x45, 1)[0]
            require(lives>0, 'Fixture needs a native retry')
            survivor = first_dead^1
            runtime.fixture(1, first_dead)
            wait_for(runtime, lambda: runtime.lifecycle()[first_dead]==5)
            require(runtime.lifecycle()[survivor]==0, 'Surviving partner was killed')
            require(runtime.read(PLAY+0x45, 1)[0]==lives, 'One death consumed a shared retry')
            runtime.screenshot(args.out/f'p{first_dead+1}-fallen.png')
            before = runtime.players()
            clock_before = runtime.state()
            runtime.input(0x20, survivor)
            time.sleep(.15)
            runtime.input(0, survivor)
            after = runtime.players()
            clock_after = runtime.state()
            require(struct.unpack_from('<i', after[survivor], 8)[0]>
                    struct.unpack_from('<i', before[survivor], 8)[0], 'Survivor lost independent controls')
            require(clock_after['world_ticks']-clock_before['world_ticks']>5, 'Survivor world stopped')
            require(clock_after['world_ticks']-clock_before['world_ticks']==
                    clock_after['native_timer']-clock_before['native_timer'], 'Survivor world advanced twice')
            runtime.input(1, first_dead)
            time.sleep(.08)
            runtime.input(0, first_dead)
            require(runtime.lifecycle()[first_dead]==5, 'Fallen actor rejoined on Select')
            runtime.screenshot(args.out/f'p{survivor+1}-survivor.png')
            runtime.fixture(1, survivor)
            wait_for(runtime, lambda: runtime.read(PLAY+0x45, 1)[0]!=lives)
            require(runtime.read(PLAY+0x45, 1)[0]==lives-1, 'Team wipe did not consume exactly one retry')
            wait_for(runtime, lambda: runtime.state()['enrolled'] and runtime.lifecycle()[:2]==(0,0),
                     timeout=20, dialogue=True)
            # Enrollment/lifecycle reset precedes native health-fill and input
            # release. Observe that boundary before asserting a live respawn.
            wait_for(runtime,lambda:not runtime.read(PLAY+0x1C,1)[0] and
                     all(actor[4]==1 and actor[5]>=2 and actor[0x5C]&0x7f
                         for actor in runtime.players()),timeout=20,dialogue=True)
            require(all(actor[0x5C]&0x7f for actor in runtime.players()), 'Team failed to respawn')
            runtime.screenshot(args.out/f'team-respawn-{first_dead}.png')
            report['cases'].append(dict(first_dead=first_dead, lives_before=lives,
                                        lives_after=runtime.read(PLAY+0x45, 1)[0],
                                        before=clock_before, after=clock_after))
            print('PASS native death order', first_dead, 'retry', lives, '->', lives-1, flush=True)
        report['success'] = True
    except Exception:
        report['success']=False;report['error']=traceback.format_exc()
        report['failure']=dict(state=runtime.state(),life=runtime.lifecycle(),
                               play=runtime.read(PLAY,0x1C0).hex(),
                               players=[body.hex() for body in runtime.players()])
        runtime.screenshot(args.out/'failure.png')
        raise
    finally:
        try:
            if args.contact_protection:
                runtime.resume_native_pause();runtime.fixture(12,hp=0)
            runtime.release()
            if runtime.lifecycle()[0]==0 and runtime.read(PLAY+1,1)==b'\x00':runtime.tap(8,.05)
        except Exception:
            report['cleanup_error']=traceback.format_exc();report['success']=False
            raise
        finally:
            (args.out/'report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')


if __name__ == '__main__':
    main()

"""Controlled original-item/tank/teleport ownership checks on a private X5 run.

The debug-only native-boundary fixtures are offline-only. They do not establish
campaign progression, ordinary room traversal, or human acceptance.
"""
import argparse
import json
from pathlib import Path
import time
from coop_debug import Runtime, PLAY
from test_mmx5_coop_lifecycle_runtime import require, wait_for


def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port',type=int,required=True)
    parser.add_argument('--out',type=Path,required=True)
    parser.add_argument('--contact-protection',action='store_true',
                        help='Private setup: native contact grace; does not qualify incoming damage or mounts')
    args=parser.parse_args()
    args.out.mkdir(parents=True,exist_ok=True)
    r=Runtime(args.port)
    report=dict(success=False,human_validation=False)
    try:
        r.release()
        r.resume_native_pause()
        require(r.lifecycle()[:2]==(0,0),'Start with both native actors alive')
        if args.contact_protection:
            r.fixture(11,hp=0);r.fixture(12,hp=3)
            require(int.from_bytes(r.read(r.diagnostic()+0x64,4),'little')==1,
                    'Native contact-protection fixture unavailable')
            report['bypassed_prerequisites']=['native incoming-contact grace held for pickup/menu setup']
        r.input(0x20,1)
        time.sleep(.35)
        r.input(0,1)
        r.fixture(2,0,hp=13,ammo=0x90)
        r.fixture(2,1,hp=11,ammo=0x90)
        r.fixture(3,1,hp=0x21)
        wait_for(r,lambda:r.players()[1][0x5C]==27)
        require(r.players()[0][0x5C]==13,'P2 pickup healed P1')
        require(r.read(r.diagnostic()+0x74,1)==b'\x02','P2 did not own the native pickup')
        report['pickup_hp']=[b[0x5C] for b in r.players()]
        r.screenshot(args.out/'pickup.png')
        r.fixture(2,1,hp=11,ammo=0x90)
        r.fixture(4,hp=16)
        r.tap(8,.05,1)
        wait_for(r,lambda:r.read(PLAY+1,1)==b'\x02')
        wait_for(r,lambda:r.read(0x800D4F44,1)==b'\x01')  # Native opening fade completed.
        before=r.state()
        r.tap(8,.05,0)
        require(r.read(PLAY+1,1)==b'\x02','Other seat closed the owner menu')
        require(r.state()['world_ticks']==before['world_ticks'],'World ran in the pause menu')
        r.tap(0x10,.05,1)  # Native top row.
        r.tap(0x20,.05,1)  # Native first E tank.
        r.screenshot(args.out/'tank-menu.png')
        r.tap(0x4000,.05,1)
        wait_for(r,lambda:r.players()[1][0x5C]==27)
        require(r.players()[0][0x5C]==13,'P2 tank healed P1')
        require(r.read(PLAY+0x76,1)[0]&127==0,'Shared tank was not spent once')
        report['tank_hp']=[b[0x5C] for b in r.players()]
        r.tap(8,.05,1)
        wait_for(r,lambda:r.read(PLAY+1,1)==b'\x00')
        time.sleep(.2)
        r.input(1,1)
        wait_for(r,lambda:r.lifecycle()[1]==2)
        r.input(0,1)
        # Enemies can damage an alive actor during the 90-update hold. Compare
        # the protected absence boundary, rather than an earlier HP sample.
        absent=r.players()[1]
        inventory=absent[0xA8:0xC8]
        health=absent[0x5C]
        time.sleep(.1)
        r.screenshot(args.out/'absent.png')
        r.tap(1,.06,1)
        wait_for(r,lambda:r.lifecycle()[1]==0)
        require(r.players()[1][0x5C]==health,'Return refilled/lost P2 health')
        require(r.players()[1][0xA8:0xC8]==inventory,'Return changed per-player ammo')
        r.screenshot(args.out/'returned.png')
        report['return_inventory']=inventory.hex()
        report['return_hp']=health
        r.fixture(5)
        wait_for(r,lambda:r.read(r.diagnostic()+0x4D,1)==b'\x03')
        require(r.players()[1][3]==0,'Passenger did not finish the native departure')
        r.screenshot(args.out/'script-passenger-hidden.png')
        r.fixture(6)
        wait_for(r,lambda:r.read(r.diagnostic()+0x4C,2)==b'\x00\x00')
        require(r.players()[1][0x5C]==health and r.players()[1][0xA8:0xC8]==inventory,
                'Scene transport changed P2 resources')
        r.screenshot(args.out/'script-returned.png')
        report['success']=True
        print('PASS native pickup, shared tank, return inventory and script passenger',flush=True)
    finally:
        if args.contact_protection:r.fixture(12,hp=0)
        r.release()
        if r.lifecycle()[0]==0 and r.read(PLAY+1,1)==b'\x00':
            r.tap(8,.05)
        (args.out/'report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')


if __name__=='__main__':
    main()

"""Verify the actual x64 follow, queue-log and accessory handlers in Release.

Usage: python verify_pet_follow_runtime_release.py <Release directory>
Requires capstone. The existing PE reader resolves incremental-link thunks.
"""
import bisect
from pathlib import Path
import runpy
import sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM, CS_OP_REG

reader=Path(__file__).with_name('verify_scenario_lfg_release.py')
sys.argv[0]=str(reader)
n=runpy.run_path(str(reader))
decoder=Cs(CS_ARCH_X86,CS_MODE_64);decoder.detail=True


def instructions(prefix):
    address=n['symbol'](prefix)
    index=bisect.bisect_right(n['starts'],address-n['base'])-1
    begin,end,unwind=n['functions'][index]
    assert begin<=address-n['base']<end, prefix
    origin=n['root'](begin,unwind)
    for tail_begin,tail_end,tail_unwind in n['functions'][index+1:]:
        if tail_begin!=end or n['root'](tail_begin,tail_unwind)!=origin:break
        end=tail_end
    at=n['offset'](address)
    return list(decoder.disasm(n['binary'][at:at+n['base']+end-address],address))


for prefix in ('?GetVelocity@@','?PredictPosition@@','?EnableWalking@?$FollowMovementGenerator@VCreature'):
    body=instructions(prefix)
    # Current Win64 MoveSpline layout: flags at 0x68, retained walk at 0xac.
    # Both tests must be present in each formerly stale/omitted walk path.
    done=any(i.mnemonic=='test' and len(i.operands)==2 and i.operands[0].type==CS_OP_MEM
             and i.operands[0].mem.disp==0x68 and i.operands[1].type==CS_OP_IMM and i.operands[1].imm==0x20 for i in body)
    walk=any(i.mnemonic=='cmp' and i.operands[0].type==CS_OP_MEM and i.operands[0].mem.disp==0xac for i in body)
    assert done and walk, f'Missing active-spline walk guard in linked {prefix}'

install='?InstallAccessory@Vehicle@@'
assert n['symbol']('?EnterVehicle@Unit@@') in n['calls'](install), 'Accessory does not use native ride aura'
assert n['symbol']('?HandleSpellClick@Unit@@') not in n['calls'](install), 'Accessory still invokes player click actions'
queue=instructions('?RemoveFromGroupQueue@LFGMgr@lfg@@')
levels={i.operands[1].imm for i in queue if i.mnemonic=='mov' and len(i.operands)==2
        and i.operands[0].type==CS_OP_REG and i.reg_name(i.operands[0].reg)=='r8d' and i.operands[1].type==CS_OP_IMM}
assert 2 in levels and 5 not in levels, 'Group cleanup still emits ERROR instead of DEBUG'
print('PASS: linked PE has active-spline guards in all three follow paths, accessory -> EnterVehicle without spellclick, and DEBUG-only group cleanup')

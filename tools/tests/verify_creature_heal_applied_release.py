"""Inspect the linked post-heal notification and preserved CreatureAI slots."""
import argparse
from collections import defaultdict
from pathlib import Path
import runpy
import struct
import sys
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

parser=argparse.ArgumentParser()
parser.add_argument('release',type=Path)
parser.add_argument('--previous',type=Path)
args=parser.parse_args()
reader=Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')
def load(directory):
    saved=sys.argv
    try:
        sys.argv=[str(reader),str(directory)]
        return runpy.run_path(str(reader))
    finally:
        sys.argv=saved
n=load(args.release)
body=n['instructions']('?DealHeal@Unit@@')
def sites(method):
    return [i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm)==n['symbol'](method)]
received=[i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0xb0]
done=[i for i,x in enumerate(body) if x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0xb8]
applied,=sites('?ModifyHealth@Unit@@')
notified,=sites('?NotifyHealApplied@Creature@@')
assert len(received)==len(done)==1 and received[0]<done[0]<applied<notified
assert [(x.mnemonic,x.op_str) for x in body[notified-3:notified]]==[
    ('mov','r9d, eax'),('mov','r8d, dword ptr [rbp - 0x59]'),('mov','rdx, rdi')]
assert body[applied-5].mnemonic=='je' and body[applied-5].operands[0].imm==body[notified+1].address
assert any(x.mnemonic=='cmp' and x.op_str=='byte ptr [rsi + 0x16], 3' for x in body[applied:notified])
assert body[notified-4].mnemonic=='je' and body[notified-4].operands[0].imm==body[notified+1].address
body=n['instructions']('?NotifyHealApplied@Creature@@')
calls=[i for i,x in enumerate(body) if x.mnemonic=='call']
assert len(calls)==2
assert all(body[i].operands[0].type==CS_OP_MEM and body[i].operands[0].mem.disp==0x288 for i in calls)
def index(mnemonic,operands):
    values=[i for i,x in enumerate(body) if x.mnemonic==mnemonic and x.op_str==operands]
    assert len(values)==1,(mnemonic,operands,values)
    return values[0]
assert body[0].mnemonic=='test' and body[0].op_str=='r8d, r8d' and body[1].mnemonic=='je'
negative=index('test','r9d, r9d');assert body[negative+1].mnemonic=='js'
lock=index('lea','rbx, [rcx + 0x3a9d]')
prior=index('movzx','r14d, byte ptr [rbx]')
claimed=index('mov','byte ptr [rbx], 1')
active=index('mov','rsi, qword ptr [rcx + 0x9c0]')
suspended=index('mov','rdi, qword ptr [rcx + 0x9c8]')
alias=index('cmp','rdi, rsi')
restored=index('mov','byte ptr [rbx], r14b')
assert negative<lock<prior<claimed<active<suspended<calls[0]<alias<calls[1]<restored
assert body[alias+1].mnemonic=='je' and body[alias+1].operands[0].imm==body[restored].address
assert body[negative+1].operands[0].imm>body[restored].address
assert n['instructions']('?OnHealApplied@CreatureAI@@')[0].mnemonic=='ret'

def table(native):
    candidates=[]
    for x in native['instructions']('?GetAI@npc_sr_orik_cato_script@@'):
        for operand in x.operands:
            if x.mnemonic=='lea' and operand.type==CS_OP_MEM and operand.mem.base==X86_REG_RIP:
                at=native['offset'](x.address+x.size+operand.mem.disp)
                if struct.unpack_from('<Q',native['binary'],at+0xa8)[0]==native['symbol']('?DamageTakenAfterScaling@npc_sr_orik_cato_scriptAI@'):
                    candidates.append(at)
    assert len(candidates)==1
    return candidates[0]
current=table(n)
assert struct.unpack_from('<Q',n['binary'],current+0x288)[0]==n['symbol']('?OnHealApplied@CreatureAI@@')
assert struct.unpack_from('<Q',n['binary'],current+0x278)[0]==n['symbol']('?MoveInLineOfSight@CreatureAI@@')
assert struct.unpack_from('<Q',n['binary'],current+0x280)[0]==n['symbol']('?_EnterEvadeMode@CreatureAI@@')
if args.previous:
    old=load(args.previous);previous=table(old)
    def identities(native):
        result=defaultdict(set)
        for name,address in native['symbols'].items():result[native['resolve'](address)].add(name)
        return result
    before,after=identities(old),identities(n)
    for slot in range(0,0x288,8):
        a=old['resolve'](struct.unpack_from('<Q',old['binary'],previous+slot)[0])
        b=n['resolve'](struct.unpack_from('<Q',n['binary'],current+slot)[0])
        assert before[a]&after[b],(hex(slot),before[a],after[b])
    print('PASS all81 pre-existing selected CreatureAI slots retain their native function identities against previous Release; new empty hook appended at0x288')
print('PASS native HealReceived->HealDone->one ModifyHealth->Creature-only notification with final amount/gain; zero/negative guard, current+suspended snapshots/alias check, prior AI lock restoration and empty default. Client progression remains separate')

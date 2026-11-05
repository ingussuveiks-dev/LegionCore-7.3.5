"""Selected native Devourer pickup binds a full Player GUID and quest attempt."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values
from merlis_intro_pe import reachable

p=runpy.run_path(str(Path(__file__).with_name('verify_devourer_pickup_release.py')))
n,ins=p['n'],p['instructions']
method=lambda name:'?'+name+'@AI@go_art_devourer@@'
pairs=lambda rows:{(x.mnemonic,x.op_str) for x in rows}
def refs(rows):
    return {x.address+x.size+o.mem.disp for x in rows for o in x.operands
            if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP}
factory=ins('?GetAI@go_art_devourer@@')
callbacks={1:'UpdateAI',6:'SetGUID',7:'GetGUID',9:'GossipHello',16:'GetData'}
tables=[]
for at in refs(factory):
    rows=struct.unpack_from('<22Q',n['binary'],n['offset'](at))
    if all(n['resolve'](rows[i])==n['symbol'](method(name)) for i,name in callbacks.items()):tables.append(at)
assert len(tables)==1
assert {('mov','ecx, 0x38'),('mov','qword ptr [rdi + 0x20], rax'),('mov','qword ptr [rdi + 0x28], rax'),('mov','word ptr [rdi + 0x34], ax')} <= pairs(factory)
owner,begin,update,cancel=[ins(method(x)) for x in ('ClaimantPlayer','SetGUID','UpdateAI','Cancel')]
lookup,=sites(n,owner,'?GetPlayer@ObjectAccessor@@')
identity,=sites(n,owner,'??9ObjectGuid@@')
receipt,=sites(n,owner,'?GetQuestCancellationReceipt@Player@@')
assert sites(n,owner,'?IsInWorld@Object@@') and lookup<identity<receipt and 41924 in values(owner)
denied,=[x.address for x in owner if (x.mnemonic,x.op_str)==('mov','bpl, 1')]
assert owner[identity+2].mnemonic=='jne' and owner[identity+2].operands[0].imm==denied
assert {('movups','xmm0, xmmword ptr [rsi + 0x10]'),('mov','rax, qword ptr [rsi + 0x20]'),('movzx','eax, byte ptr [rax]'),('cmp','qword ptr [rax], rcx'),('xor','bpl, bpl'),('test','bpl, bpl'),('cmovne','rdi, r15'),('xor','r15d, r15d'),('mov','rax, rdi')} <= pairs(owner)
identity,=sites(n,begin,'??9ObjectGuid@@')
receipt,=sites(n,begin,'?GetQuestCancellationReceipt@Player@@')
cast,=sites(n,begin,'?CastSpell@Unit@@')
fresh,=sites(n,begin,method('ClaimantPlayer'))
assert sites(n,begin,'?GetPlayer@ObjectAccessor@@')[0]<identity<receipt<cast<fresh
assert begin[identity+2].mnemonic=='jne' and begin[cast].address not in reachable(begin,begin[identity+2].operands[0].imm)
assert {209816,41924,128476} <= values(begin)
assert {('lea','rbx, [rcx + 0x35]'),('cmp','byte ptr [rbx], 0'),('mov','byte ptr [rbx], 1'),('mov','byte ptr [rbx], 0'),('movups','xmmword ptr [rdi + 0x10], xmm0'),('lea','rcx, [rdi + 0x20]')} <= pairs(begin)
assert len(sites(n,begin,method('Cancel')))==2 and len(sites(n,begin,'?CanTake@DevourerAcquisition@@'))==2
assert begin[cast+4].mnemonic=='je' and begin[cast+4].operands[0].imm==begin[fresh].address
guard,=[i for i,x in enumerate(update) if (x.mnemonic,x.op_str)==('cmp','byte ptr [rcx + 0x35], 0')]
award,=sites(n,update,'?UpdateAchievementCriteria@Player@@')
owner_call,=sites(n,update,method('ClaimantPlayer'))
assert update[guard+1].mnemonic=='jne' and update[award].address not in reachable(update,update[guard+1].operands[0].imm)
gate,=sites(n,update,'?CanTake@DevourerAcquisition@@')
assert owner_call<gate<award and update[gate+2].mnemonic=='je'
assert update[award].address not in reachable(update,update[gate+2].operands[0].imm)
latch,=[i for i,x in enumerate(update) if (x.mnemonic,x.op_str)==('mov','byte ptr [rdi + 0x34], 1')]
clear,=sites(n,update,'?Clear@ObjectGuid@@')
assert owner_call<latch<clear<award
assert {50301,128476,128479,15000,1948,105843,1000} <= values(update)
assert sites(n,update,'?GetChildItem@Item@@') and sites(n,update,'??8ObjectGuid@@')
assert sites(n,cancel,'?Clear@ObjectGuid@@') and {('mov','qword ptr [rbx + 0x20], rax'),('mov','qword ptr [rbx + 0x28], rax')} <= pairs(cancel)
for body in (owner,begin,update,cancel):assert not sites(n,body,'?AddItem@Player@@')
print('PASS selected registered Devourer claim: actual0x38 AI/vtable, full128 Player identity before209816 and delayed50301, current native quest receipt/cancel flag, post-cast refresh and starting guard before completion/reentry; canceled claim clears owner+receipt, exact native child pair before once-only50301, retained15s timeout/1948/model cleanup/no item grant. Native seeded equipment and actual client remain separate.')

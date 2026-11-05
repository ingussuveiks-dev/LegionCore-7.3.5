"""Registered native cage/cache admission, timed cast receipt and original door."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values, floats
from merlis_intro_pe import reachable

p=runpy.run_path(str(Path(__file__).with_name('verify_devourer_door_release.py')))
n,ins=p['n'],p['ins']
method=lambda name:'?'+name+'@spell_devourer_escape_SpellScript@spell_devourer_escape@@'
pairs=lambda body:{(x.mnemonic,x.op_str) for x in body}
def refs(body):
    return {x.address+x.size+o.mem.disp for x in body for o in x.operands if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP}
factory=ins('?GetSpellScript@spell_devourer_escape@@')
assert any(n['symbol']('?GetSpellScript@spell_devourer_escape@@') in struct.unpack_from('<8Q',n['binary'],n['offset'](at)) for at in refs(ins('?AddSC_Devourer@@')))
assert any(struct.unpack_from('<10Q',n['binary'],n['offset'](at))[5]==n['symbol'](method('Register')) and n['symbol'](method('Load')) in struct.unpack_from('<10Q',n['binary'],n['offset'](at)) for at in refs(factory))
assert ('mov','ecx, 0x228') in pairs(factory)
load,check,register,context=[ins(x) for x in (method('Load'),method('CheckEscape'),method('Register'),'?CanUse@DevourerEscape@@')]
assert n['symbol'](method('CheckEscape')) in refs(register) and ('lea','rcx, [rbx + 0xa8]') in pairs(register)
assert {41924,211689,249809,248992} <= values(load) and 10.0 in floats(n,load)
assert sites(n,load,'?GetQuestCancellationReceipt@Player@@') and sites(n,load,'?FindNearestGameObject@WorldObject@@')
assert {('movups','xmmword ptr [rsi + 0x1f8], xmm0'),('movups','xmmword ptr [rsi + 0x208], xmm0'),('lea','rcx, [rsi + 0x218]'),('mov','al, 1')} <= pairs(load)
recipient,=sites(n,check,'??9ObjectGuid@@');receipt,=sites(n,check,'?GetQuestCancellationReceipt@Player@@');source,=sites(n,check,'?GetGameObject@ObjectAccessor@@');identity,=sites(n,check,'??8ObjectGuid@@');admit,=sites(n,check,'?CanUse@DevourerEscape@@')
assert recipient<receipt<source<identity<admit
assert {41924,211689,249809,248992,81} <= values(check)
assert {('mov','rax, qword ptr [r14 + 0x218]'),('movzx','eax, byte ptr [rax]'),('cmp','qword ptr [rcx], rax'),('lea','rdx, [r14 + 0x1f8]'),('lea','rdx, [r14 + 0x208]')} <= pairs(check)
assert check[recipient+2].mnemonic=='jne' and check[identity+2].mnemonic=='je'
invalid,=[i for i,x in enumerate(check) if (x.mnemonic,x.op_str)==('mov','r15b, 1')]
gate,=[i for i,x in enumerate(check) if (x.mnemonic,x.op_str)==('test','r15b, r15b')]
# MSVC merges the receipt temporaries' shared_ptr cleanup before testing this
# flag. A raw CFG walk alone would incorrectly take its impossible flag0 edge.
assert check[recipient+2].operands[0].imm==check[invalid].address and invalid<gate<source
assert all(not (x.operands and x.op_str.startswith('r15b,') and x.mnemonic!='test') for x in check[invalid+1:gate])
assert check[gate+1].mnemonic=='je' and check[gate+2].op_str=='eax, 0x51' and check[gate+3].mnemonic=='jmp'
assert check[admit].address not in reachable(check,check[gate+3].operands[0].imm)
assert check[admit].address not in reachable(check,check[identity+2].operands[0].imm)
assert {249809,248992,1607,41924,136802,1078,2,3,4} <= values(context)
for target,count in [('?IsInWorld@Object@@',2),('?IsPreDelete@Object@@',2),('?IsQuestObjectExpired@WorldObject@@',2),('?CanTargetQuestObject@WorldObject@@',2),('?GetQuestStatus@Player@@',2),('?HasItemCount@Player@@',1),('?GetQuestRewardStatus@Player@@',1),('?GetInstanceScript@WorldObject@@',1),('?CanContact@Player@@',1),('?InSamePhase@WorldObject@@',1),('?InSamePhaseId@WorldObject@@Q',1),('?IsWithinLOSInMap@WorldObject@@',1),('?canSeeOrDetect@WorldObject@@',1),('?GetInteractionDistance@GameObject@@',1),('?IsWithinDistInMap@WorldObject@@',1)]:
    assert len(sites(n,context,target))==count,target
use,hello=[ins('?'+x+'@go_devourer_miscAI@go_devourer_misc@@') for x in ('GossipUse','GossipHello')]
assert sites(n,use,'?CanUse@DevourerEscape@@') and ('xor','al, 1') in pairs(use)
for target in ('?UpdateAchievementCriteria@Player@@','?RemoveAura@Unit@@','?SetPhaseMask@GameObject@@'):
    assert not sites(n,hello,target)
# Native noninstant cast rechecks CheckCast(false), whose first call dispatches
# the selected registered handler. The actual timed tests cover its refusal.
cast=ins('?cast@Spell@@');native=ins('?CheckCast@Spell@@')
# Release inlines CallScriptCheckCastHandlers at CheckCast's entry. Follow its
# actual registered-handler loop, before the later inline switch table bytes.
assert sites(n,cast,'?CheckCast@Spell@@')
prepare,=sites(n,native[:64],'?_PrepareScriptCall@SpellScript@@')
invoke,=sites(n,native[:64],'?Call@CheckCastHandler@SpellScript@@')
assert prepare<invoke and {('mov','r14d, 0xf'),('mov','rdi, qword ptr [rbx + 0xb0]'),('mov','rbx, qword ptr [rbx + 0xa8]'),('cmove','esi, eax')} <= pairs(native[:64])
for body in (load,check,context,register):
    for target in ('?UpdateAchievementCriteria@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?SetCurrentStep@Scenario@@','?CastSpell@Unit@@'):
        assert not sites(n,body,target)
print('PASS registered211689/211687 escape SpellScript0x228: actual Load/full128 recipient+source/current41924 cancellation receipt, native CheckCast on start and timed completion, qualified real Rogue/key/1078-stage2/GO/range/phase/LOS/scope admission, no Hello progress or manual aura/credit/item/cast replacement; original native two-second reclaim and10ms aura removals plus once-only50536 door retained. Seeded native endpoints/client traversal separate.')

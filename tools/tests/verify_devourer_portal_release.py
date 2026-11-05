"""Registered Valzuun attempt/recipient and native owned portal use in linked PE."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values, floats

p=runpy.run_path(str(Path(__file__).with_name('verify_devourer_clones_release.py')))
n,ins=p['n'],p['ins']
pairs=lambda rows:{(x.mnemonic,x.op_str) for x in rows}
refs=lambda rows:{x.address+x.size+o.mem.disp for x in rows for o in x.operands if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP}
method=lambda name:'?'+name+'@npc_valzuun@@'
text=(n['args'].release/'worldserver.map').read_text(errors='replace')
table,=[int(x,16) for x in re.findall(r'\s+\w+:\w+\s+\?\?_7npc_valzuun@@6BScriptObject@@@\s+([0-9a-fA-F]{16})',text)]
assert table in refs(ins('?AddSC_Devourer@@'))
slots=struct.unpack_from('<12Q',n['binary'],n['offset'](table))
assert slots[2]==n['symbol'](method('OnGossipHello')) and slots[5]==n['symbol'](method('OnQuestAccept'))
accept,hello,start,owner,active=[ins(method(x)) for x in ('OnQuestAccept','OnGossipHello','Start','Owner','Active')]
assert 41924 in values(accept) and sites(n,accept,method('Start')) and sites(n,hello,method('Start'))
assert ('xor','al, al') in pairs(hello)
assert {41924,3,1,4}<=values(active)
for target in ('?IsInWorld@Object@@','?IsPreDelete@Object@@','?CanContact@Player@@','?GetQuestStatus@Player@@','?GetQuestRewardStatus@Player@@'):
    assert sites(n,active,target),target
assert {105464,105488,248901,1502,41924,1078,30000,1000,4000,11500}<=values(start)
assert {5.0,100.0}<=floats(n,start)
summon,=sites(n,start,'?SummonCreature@WorldObject@@')
bind,=sites(n,start,'?BindQuestObject@Player@@')
ai,=sites(n,start,'?AIM_Initialize@Creature@@')
assert summon<bind<ai<sites(n,start,method('Owner'))[0]
assert {('movdqa','xmmword ptr [rsp + 0xe0], xmm6'),('mov','dword ptr [rsp + 0x38], 0x7530')}<=pairs(start[:summon])
for target in ('?GetCreatureListWithEntryInGrid@WorldObject@@','?GetGameObjectListWithEntryInGrid@WorldObject@@','?GetQuestCancellationReceipt@Player@@','?IsWithinLOSInMap@WorldObject@@'):
    assert sites(n,start,target),target
lookup,=sites(n,owner,'?GetPlayer@ObjectAccessor@@')
identity=sites(n,owner,'??8ObjectGuid@@');assert len(identity)==2 and lookup<identity[0]<identity[1]<sites(n,owner,'?GetQuestCancellationReceipt@Player@@')[0]
assert {105488,1502,41924}<=values(owner)
assert {('movups','xmm0, xmmword ptr [r12]'),('movzx','eax, byte ptr [rax]'),('cmp','qword ptr [rax], rcx')}<=pairs(owner)
dedup=ins('??R<lambda_1>@?1??Start@npc_valzuun@@')
assert sites(n,dedup,'??8ObjectGuid@@') and sites(n,dedup,'?CancellationReceipt@QuestObjectScope@@')
observer=ins('?SpellHitTarget@OpeningAI@npc_valzuun@@')
assert 209056 in values(observer) and sites(n,observer,'??8ObjectGuid@@') and ('mov','byte ptr [rax], 1') in pairs(observer)
observer_table,=[int(x,16) for x in re.findall(r'\s+\w+:\w+\s+\?\?_7OpeningAI@npc_valzuun@@6B@\s+([0-9a-fA-F]{16})',text)]
assert observer_table in refs(start)
assert n['symbol']('?SpellHitTarget@OpeningAI@npc_valzuun@@') in struct.unpack_from('<82Q',n['binary'],n['offset'](observer_table))
cast=ins('?_Do_call@?$_Func_impl_no_alloc@V<lambda_3>@?1??Start@npc_valzuun@@')
fresh,=sites(n,cast,method('Owner'));native,=sites(n,cast,'?CastSpell@Unit@@')
assert fresh<native and 209056 in values(cast) and {('mov','rdx, rdi'),('mov','rdi, rax'),('sete','cl'),('mov','byte ptr [rax], cl')}<=pairs(cast)
publish=ins('??R<lambda_4>@?1??Start@npc_valzuun@@')
fresh,=sites(n,publish,method('Owner'));credit,=sites(n,publish,'?GetQuestObjectiveData@Player@@');portal,=sites(n,publish,'?SummonGameObject@WorldObject@@')
assert fresh<credit<portal and {41924,105435,248901,300,6000}<=values(publish)
assert {('mov','rax, qword ptr [rdi + 0x28]'),('mov','rax, qword ptr [rdi + 0x38]'),('cmp','byte ptr [rax], 0')}<=pairs(publish[:credit])
assert len([x for x in publish[:credit] if (x.mnemonic,x.op_str)==('cmp','byte ptr [rax], 0')])==2
gate=ins('?PortalPlayer@go_devourer_miscAI@go_devourer_misc@@')
assert {248901,1502,41924,4}<=values(gate)
for target in ('?CanContact@Player@@','?CanTargetQuestObject@WorldObject@@','?GetInteractionDistance@GameObject@@','?IsWithinDistInMap@WorldObject@@','?InSamePhase@WorldObject@@','?InSamePhaseId@WorldObject@@Q','?canSeeOrDetect@WorldObject@@','?IsWithinLOSInMap@WorldObject@@','?getObjectHitPos@Map@@','?isInLineOfSight@Map@@','?GetQuestCancellationReceipt@Player@@','?CancellationReceipt@QuestObjectScope@@','??8ObjectGuid@@'):
    assert sites(n,gate,target),target
use=ins('?GossipUse@go_devourer_miscAI@go_devourer_misc@@')
assert 248901 in values(use) and sites(n,use,'?PortalPlayer@go_devourer_miscAI@go_devourer_misc@@')
for body in (start,owner,active,cast,publish,gate,observer):
    for forbidden in ('?AddItem@Player@@','?CompleteQuest@Player@@','?KilledMonsterCredit@Player@@','?UpdateAchievementCriteria@Player@@','?SafeTeleport@Player@@'):
        assert not sites(n,body,forbidden),forbidden
print('PASS registered Valzuun41924 accept/gossip: qualified source, full128 viewer before native summon, current quest scope/receipt and live dedup; native selected-player209056 plus actual current hit acknowledgement before owned248901/300s, original1/4/11.5s and6s closing voice. Registered GO use guards owner/current attempt/class/map/phase/3D range and real visible surface, retaining other blockers and native credit/209066. No manual quest/item/transfer. Native tests distinguish explicit faults and client traversal.')

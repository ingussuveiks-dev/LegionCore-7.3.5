"""Selected final Wardstone control: native use/receipts before key and phases."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values, floats
from merlis_intro_pe import reachable

p=runpy.run_path(str(Path(__file__).with_name('verify_devourer_claim_release.py')))
n,ins=p['n'],p['ins']
method=lambda name:'?'+name+'@go_devourer_miscAI@go_devourer_misc@@'
pairs=lambda body:{(x.mnemonic,x.op_str) for x in body}
def refs(body):
    return {x.address+x.size+o.mem.disp for x in body for o in x.operands
            if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP}
text=(n['args'].release/'worldserver.map').read_text(errors='replace')
def table(name):
    rows=re.findall(r'\s+\w+:\w+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{16})',text)
    assert len(rows)==1,name
    return int(rows[0],16)
registered=table('??_7go_devourer_misc@@6BScriptObject@@@')
assert registered in refs(ins('?AddSC_Devourer@@'))
assert n['resolve'](struct.unpack_from('<13Q',n['binary'],n['offset'](registered))[12])==n['symbol']('?GetAI@go_devourer_misc@@')
factory=ins('?GetAI@go_devourer_misc@@')
slots={8:'GossipUse',9:'GossipHello',16:'GetData'}
selected=[at for at in refs(factory) if all(n['resolve'](struct.unpack_from('<22Q',n['binary'],n['offset'](at))[slot])==n['symbol'](method(name)) for slot,name in slots.items())]
assert len(selected)==1 and {('mov','ecx, 0x18'),('mov','dword ptr [rax + 0x10], 0')} <= pairs(factory)
use,hello,context,actor,opening=[ins(method(x)) for x in ('GossipUse','GossipHello','DoorContext','DoorActor','OpenDoor')]
assert 249390 in values(use) and 249390 in values(hello)
assert sites(n,use,method('OpenDoor')) and sites(n,hello,method('OpenDoor'))
assert ('xor','al, al') in pairs(use) and ('mov','r8b, 1') in pairs(use)
assert {249390,1607,1078,41924,48002,48005,48004,2,3,4} <= values(context)
for target,count in [('?IsInWorld@Object@@',2),('?IsPreDelete@Object@@',2),('?IsQuestObjectExpired@WorldObject@@',2),('?CanTargetQuestObject@WorldObject@@',2),('?GetQuestStatus@Player@@',2),('?GetQuestRewardStatus@Player@@',1),('?GetInstanceScript@WorldObject@@',1),('?CanContact@Player@@',1),('?InSamePhase@WorldObject@@',1),('?InSamePhaseId@WorldObject@@Q',1),('?IsWithinLOSInMap@WorldObject@@',1),('?canSeeOrDetect@WorldObject@@',1),('?GetInteractionDistance@GameObject@@',1),('?IsWithinDistInMap@WorldObject@@',1)]:
    assert len(sites(n,context,target))==count,target
assert {('cmp','al, 4'),('test','byte ptr [rbx + 0x4ff0], 1')} <= pairs(context)
assert 249391 in values(actor) and 10.0 in floats(n,actor)
for target in ('?IsInWorld@Object@@','?IsPreDelete@Object@@','?IsQuestObjectExpired@WorldObject@@','?InSamePhase@WorldObject@@','?InSamePhaseId@WorldObject@@Q','?IsWithinDistInMap@WorldObject@@','?CanTargetQuestObject@WorldObject@@'):
    assert sites(n,actor,target),target
assert len(sites(n,opening,method('DoorContext')))==2 and len(sites(n,opening,method('DoorActor')))==2
find,=sites(n,opening,'?FindNearestGameObject@WorldObject@@')
award,=sites(n,opening,'?UpdateAchievementCriteria@Player@@')
lookup,=sites(n,opening,'?GetGameObject@ObjectAccessor@@')
identities=sites(n,opening,'??9ObjectGuid@@')
receipts=sites(n,opening,'?GetQuestCancellationReceipt@Player@@')
assert len(identities)==2 and len(receipts)==2
assert find<receipts[0]<award<identities[0]<receipts[1]<lookup<identities[1]
assert {136802,249391,50536,41924,48004,105538,105537,209349,5000} <= values(opening)
assert {10.0,90.0,15.0} <= floats(n,opening)
# Native virtual SetPhaseMask slot20 retains model collision refresh. Both
# captured actors are changed only after the current real leaf and full GUIDs.
go_table=table('??_7GameObject@@6BWorldObject@@@')
assert n['resolve'](struct.unpack_from('<21Q',n['binary'],n['offset'](go_table))[20])==n['symbol']('?SetPhaseMask@GameObject@@')
phases=[i for i,x in enumerate(opening) if x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0xa0]
assert len(phases)==2
latch,=[i for i,x in enumerate(opening) if (x.mnemonic,x.op_str)==('mov','byte ptr [rbx + 0x10], 1')]
destroy,=sites(n,opening,'?DestroyItemCount@Player@@')
assert identities[1]<latch<phases[0]<phases[1]<destroy
assert opening[identities[1]+2].mnemonic=='jne'
assert opening[latch].address not in reachable(opening,opening[identities[1]+2].operands[0].imm)
post_leaf,=sites(n,opening,method('DoorLeaf'))
assert award<post_leaf<lookup and opening[post_leaf+2].mnemonic=='je'
assert opening[latch].address not in reachable(opening,opening[post_leaf+2].operands[0].imm)
assert {('cmp','dword ptr [rsp + 0x44], 2'),('cmp','r15d, 2'),('mov','byte ptr [rdi], 1'),('mov','byte ptr [rdi], 0'),('movaps','xmmword ptr [rsp + 0x90], xmm0'),('movaps','xmmword ptr [rsp + 0xa0], xmm0'),('cmp','qword ptr [rax], rcx'),('cmp','rax, r13'),('movzx','ecx, byte ptr [rax]'),('mov','byte ptr [rsp + 0x40], 1'),('mov','byte ptr [rsp + 0x40], 0')} <= pairs(opening)
assert len(sites(n,opening,'?GetCreatureListWithEntryInGrid@WorldObject@@'))==2
for target in ('?MoveRandom@MotionMaster@@','?DespawnOrUnsummon@Creature@@','?Talk@CreatureAI@@'):
    assert sites(n,opening,target)
for body in (context,actor,opening):
    for target in ('?AddItem@Player@@','?CompleteQuest@Player@@','?SetCurrentStep@Scenario@@'):
        assert not sites(n,body,target)
print('PASS registered Wardstone door0x18 AI/use+hello: qualified1078/2 actual cage48002/cache48005 before50536, current leaf48004/root acknowledgement before key/both native phase+collision updates; capture barrier before control phase, fresh fullGUID/current receipt/actor and reentry guards, earned checkpoint cleanup without another key, original90yd frightened-add15yd/5s/Talk0 and no item/quest/scenario shortcut. Client traversal/grid/rendering separate.')

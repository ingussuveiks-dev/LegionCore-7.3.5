"""Inspect registered Kingslayers native activation and independent event boundaries."""
from pathlib import Path
import runpy,re,struct,subprocess
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites,values,floats
repo=Path(__file__).resolve().parents[2];path='src/server/game/Spells/SpellEffects.cpp'
before=subprocess.check_output(['git','-C',str(repo),'show','a9e86af7b:'+path],text=True,encoding='utf-8');current=(repo/path).read_text(encoding='utf-8')
addition='''    // This authored door owns activation/progression in its AI after native
    // lock validation. Other doors retain their original early-use path.
    bool const kingslayersDoor = gameObjTarget && gameObjTarget->GetEntry() == 251053 &&
        gameObjTarget->GetDBTableGUIDLow() == 133953 && gameObjTarget->GetMapId() == 1620 &&
        sObjectMgr->GetScriptName(gameObjTarget->GetScriptId()) == "go_kingslayers_door";

'''
assert current.count(addition)==1 and current.replace(addition,'').replace('goInfo->type == GAMEOBJECT_TYPE_DOOR && !kingslayersDoor','goInfo->type == GAMEOBJECT_TYPE_DOOR')==before,'Shared spell certificate: only exact original door early activation may change'
p=runpy.run_path(str(Path(__file__).with_name('verify_althea_credit_release.py')))
n=p['n'];ins=n['instructions'];suffix='@go_kingslayers_doorAI@go_kingslayers_door@@'
method=lambda name:'?'+name+suffix
body=ins(method('Open'));context=ins(method('Context'));source=ins(method('Source'))
use,=sites(n,body,'?UseDoorOrButton@GameObject@@');awards=sites(n,body,'?UpdateAchievementCriteria@Player@@')
assert len(awards)==2 and use<awards[0]<awards[1]
assert {51549,92}<=values(body[awards[0]-8:awards[0]]) and {51654,92}<=values(body[awards[1]-8:awards[1]])
contexts=sites(n,body,method('Context'));assert contexts[0]<use<contexts[1]<awards[0]<contexts[2]<awards[1]<contexts[3]
assert len(sites(n,body,method('Leaf')))==3 and len(sites(n,body,'?CanUpdateCriteria@Scenario@@'))==2
script,=sites(n,body,'?ScriptsStart@Map@@');move,=sites(n,body,'?MovePoint@MotionMaster@@')
assert contexts[3]<move<script and {108214,132653,48577,48597}<=values(body)
assert {60.0,-8625.5,415.93,103.7}<=floats(n,body)
assert ('cmp','byte ptr [rcx + 0x10], 0') in [(x.mnemonic,x.op_str) for x in body[:8]]
assert {251053,133953,1620}<=values(source)
for target in ('?IsInWorld@Object@@','?isSpawned@GameObject@@','?IsQuestObjectExpired@WorldObject@@','?IsPreDelete@Object@@','?CanContact@Player@@','?InSamePhase@WorldObject@@','?canSeeOrDetect@WorldObject@@','?GetInteractionDistance@GameObject@@','?IsWithinDistInMap@WorldObject@@','?GetQuestRewardStatus@Player@@','?GetQuestStatus@Player@@','?CanTargetQuestObject@WorldObject@@','?GetScenario@ScenarioMgr@@'):
 assert sites(n,context,target),target
assert {42504,42627,1123,48577,48597}<=values(context)
assert not sites(n,context,'?IsWithinLOSInMap@WorldObject@@'),'The closed door own mesh must not deny native range interaction'
for name in ('GossipUse','GossipHello'):
 hook=ins(method(name));assert {251053,133953,1620}<=values(hook) and len(sites(n,hook,method('Open')))==1
hello=ins(method('GossipHello'));at,=sites(n,hello,method('Open'))
assert any(x.mnemonic=='test' and x.op_str=='dil, dil' for x in hello[:at]),'Report use must not open'
effect=ins('?EffectOpenLock@Spell@@');early,=sites(n,effect,'?UseDoorOrButton@GameObject@@');lock,=sites(n,effect,'?CanOpenLock@Spell@@');loot,=sites(n,effect,'?SendLoot@Spell@@')
assert early<lock<loot and {251053,133953,1620}<=values(effect[:early])
assert sites(n,effect[:early],'?GetScriptName@ObjectMgr@@')
window=[(x.mnemonic,x.op_str) for x in effect[early-8:early]]
assert ('test','dil, dil') in window and any(x.mnemonic=='jne' and x.operands[0].imm>effect[early].address for x in effect[early-8:early]),'Scoped door must skip pre-lock activation'
maptext=(n['args'].release/'worldserver.map').read_text(errors='replace')
table,=re.findall(r'\s+\w+:\w+\s+\?\?_7go_kingslayers_doorAI@go_kingslayers_door@@6B@\s+([0-9a-fA-F]{16})',maptext)
address=int(table,16);factory=ins('?GetAI@go_kingslayers_door@@')
refs={x.address+x.size+o.mem.disp for x in factory for o in x.operands if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP}
assert address in refs
slots=struct.unpack_from('<30Q',n['binary'],n['offset'](address))
for name in ('GossipUse','GossipHello'):assert any(n['resolve'](v)==n['symbol'](method(name)) for v in slots)
for candidate in (body,context,source):
 for forbidden in ('?AddItem@Player@@','?CompleteQuest@Player@@','?KilledMonsterCredit@Player@@'):assert not sites(n,candidate,forbidden)
assert 51613 not in values(body+context+source),'Door must not manufacture victory'
print('PASS actual registered door AI/vtable owns native activation before51549/51654, rechecks native scenario/quest scope and real counters, report/refusal guards, original Garona and once-per-activation DB scripts. Exact source skips pre-lock activation in EffectOpenLock; other doors retain it. No victory/item/forced quest grants; controlled native and actual client traversal remain distinct.')

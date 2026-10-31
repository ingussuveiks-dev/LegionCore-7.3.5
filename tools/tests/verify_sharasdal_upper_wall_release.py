"""Verify selected upper Grash arrival, real wall acknowledgement and receipt in PE."""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

retained=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_acquisition_release.py')))
n=retained['n'];ins=n['instructions'];sites=retained['sites'];suffix='@npc_grashAI@npc_grash@@'
def method(name):return '?'+name+suffix
def values(body):return {o.imm for x in body for o in x.operands if o.type==CS_OP_IMM}
def fields(body):return {o.mem.disp for x in body for o in x.operands if o.type==CS_OP_MEM}
def needs(body,*targets):
 for target in targets:assert sites(body,target),target
def floats(body):
 return {round(struct.unpack_from('<f',n['binary'],n['offset'](x.address+x.size+o.mem.disp))[0],3)
         for x in body for o in x.operands if o.type==CS_OP_MEM and o.mem.base==X86_REG_RIP}
def table_has(at,*methods):
 try:rows=struct.unpack_from('<82Q',n['binary'],n['offset'](at))
 except (ValueError,struct.error):return False
 return all(n['symbol'](m) in rows for m in methods)
factory='?GetAI@npc_grash@@';refs=retained['refs']
tables=[at for at in refs(factory) if table_has(at,*(method(m) for m in ['sGossipSelect','MovementInform','UpdateLifecycle','SetGUID','Reset','DoAction']))]
assert len(tables)==1
assert any(table_has(at,factory,'?OnGossipHello@npc_grash@@') for at in refs('?AddSC_Sharasdal@@'))
table=tables[0]
slot,=[i*8 for i,v in enumerate(struct.unpack_from('<82Q',n['binary'],n['offset'](table))) if v==n['symbol'](method('MovementInform'))]
native=ins('?DoFinalize@?$PointMovementGenerator@VCreature@@@@')
assert 8 in values(native) and slot in fields(native) # real finalized spline -> selected AI slot
world=ins(method('UpperWallScenario'))
assert {105298,362092,1600,1066}<=values(world)
needs(world,'?IsInWorld@Object@@','?IsPreDelete@Object@@','?GetScenario@ScenarioMgr@@')
select=ins(method('sGossipSelect'))
assert 49819 not in values(select) and {40341,47133,4}<=values(select)
assert {0x1bc,0x1d0,0x1e0,0x1e4,0x1f4}<=fields(select)
needs(select,method('UpperWallScenario'),'?AddQuestDelayedEvent@Player@@','?getQuestStatus@Player@@','?MovePoint@MotionMaster@@')
issued,=[i for i,x in enumerate(select) if x.mnemonic=='inc' and x.op_str=='dword ptr [rdi + 0x1e0]']
assert issued<sites(select,'?MovePoint@MotionMaster@@')[0]
assert {-17.96,802.05,807.66}<=floats(select)
movement=ins(method('MovementInform'));arrival=ins(method('UpperWallArrival'));credit=ins(method('CompleteUpperWall'))
assert {105298,362092}<=values(movement);needs(movement,method('UpperWallArrival'))
assert not sites(movement,'?UpdateAchievementCriteria@Player@@')
assert {0x1f4,0x1e4,0x1f5}<=fields(arrival) and {17.96,802.05,807.66,1.0}<=floats(arrival)
needs(arrival,method('UpperWallScenario'),method('CancelUpperWall'),method('CompleteUpperWall'))
needs(credit,method('UpperWallScenario'),'?GetPlayer@ObjectAccessor@@','?getQuestStatus@Player@@','?Eligible@SharasdalGrash@@',
      '?CastSpell@Unit@@','?FindNearestGameObject@WorldObject@@','?GetDBTableGUIDLow@GameObject@@','?IsPreDelete@Object@@',
      '?InSamePhase@WorldObject@@','?HandleGameObject@InstanceScript@@','?GetGoState@GameObject@@','?UpdateAchievementCriteria@Player@@')
assert {40341,47133,4,209904,248514,134298,362095,362093,102826,104853,49819,1000,30,300000}<=values(credit)
cast,=sites(credit,'?CastSpell@Unit@@');opened,=sites(credit,'?HandleGameObject@InstanceScript@@');state,=sites(credit,'?GetGoState@GameObject@@');event,=sites(credit,'?UpdateAchievementCriteria@Player@@')
assert cast<opened<state<event
assert any(x.mnemonic=='test' and x.op_str=='eax, eax' for x in credit[cast:cast+5])
assert any(x.mnemonic=='sete' and x.op_str=='cl' for x in credit[state:state+6])
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rbx + 0x1f7], cl' for x in credit[state:state+6])
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rbx + 0x1f6], al' for x in credit[cast:cast+6])
# Compiler forms the exact upper ally GUID as362093 + (entry !=102826).
assert any(x.mnemonic=='add' and x.op_str=='rcx, 0x5866d' for x in credit)
assert any(x.mnemonic=='setne' and x.op_str=='cl' for x in credit)
assert {0x1bc,0x1d8,0x1e8,0x1ec,0x1f0,0x1f7,0x1f8,0x1f9}<=fields(credit)
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rbx + 0x1f8], 1' for x in credit[:event])
assert any(i<event for i in sites(credit,'?Receipt@SharasdalGrash@@')) and any(i>event for i in sites(credit,'?Receipt@SharasdalGrash@@'))
life=ins(method('UpdateLifecycle'));assert sites(life,method('CompleteWallCredit'))[0]<sites(life,method('CompleteUpperWall'))[0]
rearm=ins(method('SetGUID'));assert {40341}<=values(rearm) and {0x190,0x1bc,0x1f5,0x1f9,0x1ec}<=fields(rearm)
assert not sites(rearm,'?UpdateAchievementCriteria@Player@@') and not sites(rearm,'?MovePoint@MotionMaster@@')
entry=ins('?OnPlayerEnter@instance_sharasdal_InstanceMapScript@instance_sharasdal@@')
assert {0x39f,0x444}<=fields(entry) # lower and appended upper complete GUIDs, same distinct-AI dispatch loop
create=ins('?OnCreatureCreate@instance_sharasdal_InstanceMapScript@instance_sharasdal@@')
assert 362092 in values(create) and 0x444 in fields(create)
for body in (select,arrival,credit,rearm):
 for forbidden in ('?AddItem@Player@@','?CompleteQuest@Player@@','?AddDelayedEvent@Unit@@','?FindNearestPlayer@WorldObject@@'):
  assert not sites(body,forbidden)
for message in (b'Sharasdal upper Grash point finalized away from its wall',b'Sharasdal upper wall cast/object unavailable after thirty attempts',b'Sharasdal opened upper wall could not reach its original recipient/native criterion'):
 assert message in n['binary']
print('PASS selected upper Grash factory/native Point finalize and issued serial/actual center pose; ordinary self Thud succeeds before exact134298 open-state acknowledgement, only then original40341/weak lease/player49819/native47133 receipt. Finite movement/cast/object/delivery recovery, upper/lower re-entry dispatch and earlier field order verified; retained acquisition/Lady/Kraliss/lower routes checked. Actual client movement/presentation/full1066 remain separate.')

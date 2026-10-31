"""Verify actual illusion admission, native arrival acknowledgement and guide recovery."""
from pathlib import Path
import runpy,struct
from capstone import CS_OP_IMM,CS_OP_MEM

retained=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_upper_wall_release.py')))
n=retained['n'];ins=n['instructions'];sites=retained['sites'];refs=retained['refs']
values=retained['values'];fields=retained['fields'];floats=retained['floats']
suffix='@npc_lady_zitrinAI@npc_lady_zitrin@@'
def method(name):return '?'+name+suffix
def needs(body,*targets):
 for target in targets:assert sites(body,target),target
def resolved_rows(at):
 try:rows=struct.unpack_from('<82Q',n['binary'],n['offset'](at))
 except (ValueError,struct.error):return []
 result=[]
 for row in rows:
  try:result.append(n['resolve'](row))
  except ValueError:result.append(row)
 return result
def table_has(at,*methods):
 rows=resolved_rows(at)
 return all(n['symbol'](m) in rows for m in methods)
def events(body):
 result=[]
 for at in sites(body,'?UpdateAchievementCriteria@Player@@'):
  args=[x.operands[1].imm for x in body[max(0,at-12):at] if x.mnemonic=='mov' and x.op_str.startswith('r8d, ') and x.operands[1].type==CS_OP_IMM]
  assert args;result.append(args[-1])
 return result
factory='?GetAI@npc_lady_zitrin@@'
tables=[at for at in refs(factory) if table_has(at,*(method(m) for m in ['MoveInLineOfSight','UpdateAI','UpdateLifecycle','SetGUID','Reset','JustDied']))]
assert len(tables)==1
table=tables[0];rows=resolved_rows(table)
assert any(table_has(at,factory) for at in refs('?AddSC_Sharasdal@@'))
slot,=[i*8 for i,v in enumerate(rows) if v==n['symbol'](method('MoveInLineOfSight'))]
safe=ins('?MoveInLineOfSight_Safe@CreatureAI@@')
assert slot==0x278 and slot in fields(safe)
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rcx + 0x18], 1' for x in safe)
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rbx + 0x18], 0' for x in safe)
assert rows[0x110//8]==n['symbol'](method('CompleteGauntletArrival')) # selected lifecycle tail thunk
world=ins(method('GauntletWorld'));player=ins(method('GauntletPlayer'));los=ins(method('MoveInLineOfSight'));complete=ins(method('CompleteGauntletArrival'))
assert {104856,362091,1600,1066}<=values(world)
needs(world,'?IsInWorld@Object@@','?IsPreDelete@Object@@','?GetScenario@ScenarioMgr@@')
assert {40341,7,1,3}<=values(player) and 40.0 in floats(player)
needs(player,'?CanContact@Player@@','?InSamePhase@WorldObject@@','?canSeeOrDetect@WorldObject@@','?IsWithinLOSInMap@WorldObject@@','?IsWithinDistInMap@WorldObject@@','?GetQuestRewardStatus@Player@@','?GetQuestStatus@Player@@')
assert {3,47131,40341}<=values(los) and {0x62,0x78,0x88,0x8a,0x90}<=fields(los)
needs(los,method('GauntletWorld'),method('GauntletPlayer'),'?AddQuestDelayedEvent@Player@@','?getQuestStatus@Player@@',method('CompleteGauntletArrival'))
assert not events(los) and not sites(los,'?DespawnOrUnsummon@Creature@@')
assert events(complete)==[50314] and {40341,3,4,47131,47378,105298,362092,1000,30}<=values(complete)
assert {0x62,0x80,0x88,0x89,0x8a,0x8b,0x8c,0x90,0x94}<=fields(complete)
needs(complete,'?GetPlayer@ObjectAccessor@@','?getQuestStatus@Player@@',method('GauntletPlayer'),'?Receipt@SharasdalGrash@@','?FindNearestCreature@WorldObject@@','?InSamePhase@WorldObject@@','?MovePoint@MotionMaster@@','?DespawnOrUnsummon@Creature@@')
event,=sites(complete,'?UpdateAchievementCriteria@Player@@')
assert any(i<event for i in sites(complete,'?Receipt@SharasdalGrash@@')) and any(i>event for i in sites(complete,'?Receipt@SharasdalGrash@@'))
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rbx], 1' for x in complete[:event])
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rbx], 0' for x in complete[event:])
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rdi + 0x89], 1' for x in complete[event:])
assert any(x.mnemonic=='mov' and x.op_str=='word ptr [rdi + 0x60], 1' for x in complete[event:]) # done=true,intro=false only after receipt
action,=[i for i,x in enumerate(complete) if x.mnemonic=='call' and x.op_str=='qword ptr [rax + 0x48]']
assert event<action<sites(complete,'?MovePoint@MotionMaster@@')[0]
assert any(x.mnemonic=='mov' and x.op_str=='byte ptr [rdi + 0x88], 0' for x in complete[action-12:action])
assert [(x.mnemonic,x.op_str) for x in complete[action-6:action-1]]==[
 ('mov','rcx, qword ptr [rsi + 0x9c8]'),('test','rcx, rcx'),('jne',complete[action-4].op_str),
 ('mov','rcx, qword ptr [rsi + 0x9c0]'),('mov','rax, qword ptr [rcx]')]
assert complete[action-4].operands[0].imm==complete[action-2].address
assert complete[action-1].op_str=='edx, 3' # original suspended guide preferred, one actual DoAction3
assert {10.37,803.32,806.75,200.0}<=floats(complete)
rearm=ins(method('SetGUID'));assert {40341,105298,362092}<=values(rearm) and {0x62,0x88,0x89,0x8b,0x90}<=fields(rearm)
needs(rearm,'??8ObjectGuid@@','?GetCreature@Map@@')
assert not events(rearm) and not sites(rearm,'?MovePoint@MotionMaster@@')
instance='@instance_sharasdal_InstanceMapScript@instance_sharasdal@@'
entry=ins('?OnPlayerEnter'+instance);create=ins('?OnCreatureCreate'+instance);reentry=ins('?RearmGauntlet'+instance)
needs(entry,'?RearmGauntlet'+instance);needs(create,'?RearmGauntlet'+instance)
assert {104856,362091,105298,362092}<=values(create) and {0x444,0x454}<=fields(create)
assert {0x454,0x9c0,0x9c8,0x80}<=fields(reentry)
assert len([x for x in reentry if x.mnemonic=='call' and x.op_str=='qword ptr [rax + 0x80]'])==2
assert any(x.mnemonic=='cmp' and x.op_str=='rbx, rdi' for x in reentry)
waves=ins(method('UpdateAI'));constructor=ins(factory)
needs(waves,method('GauntletWorld'),'?SummonCreature@WorldObject@@')
assert {3,105251,209132,6000}<=values(waves) and {104856,362091,2000}<=values(constructor)
assert not events(waves) and not sites(waves,'?DespawnOrUnsummon@Creature@@')
for body in (los,complete,rearm):
 for forbidden in ('?AddItem@Player@@','?CompleteQuest@Player@@','?CreateConversation@Unit@@','?AddDelayedEvent@Unit@@','?FindNearestPlayer@WorldObject@@'):
  assert not sites(body,forbidden)
for message in (b'Sharasdal gauntlet arrival lost its native stage/prerequisite',b'Sharasdal gauntlet original arrival/native criterion or upper Grash unavailable after thirty attempts'):
 assert message in n['binary']
print('PASS selected actual illusion factory/SafeLOS278/lifecycle110: exact362091/world1066/player40341/native3+ascent47131, original full GUID/quest lease and sole50314 with47378 acknowledgement before latch/despawn/guide continuation. Native original suspended Grash preferred once, finite retry and actual recipient/actor-entry rearm; wave2s/6s payload gated to native3. Full upper wall/acquisition/Lady/Kraliss/other retained PE passes; no actual client/whole1066 claim.')

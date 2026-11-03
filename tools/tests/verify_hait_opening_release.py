"""Registered initial Hait receipt/instance owner and native child handoff."""
from pathlib import Path
import runpy,struct,json,hashlib
from capstone import CS_OP_MEM,CS_OP_IMM
from sharasdal_erunak_entry_pe import sites,values,fields,floats
from monk_intro_instance_pe import construction,reachable

p=runpy.run_path(str(Path(__file__).with_name('verify_monk_hait_click_release.py')))
n,ins,refs,table,resolved=(p[k] for k in ('n','ins','refs','table','resolved'))
def state(name):return '?'+name+'@State@MonkHaitOpening@@'
def inst(name):return '?'+name+'@instance_monk_intro@@'
def pairs(body):return [(o.mnemonic,o.op_str) for o in body]
def needs(body,*names):
 for name in names:assert sites(n,body,name),('Opening native dependency',name)

allocation,ctor,_=construction(n)
assert n['monk_hait_opening_successor'] and 0x4a0 in values(allocation)
vt,raw=table('??_7instance_monk_intro@@6B@',140)
loader,lraw=table('??_7?$GenericInstanceMapScript@Uinstance_monk_intro@@@@6BScriptObject@@@',2)
assert vt in refs(ctor) and loader in refs(ins('?AddSC_instance_class_art_scenarios@@'))
assert n['symbol']('?GetInstanceScript@?$GenericInstanceMapScript@Uinstance_monk_intro@@') in resolved(lraw)
for name in ('OnCreatureCreate','GetGuidData','SetGuidData','Update'):assert n['symbol'](inst(name)) in resolved(raw)
assert {('mov','qword ptr [r14 + 0x498], rbx'),('mov','qword ptr [rax], rbp'),('mov','ecx, '+hex(n['monk_hait_state_size']))}<=set(pairs(ctor))
dispatch=ins(inst('Update'));update=ins(state('Update'));accept=ins(state('Accept'));launch=ins(state('Launch'))
opening,=sites(n,dispatch,state('Update'));nine,=sites(n,dispatch,'?Update@State@MonkNineRescue@@')
assert opening<nine and ('mov','rcx, qword ptr [rcx + 0x498]') in pairs(dispatch[:opening])
get=ins(inst('GetGuidData'));put=ins(inst('SetGuidData'));created=ins(inst('OnCreatureCreate'))
assert 94355 in values(get) and 94354 in values(put)
needs(put,state('Accept'))
assert 0x498 in fields(get) and 0x38 in fields(get) and 0x498 in fields(put)
assert {97778,98001,98011,362105,362111,362135}<=values(created)
assert 0x498 in fields(created)
needs(accept,state('Progress'),'?FindPlayer@ObjectAccessor@@','?getQuestStatus@Player@@','?GetCreature@Map@@',
      '?IsInWorld@Object@@','?IsPreDelete@Object@@','?ShouldHideFor@WorldObject@@','?CanContact@Player@@',
      '?GetQuestStatus@Player@@','?GetQuestRewardStatus@Player@@','?FindQuestSlot@Player@@',
      '?InSamePhase@WorldObject@@','?canSeeOrDetect@WorldObject@@','?IsWithinLOSInMap@WorldObject@@',
      '?IsWithinDistInMap@WorldObject@@','?FindNearestCreature@WorldObject@@','?GetReqKillOrCastCurrentCount@Player@@','?getMSTime@@')
assert {12103,97778,98514,98001,98011,362105,362111,362135}<=values(accept)
assert {5.0,40.0,90.0}<=floats(n,accept)
assert len(sites(n,accept,'?GetReqKillOrCastCurrentCount@Player@@'))==2
assert len(sites(n,accept,'?FindNearestCreature@WorldObject@@'))==2
progress=ins(state('Progress'));retained=ins(state('Retained'))
needs(progress,'?GetScenario@ScenarioMgr@@','?GetScenarioId@Scenario@@','?GetCurrentStep@Scenario@@')
assert {1014,943}<=values(progress) and (('cmp','eax, 0') in pairs(progress) or ('test','eax, eax') in pairs(progress))
needs(retained,'?FindPlayer@ObjectAccessor@@','?getQuestStatus@Player@@','?GetQuestStatus@Player@@','?GetQuestRewardStatus@Player@@','?FindQuestSlot@Player@@')
assert 12103 in values(retained) and any(o.mnemonic=='lock cmpxchg' for o in retained)
needs(update,state('Progress'),state('Retained'),state('Launch'),'?getMSTime@@','?GetCreature@Map@@','?SendChat@CreatureTextMgr@@','?outMessage@Log@@')
assert {1000,4000,7000,10000,120000,97778,98001,98011,362105,362111,362135}<=values(update)
assert {('lea','rbx, [rcx + 0x5d]'),('cmp','byte ptr [rbx], 0'),('cmp','byte ptr [rcx + 0x5e], 0')}<=set(pairs(update))
assert 0x5f in fields(update) and 0x5c in fields(update) and 0x58 in fields(update)
needs(launch,'?SendChat@CreatureTextMgr@@','?CastSpell@Unit@@','?MovePoint@MotionMaster@@',
      '?FindNearestGameObject@WorldObject@@','?UseDoorOrButton@GameObject@@','?SetFlag@Object@@','?AddDelayedEvent@FunctionProcessor@@')
assert {193053,244682,20000,23000,8000,15000,0x380}<=values(launch)
assert {3812.82,1810.81,950.99,3812.91,1812.55}<=floats(n,launch)
assert {8,16,32,64}<=values(launch) and 0x5c in fields(launch) and 0x5f in fields(launch)
callbacks=[name for name in n['symbols'] if name.startswith('?_Do_call@') and '??Launch@State@MonkHaitOpening@@' in name]
assert len(callbacks)==4
attack=ins(state('AttackDemon'))
retry,=[name for name in n['symbols'] if name.startswith('?_Do_call@') and '??AttackDemon@State@MonkHaitOpening@@' in name]
for name in callbacks+[retry]:
 assert any(n['symbol'](name) in resolved(struct.unpack_from('<8Q',n['binary'],n['offset'](address))) for body in (launch,attack) for address in refs(body)),('Actual installed callback',name)
bodies=[ins(name) for name in callbacks]
flight,=[body for body in bodies if 227140 in values(body)];cleanup,=[body for body in bodies if sites(n,body,'?RemoveAllPassengers@Vehicle@@')]
assert {193268,227140}<=values(flight) and len(sites(n,flight,'?CastSpell@Unit@@'))==2
assert len(sites(n,flight,'?GetCreature@ObjectAccessor@@'))==1 and 0x10 in fields(flight)
assert {185627,0x380,10}<=values(cleanup) and len(sites(n,cleanup,'?GetCreature@ObjectAccessor@@'))==2
passengers,=sites(n,cleanup,'?RemoveAllPassengers@Vehicle@@');flags,=sites(n,cleanup,'?RemoveFlag@Object@@');combat,=sites(n,cleanup,state('AttackDemon'))
assert passengers<flags<combat and {0x10,0x20}<=fields(cleanup)
needs(attack,'?GetCreature@ObjectAccessor@@','?AddDelayedEvent@FunctionProcessor@@','?outMessage@Log@@')
assert 500 in values(attack) and 0x9c0 in fields(attack) and 0xa18 in fields(attack)
needs(ins(retry),state('AttackDemon'))
for body in (accept,update,launch,attack,*bodies):
 for forbidden in ('?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@'):
  assert not sites(n,body,forbidden),('No fabricated progress',forbidden)
action=ins('?DoAction@npc_master_haitAI@npc_master_hait@@')
guard=next(i for i,o in enumerate(action) if o.mnemonic=='cmp' and o.op_str.endswith(' + 0x3a88], 0x58679'))
branch=action[guard+1];assert branch.mnemonic in ('je','jne')
equal=branch.operands[0].imm if branch.mnemonic=='je' else branch.address+branch.size
assert all(action[i].address not in reachable(action,equal) for i in sites(n,action,'?AddDelayedEvent@FunctionProcessor@@'))
hook=ins(inst('OnCreatureDamageTaken'))
assert n['symbol'](inst('OnCreatureDamageTaken')) in resolved(raw)
if sites(n,hook,state('Protect')):protection=ins(state('Protect'))
else:protection=hook
assert {1014,97778,362105}<=values(protection)
needs(protection,'?GetGuidValue@Object@@','??8ObjectGuid@@')
assert any(o.mnemonic=='mov' and o.operands[0].type==CS_OP_MEM and o.operands[0].size==4 and o.operands[1].type==CS_OP_IMM and o.operands[1].imm==0 for o in protection)
assert 0x498 in fields(hook)
print('PASS registered4a0 instance/498-owned accepted Hait opening: retained96-byte prefix, native97778+98514 receipt transfer, original weak owner/full actor GUIDs/943stage0, actual1/4/7/10 clocks/resumable once commands/finite120s diagnostic, independent actual Demon20/23 and pupil8/15 queues, original flags/passengers/casts/current-AI attack retry. Appended defeat is checked separately from client/full943.')
print(json.dumps({'world_sha256':hashlib.sha256(n['binary']).hexdigest(),'instance_size':0x4a0,'owned_state_offset':0x498,'state_size':n['monk_hait_state_size'],'deadlines_ms':[1000,4000,7000,10000],'native_demon_ms':[20000,23000]}))

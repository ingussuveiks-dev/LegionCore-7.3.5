"""Real instance-owned Vizznak receipt and native original point1 in x64 PE.

Run with the extracted ASAN/current DB2 catalog suite. Neither a native model
nor this selected PE inspection certifies client movement, dialogue or taxi.
"""
from pathlib import Path
import hashlib,json,runpy
from sharasdal_erunak_entry_pe import sites,values,fields,floats
from monk_intro_instance_pe import reachable

p=runpy.run_path(str(Path(__file__).with_name('verify_monk_nine_arrival_release.py')))
n,ins,needs=(p[k] for k in ('n','ins','needs'))
assert n['monk_initial_successor']
pairs=lambda body:[(o.mnemonic,o.op_str) for o in body]
state=lambda name:'?'+name+'@State@MonkNineRescue@@'
context=lambda name:'?'+name+'@State@MonkChenRescue@@'
instance=lambda name:'?'+name+'@instance_monk_intro@@'
dispatch=ins(instance('Update'));death_dispatch=ins(instance('CreatureDies'))
first,=sites(n,dispatch,state('Update'));late,=sites(n,dispatch,'?Update@State@MonkNineArrival@@')
assert first<late and ('mov','rcx, qword ptr [rcx + 0x490]') in pairs(dispatch[:first])
first_death,=sites(n,death_dispatch,state('Death'));chen,=sites(n,death_dispatch,context('Death'))
assert first_death<chen and ('mov','rcx, qword ptr [rcx + 0x490]') in pairs(death_dispatch[:first_death])
assert {('mov','rdi, r8'),('mov','rsi, rdx')}<=set(pairs(death_dispatch[:first_death]))
request_dispatch=ins(instance('SetGuidData'));request_call,=sites(n,request_dispatch,state('Request'))
assert 94361 in values(request_dispatch)
assert {('movups','xmm0, xmmword ptr [rsi]'),('mov','rcx, qword ptr [rbp + 0x490]'),
        ('movaps','xmmword ptr [rsp + 0x20], xmm0')}<=set(pairs(request_dispatch[request_call-5:request_call]))
gate=next(i for i,o in enumerate(request_dispatch) if (o.mnemonic,o.op_str)==('cmp','edi, 0x17099'))
assert request_dispatch[gate+1].mnemonic=='jne'
assert request_dispatch[request_call].address not in reachable(request_dispatch,request_dispatch[gate+1].operands[0].imm)

death=ins(state('Death'));update=ins(state('Update'));valid=ins(state('Valid'))
actor=ins(state('Actor'));current=ins(state('Current'));leaf=ins(state('Leaf'));request=ins(state('Request'))
needs(death,context('Progress'),'?GetCurrentStep@Scenario@@','?GetCreature@Map@@',
      '?IsInWorld@Object@@','?IsPreDelete@Object@@','??8ObjectGuid@@',
      '?AddQuestDelayedEvent@Player@@','?getQuestStatus@Player@@','?GetQuestStatus@Player@@',
      '?GetQuestRewardStatus@Player@@','?FindQuestSlot@Player@@','?InSamePhase@WorldObject@@',
      '?ShouldHideFor@WorldObject@@','?IsWithinDistInMap@WorldObject@@',
      '?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@',state('Update'))
assert {97968,362187,12103,10,25}<=values(death) and 100.0 in floats(n,death)
assert sites(n,death,'?AddQuestDelayedEvent@Player@@')[0]<sites(n,death,'?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')[0]<sites(n,death,state('Update'))[0]
assert not sites(n,death,'?UpdateAchievementCriteria@Player@@')
assert {1014,97774,362181}<=values(actor)
needs(actor,'?GetCreature@Map@@','?IsInWorld@Object@@','?IsPreDelete@Object@@')
assert ('lea','rdx, [rdi + 0x10]') in pairs(actor)
needs(valid,context('Progress'),context('Retained'),state('Leaf'),state('Cancel'),'?GetCurrentStep@Scenario@@')
assert {('cmp','qword ptr [rcx + 0x78], rdx'),('cmp','rax, qword ptr [rdi + 8]')}<=set(pairs(valid))
needs(current,context('Retained'),'?CanContact@Player@@','?IsInWorld@Object@@',
      '?InSamePhase@WorldObject@@','?ShouldHideFor@WorldObject@@','?IsWithinDistInMap@WorldObject@@')
assert 100.0 in floats(n,current)
needs(leaf,context('Progress'),'?GetAchievementMgr@Scenario@@','?GetCriteriaProgressMap@')
assert 29046 in values(leaf) and ('cmp','qword ptr [rcx + 0x28], 0') in pairs(leaf)
needs(request,'?FindPlayer@ObjectAccessor@@','?getQuestStatus@Player@@','?AddQuestDelayedEvent@Player@@',
      state('Current'),context('Retained'),'?IsWithinLOSInMap@WorldObject@@','?canSeeOrDetect@WorldObject@@','?SendChat@CreatureTextMgr@@')
assert 30.0 in floats(n,request) and 12103 in values(request)
needs(update,state('Valid'),state('Actor'),state('Current'),state('Leaf'),'?getMSTime@@',
      '?CastSpell@Unit@@','?SendChat@CreatureTextMgr@@','?MoveJump@MotionMaster@@',
      '?UpdateAchievementCriteria@Player@@','?MovePoint@MotionMaster@@',
      '?GetMotionSlot@MotionMaster@@','?IsWithinDist3d@WorldObject@@',
      '?SetHomePosition@Creature@@','?SetFlag@Object@@','?outMessage@Log@@')
assert {3000,7000,4000,2000,193248,193721,46734,92}<=values(update)
event,=sites(n,update,'?UpdateAchievementCriteria@Player@@');move,=sites(n,update,'?MovePoint@MotionMaster@@')
casts=sites(n,update,'?CastSpell@Unit@@');chats=sites(n,update,'?SendChat@CreatureTextMgr@@')
assert len(casts)==2 and len(chats)==4 and casts[0]<chats[0]<chats[1]<event<move<chats[2]<chats[3]<casts[1]
assert {('mov','edx, 0x5c'),('mov','r8d, 0xb68e')}<=set(pairs(update[event-10:event]))
assert sites(n,update,state('Leaf'))[-1]<move
assert {('mov','byte ptr [rsi + 0x92], 1'),('cmp','dword ptr [rsi + 0x80], 2')}<=set(pairs(update[event:move]))
assert {('mov','edx, 1'),('mov','byte ptr [rsp + 0x28], 1')}<=set(pairs(update[move-16:move]))
assert {3848.98,1945.78,931.85}<=set(round(x,2) for x in floats(n,update))
# Actual Unit spline pointer8c8 -> native GetId64 and Finalized68/20. Compare
# the installed active-point identity before retaining that issued spline ID.
assert {('cmp','rax, rdi'),('mov','ecx, dword ptr [rax + 0x64]'),
        ('cmp','ecx, dword ptr [rsi + 0x70]'),('mov','dword ptr [rsi + 0x74], ecx'),
        ('cmp','dword ptr [rcx + 0x64], eax'),('test','byte ptr [rcx + 0x68], 0x20')}<=set(pairs(update[move:chats[2]]))
assert 0.5 in floats(n,update)
assert ('cmp','ecx, 0xfa0') in pairs(update[chats[2]:chats[3]])
assert {('mov','byte ptr [rsi + 0x95], 1'),('mov','byte ptr [rbx], 0')}<=set(pairs(update))
for method in ('SetData','MovementInform'):
    body=ins('?'+method+'@npc_nine_cziAI@npc_nine_czi@@')
    gate=next(i for i,o in enumerate(body) if o.mnemonic=='cmp' and '0x586c5' in o.op_str)
    assert {1014,97774,362181}<=values(body[:gate+1]) and body[gate+1].mnemonic=='je'
    branch=reachable(body,body[gate+1].operands[0].imm)
    for forbidden in ('?AddDelayedEvent@FunctionProcessor@@','?UpdateAchievementCriteria@Player@@','?Talk@CreatureAI@@','?SetHomePosition@Creature@@'):
        assert all(body[i].address not in branch for i in sites(n,body,forbidden))
for body in (death,update,request,valid,current,actor,leaf):
    for forbidden in ('?FindNearestPlayer@WorldObject@@','?CompleteQuest@Player@@','?AddItem@Player@@','?KilledMonsterCredit@Player@@','?TeleportTo@Player@@','?AddDelayedEvent@Unit@@','?AddDelayedEvent@FunctionProcessor@@'):
        assert not sites(n,body,forbidden)
print('PASS registered498 instance owns initial Nine state at490; genuine97968/362187 owner/pet/guardian death, original full GUID/weak12103/Scenario identity, actual46734/29046 receipt, native3s/7s point1 and exact issued spline completion/pose before4s readiness; scoped legacy bypass and retained late/Chen checks. Client/full943/all36 remain separate.')
print(json.dumps({'world_sha256':hashlib.sha256(n['binary']).hexdigest(),'instance_size':0x498,'owned_state_offset':0x490,'event':46734,'criterion':29046}))

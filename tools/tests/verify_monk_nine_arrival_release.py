"""Actual registered late Nine arrival, original visitor and native receipts.

Selected native PE checks complement the full extracted ASAN/current-world
catalog, not actual client flight, text delivery or all943/all36 traversal.
"""
from pathlib import Path
import hashlib,json,runpy
from sharasdal_erunak_entry_pe import sites,values,fields,floats
from monk_intro_instance_pe import reachable as constructor_reachable

p=runpy.run_path(str(Path(__file__).with_name('verify_monk_chen_early_presentation_release.py')))
n,ins,needs,reachable=(p[k] for k in ('n','ins','needs','reachable'))
pairs=lambda body:[(o.mnemonic,o.op_str) for o in body]
def state(name):return '?'+name+'@State@MonkNineArrival@@'
def context(name):return '?'+name+'@State@MonkChenRescue@@'
assert p['p']['nine_successor'],'Real outlined490 constructor required'
dispatch=ins('?Update@instance_monk_intro@@');request=ins('?SetGuidData@instance_monk_intro@@')
first,=sites(n,dispatch,state('Update'));chen,=sites(n,dispatch,'?UpdateRescue@State@MonkChenPresentation@@')
assert first<chen and ('add','rcx, 0x430') in pairs(dispatch[:first])
assert 94360 in values(request) and ('lea','rcx, [rbp + 0x430]') in pairs(request)
assert len(sites(n,request,state('Request')))==1
nine_call,=sites(n,request,state('Request'));rp=pairs(request)
base=rp.index(('mov','rbp, rcx'));other=rp.index(('add','rbp, 0x3b8'));gate=rp.index(('cmp','edi, 0x17098'))
assert request[nine_call].address not in constructor_reachable(request,request[0].address,{request[base].address})
assert request[nine_call].address not in constructor_reachable(request,request[other].address)
assert request[gate+1].mnemonic=='jne' and request[nine_call].address not in constructor_reachable(request,request[gate+1].operands[0].imm)
assert rp[gate+2:gate+6]==[('movups','xmm0, xmmword ptr [rsi]'),('lea','rcx, [rbp + 0x430]'),('lea','rdx, [rsp + 0x20]'),('movaps','xmmword ptr [rsp + 0x20], xmm0')]
created=ins('?OnCreatureCreate@instance_monk_intro@@')
assert {1014,97774,362252}<=values(created)
needs(created,context('Retained'),state('Cancel'),'?GetGuidValue@Object@@')
assert ('movups','xmmword ptr [rdi + 0x10], xmm0') in pairs(created)
actor=ins(state('Actor'));leaf=ins(state('Leaf'));current=ins(state('Current'))
needs(actor,'?GetCreature@Map@@','?IsInWorld@Object@@','?IsPreDelete@Object@@')
assert {1014,97774,362252}<=values(actor) and 1736.0 in floats(n,actor)
assert {('lea','rdx, [rdi + 0x10]'),('cmp','qword ptr [rbx + 8], rcx'),('cmp','dword ptr [rbx + 0xa18], 0')}<=set(pairs(actor))
needs(leaf,context('Progress'),'?GetAchievementMgr@Scenario@@','?GetCriteriaProgressMap@')
assert 29069 in values(leaf) and ('cmp','qword ptr [rcx + 0x28], 0') in pairs(leaf)
needs(current,context('Retained'),'?IsInWorld@Object@@','?CanContact@Player@@','?InSamePhase@WorldObject@@','?ShouldHideFor@WorldObject@@','?canSeeOrDetect@WorldObject@@')
assert {0xa18,0x4ff0,0x6848,0x6849,0x6851}<=fields(current)
assert not sites(n,current,'?IsWithinDistInMap@WorldObject@@'),'Admission7yd must not truncate native later SAY range'
capture=ins(state('Request'));update=ins(state('Update'))
for body in (capture,update):
    needs(body,context('Retained'),context('Progress'),state('Actor'),'?GetCurrentStep@Scenario@@',
          '?IsInWorld@Object@@','?CanContact@Player@@','?InSamePhase@WorldObject@@','?ShouldHideFor@WorldObject@@',
          '?canSeeOrDetect@WorldObject@@','?IsWithinDistInMap@WorldObject@@','?IsWithinLOSInMap@WorldObject@@')
    assert 7.0 in floats(n,body)
needs(capture,'?FindPlayer@ObjectAccessor@@','?getQuestStatus@Player@@','?AddQuestDelayedEvent@Player@@',state('Update'),state('Cancel'))
assert 12103 in values(capture)
assert {('movups','xmm0, xmmword ptr [r15]'),('lock inc','dword ptr [rax + 0xc]'),
        ('movups','xmmword ptr [rbx + 0x20], xmm0'),('mov','qword ptr [rbx + 8], r14'),
        ('mov','dword ptr [rbx + 0x50], eax'),('mov','byte ptr [rbx + 0x54], 1'),('inc','qword ptr [rbx + 0x40]')}<=set(pairs(capture))
needs(update,state('Leaf'),state('Current'),state('Cancel'),'?getMSTime@@','?UpdateAchievementCriteria@Player@@','?SendChat@CreatureTextMgr@@','?DespawnOrUnsummon@Creature@@','?outMessage@Log@@')
assert {46902,92,2000,4000,3000}<=values(update)
assert {('cmp','byte ptr [rcx + 0x57], 0'),('cmp','byte ptr [rcx + 0x56], 0'),
        ('cmp','rax, qword ptr [r14 + 8]'),('cmp','eax, dword ptr [r14 + 0x50]'),
        ('mov','byte ptr [rbx], 1'),('mov','byte ptr [rbx], 0'),('mov','byte ptr [r14 + 0x58], 1'),
        ('mov','dword ptr [r14 + 0x4c], eax'),('mov','dword ptr [r14 + 0x48], eax'),
        ('mov','byte ptr [r14 + 0x56], 1'),('cmp','ecx, 0x7d0'),('cmp','ecx, 0xfa0')}<=set(pairs(update))
event,=sites(n,update,'?UpdateAchievementCriteria@Player@@');chats=sites(n,update,'?SendChat@CreatureTextMgr@@');despawn,=sites(n,update,'?DespawnOrUnsummon@Creature@@')
assert len(chats)==2 and event<chats[0]<chats[1]<despawn
assert {('mov','edx, 0x5c'),('mov','r8d, 0xb736'),('mov','rcx, rsi')}<=set(pairs(update[event-9:event]))
assert {('cmp','eax, 2'),('cmp','ecx, 0x7d0')}<=set(pairs(update[:event]))
assert event<sites(n,update,state('Leaf'))[-1]<chats[0]
assert ('mov','word ptr [r14 + 0x54], 0x100') in pairs(update[event:chats[0]])
for at,line in zip(chats,(7,8)):assert ('mov',f'r8b, {line}') in pairs(update[at-6:at])
assert ('mov','edx, 0xbb8') in pairs(update[despawn-6:despawn])
valid_names=[name for name in n['symbols'] if name.startswith('??R<lambda_1>@') and '??Update@State@MonkNineArrival@@' in name]
assert len(valid_names)==1;valid_name=valid_names[0];valid=ins(valid_name)
checks=sites(n,update,valid_name);assert len(checks)==3 and event<checks[0]<chats[0]<checks[1]<chats[1]<checks[2]<despawn
needs(valid,context('Progress'),context('Retained'),state('Actor'),state('Leaf'),'??8ObjectGuid@@','?ShouldHideFor@WorldObject@@')
assert {('cmp','qword ptr [rcx + 0x40], rdx'),('cmp','rax, qword ptr [rbx + 8]'),('cmp','rdi, qword ptr [rax + 8]')}<=set(pairs(valid))
los=ins('?MoveInLineOfSight@npc_nine_cziAI@npc_nine_czi@@')
guard=next(i for i,o in enumerate(los) if (o.mnemonic,o.op_str)==('cmp','qword ptr [rdi + 0x3a88], 0x5870c'))
assert {1014,97774,362252}<=values(los[:guard+1]) and los[guard+1].mnemonic=='jne'
scope=reachable(los,los[guard+2].address)
for forbidden in ('?FindNearestPlayer@WorldObject@@','?UpdateAchievementCriteria@Player@@','?AddDelayedEvent@FunctionProcessor@@'):
    assert all(los[i].address not in scope for i in sites(n,los,forbidden)),('Scoped source bypasses legacy nearest/queue',forbidden)
assert {('mov','rcx, rsi'),('mov','edx, 0x17098')}<=set(pairs(los[guard:guard+22]))
for body in (capture,update,actor,current,leaf,valid):
    for forbidden in ('?FindNearestPlayer@WorldObject@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@','?ActivateTaxiPathTo@Player@@','?MovePoint@MotionMaster@@','?AddDelayedEvent@Unit@@','?AddDelayedEvent@FunctionProcessor@@'):
        assert not sites(n,body,forbidden),('No invented travel/progression or foreign AI callback',forbidden)
print('PASS actual registered490 instance/430 Nine state: outlined constructor and retained context offsets, real visitor/full original source+weak quest/scenario identity, native2/29069/46902 before personal7,2000ms retry,4s personal8/native3000ms despawn, once/reentry and scoped legacy bypass. Native taxi/client/full943/all36 remain separate.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'instance_size':0x490,'state_offset':0x430,'event':46902,'criterion':29069,'callback_ms':4000,'native_despawn_ms':3000}))

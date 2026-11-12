from linked_player_abi import inherited_operand
"""Verify actual instance-owned authored8s/15s route handoffs in linked PE."""
from pathlib import Path
import runpy,json,hashlib,re
from sharasdal_erunak_entry_pe import sites,values,fields,floats

p=runpy.run_path(str(Path(__file__).with_name('verify_monk_chen_arrival_release.py')))
n,ins,state,actor,instance,pairs,needs,reachable=(p[k] for k in ('n','ins','state','actor','instance','pairs','needs','reachable'))
assert p['routes_successor'],'Previous arrival binary cannot certify persistent routes'
routes=ins(state('UpdateRoutes'));request=ins(instance('SetGuidData'));created=ins(instance('OnCreatureCreate'))
escort_speech=bool(sites(n,ins(instance('Update')),'?UpdateEscortSpeech@State@MonkChenPresentation@@'))
native_pairs=pairs
if escort_speech:
    actual=native_pairs(routes)
    assert ('mov','rdi, rcx') in actual
    source,=sites(n,routes,state('Actor'));guide_at,=sites(n,routes,'?GetCreature@Map@@')
    assert actual[source+1]==('mov','r13, rax') and actual[guide_at+1]==('mov','r15, rax')
    route_addresses={o.address for o in routes}
    def pairs(body):
        if not body or not all(o.address in route_addresses for o in body):return native_pairs(body)
        # The two actual definitions above prove the context/guide aliases;
        # call addresses, control flow and original instructions stay intact.
        return [(a,re.sub(r'\b(rdi|r15)\b',lambda m:{'rdi':'rsi','r15':'rdi'}[m[0]],b)) for a,b in native_pairs(body)]
assert {94348,94349}<=values(request)
needs(request,state('UpdateRoutes'),'??8ObjectGuid@@')
assert sites(n,request,'??8ObjectGuid@@')[-1]<sites(n,request,state('UpdateRoutes'))[0]
assert {97778,362271,1014}<=values(created) and ('movups','xmmword ptr [rbp + 0x37d], xmm0' if n.get('monk_hait_opening_successor') else 'xmmword ptr [rsi + 0x37d], xmm0' if n.get('monk_initial_successor') else 'xmmword ptr [rdi + 0x37d], xmm0') in pairs(created)
needs(routes,state('Progress'),state('Retained'),state('Leaf'),state('EscortLeaf'),state('Actor'),state('Cancel'),
      '?getMSTime@@','?GetCreature@Map@@','?FindNearestCreature@WorldObject@@','?IsInWorld@Object@@','?IsPreDelete@Object@@',
      '?SendChat@CreatureTextMgr@@','?MoveFollow@MotionMaster@@','?MovePath@MotionMaster@@',state('Routed'))
if not escort_speech:needs(routes,state('Current'))
assert {8000,15000,2000,9859005,9859006,97778,362271,98074,1014}<=values(routes)
assert 50.0 in floats(n,routes)
assert {('cmp','byte ptr [rcx + 0xcd], 0'),('cmp','byte ptr [rcx + 0x93], 0'),
        ('cmp','byte ptr [rcx + 0x95], 0'),('cmp','byte ptr [rcx + 0xce], 0'),
        ('mov','ebx, dword ptr [rsi + 0xc0]'),('mov','ebx, dword ptr [rsi + 0xc4]'),
        ('mov','ebx, dword ptr [rsi + 0xc8]'),('cmp','ecx, 0x1f40'),('cmp','ecx, 0x3a98'),('cmp','ecx, 0x7d0'),
        ('mov','byte ptr [rsi + 0xcf], 1'),('mov','byte ptr [rsi + 0xd0], 1'),
        ('mov','byte ptr [rsi + 0xd1], 1'),('mov','byte ptr [rsi + 0xd2], 1'),
        ('mov',inherited_operand('dword ptr [r13 + 0x3b1c], 0xffffffff')),('mov',inherited_operand('dword ptr [rdi + 0x3b1c], 0xffffffff')),
        ('mov','byte ptr [rsi + 0xce], 0'),('cmp','qword ptr [rdi + 8], rax'),
        ('cmp',inherited_operand('qword ptr [rdi + 0x3a88], 0x5871f')),('cmp','dword ptr [rdi + 0xa18], 0')}<=set(pairs(routes))
paths=sites(n,routes,'?MovePath@MotionMaster@@');assert len(paths)==2
for at,path in zip(paths,(9859005,9859006)):
 assert ('mov',f'edx, {path:#x}') in pairs(routes[at-3:at])
 assert ('xor','r8d, r8d') in pairs(routes[at-4:at])
chat=sites(n,routes,'?SendChat@CreatureTextMgr@@');assert len(chat)==(2 if escort_speech else 3)
assert [pairs(routes)[i-3] for i in chat]==([('mov','r8b, 0xc'),('mov','r8b, 0xd')] if escort_speech else [('mov','r8b, 5'),('mov','r8b, 0xc'),('mov','r8b, 0xd')])
if escort_speech:chat=[None]+chat
else:assert chat[0]<paths[0]
assert paths[0]<chat[1]<chat[2]<paths[1]
assert ('movups','xmm0, xmmword ptr [rsi + 0xad]') in pairs(routes[chat[1]-20:chat[1]])
assert ('movups','xmm0, xmmword ptr [rsi + 0xad]') in pairs(routes[chat[2]-20:chat[2]])
# Actual weak quest snapshot is copied before either external text callback;
# no vector iterator survives cancellation/re-entry into the native dispatcher.
assert ('lock inc','dword ptr [rax + 0xc]') in pairs(routes[:chat[1]])
valid_name=next(name for name in n['symbols'] if name.startswith('??R<lambda_2>@?1??UpdateRoutes@State@MonkChenRescue@@'))
valid=ins(valid_name)
needs(valid,state('Progress'),state('Retained'),state('Leaf'),state('EscortLeaf'),state('Cancel'),state('Actor'),'?GetCurrentStep@Scenario@@')
assert 0x60 in fields(valid) and len(sites(n,routes,valid_name))==3
guide_name=next(name for name in n['symbols'] if name.startswith('??R<lambda_4>@?1??UpdateRoutes@State@MonkChenRescue@@'))
guide=ins(guide_name)
needs(guide,valid_name,state('Retained'),'?GetCreature@Map@@','?IsInWorld@Object@@','?IsPreDelete@Object@@','?FindNearestCreature@WorldObject@@')
assert len(sites(n,guide,valid_name))==1
assert {1014,97778,362271}<=values(guide) and 50.0 in floats(n,guide)
assert {('cmp',inherited_operand('qword ptr [rdi + 0x3a88], 0x5871f')),('cmp','dword ptr [rdi + 0xa18], 0'),('cmp','rax, rdi')}<=set(pairs(guide))
checks=sites(n,routes,guide_name);assert len(checks)==3
assert chat[1]<checks[0]<chat[2]<checks[1]<paths[1]<checks[2]
rescue=ins(state('Update'));arrival=ins(state('UpdateArrival'))
assert ('mov','dword ptr [rdi + 0xc0], eax') in pairs(rescue)
assert {('mov','dword ptr [rdi + 0xc4], eax'),('movups','xmmword ptr [rdi + 0xad], xmm0'),('mov','byte ptr [rdi + 0xce], 1')}<=set(pairs(arrival))
needs(rescue,state('UpdateRoutes'))
data=ins('?Data@MonkChenRoutes@@')
needs(data,'?GetPathScript@WaypointMgr@@');assert {9859006,0x18c,0x2c}<=values(data)
if sites(n,routes,'?Issued@MonkChenRoutes@@'):
 issued=ins('?Issued@MonkChenRoutes@@')
 assert {('cmp','dword ptr [rbx + 0x2c], 0x966fbe'),('cmp','dword ptr [rbx + 0x18], 0'),('cmp',inherited_operand('dword ptr [rdi + 0x3b1c], -1'))}<=set(pairs(issued))
else:
 issued=routes[paths[1]+1:]
 needs(issued,'?GetMotionSlot@MotionMaster@@','?GetPathScript@WaypointMgr@@')
 assert {('sub','rax, 0x18c'),('cmp','rax, 0x2c'),('cmp','al, 2')}<=set(pairs(issued))
 consumed=next(op.address for op,pair in zip(issued,pairs(issued)) if pair==('mov','byte ptr [rsi + 0xce], 0'))
 for condition in ('dword ptr [r12 + 0x2c], 0x966fbe','dword ptr [r12 + 0x18], 0',inherited_operand('dword ptr [rdi + 0x3b1c], -1')):
  at=next(i for i,pair in enumerate(pairs(issued)) if pair==('cmp',condition))
  assert issued[at+1].mnemonic=='jne' and consumed not in reachable(issued,issued[at+1].operands[0].imm)
for kind,number in (('SetData',3),('MovementInform',5)):
 name=next(key for key in n['symbols'] if key.startswith('?_Do_call@') and '??'+kind+'@npc_monk_chen_porterAI@npc_monk_chen_porter@@' in key and '<lambda_'+str(number)+'>' in key)
 body=ins(name);assert (94348 if kind=='SetData' else 94349) in values(body)
 needs(body,'?GetInstanceScript@WorldObject@@','?GetGuidValue@Object@@')
for body in (routes,valid,request):
 for forbidden in ('?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@','?SummonCreature@'):
  assert not sites(n,body,forbidden)
for text in (b'original Chen9859005 route unavailable',b'original Chen9859005 route was not installed',b'original Hait9859006 route unavailable',b'original Hait9859006 route was not installed'):
 assert text in n['binary']
print('PASS actual instance-owned8s Chen escort and15s original Hait handoff; real weak source/owner clocks, exact original actors/ordinary native paths, independent once5/follow/12/13, copied weak owner and epoch revalidation after text, verified installed generators and real2s retry, no synthetic progression. Retained arrival/rescue/Monk/512 PE; native components/geometry differ from actual client/full943/all36.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'scenario':943,'ordinary_paths':[9859005,9859006],'authored_delays_ms':[8000,15000]}))

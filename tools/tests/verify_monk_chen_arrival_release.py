"""Trace real final-node receipt, native46947 and original owner's outro in PE."""
from pathlib import Path
import runpy,json,hashlib
from sharasdal_erunak_entry_pe import sites,values,fields,floats

p=runpy.run_path(str(Path(__file__).with_name('verify_monk_chen_rescue_release.py')))
n,ins,state,actor,instance,pairs,needs,reachable=(p[k] for k in ('n','ins','state','actor','instance','pairs','needs','reachable'))
assert p['arrival_successor'],'Original rescue binary cannot certify escort arrival'
routes_successor=p['routes_successor']
helper=lambda name:'?'+name+'@MonkChenArrival@@'
status=ins(actor('GetData'));factory=ins('?GetAI@npc_monk_chen_porter@@')
assert [('cmp','edx, 0x17087'),('jne',status[8].op_str),('cmp','byte ptr [rcx + 0x3a], al'),('setne','al'),('inc','eax'),('ret','')]==pairs(status[7:13])
assert ('mov','word ptr [rdi + 0x39], 0') in pairs(factory)
claim=ins(instance('GetGuidData'));request=ins(instance('SetGuidData'))
assert {94344,94346}<=values(claim)
assert {('movups','xmm0, xmmword ptr [rcx + 0x353]'),('movups','xmm0, xmmword ptr [rcx + 0x363]')}<=set(pairs(claim))
assert {94345,94347,9859005,5}<=values(request)
issued=ins(state('Routed')) if routes_successor else request
assert 4 in values(issued)
needs(issued,state('Retained'),state('Leaf'),state('Actor'),'?GetMotionSlot@MotionMaster@@')
needs(request,helper('Pose'),state('UpdateArrival'),'??8ObjectGuid@@','??9ObjectGuid@@')
assert {('cmp','dword ptr [rbx + 0x2c], 0x966fbd'),('cmp','dword ptr [rbx + 0x18], 0'),
        ('cmp','dword ptr [rsi + 0x3b1c], -1'),('mov','byte ptr [rdi + 0x93], 1')}<=set(pairs(issued))
assert {('cmp','dword ptr [rdi + 0x18], 5'),('cmp','qword ptr [rsi + rax*8 + 0x2f98], rdi'),
        ('mov','byte ptr [rbx + 0x94], 1')}<=set(pairs(request))
data=ins(helper('Data'));pose=ins(helper('Pose'))
needs(data,'?instance@WaypointMgr@@');assert 9859005 in values(data)
# Six44-byte WaypointData entries in the native script-path store, not regular
# waypoints: sub264/cmp44 is the compiler's six-element size check.
assert {('sub','rcx, 0x108'),('cmp','rcx, 0x2c')}<=set(pairs(data))
if routes_successor:
    needs(data,'?GetPathScript@WaypointMgr@@')
    from monk_script_path_pe import script_path_lookup
    script_path_lookup(n)
    needs(pose,'?GetPathScript@WaypointMgr@@')
    assert {('sub','rdx, 0x108'),('cmp','rdx, 0x2c')}<=set(pairs(pose)) and 9859005 in values(pose)
else:
    assert {0x48,0x50,0x58,0x70}<=fields(data)
    needs(pose,helper('Data'))
needs(pose,'?IsWithinDist3d@WorldObject@@')
assert 0.5 in floats(n,pose) and ('test','byte ptr [rcx + 0x68], 0x20') in pairs(pose)
assert {('movss','xmm3, dword ptr [rax - 0x20]'),('movss','xmm2, dword ptr [rax - 0x24]'),('movss','xmm1, dword ptr [rax - 0x28]')}<=set(pairs(pose))
update=ins(state('UpdateArrival'));leaf=ins(state('EscortLeaf'))
assert 29070 in values(leaf);needs(leaf,state('Progress'),'?GetAchievementMgr@Scenario@@','?GetCriteriaProgressMap@')
needs(update,state('Progress'),state('Retained'),state('Current'),state('Actor'),state('Content'),state('EscortLeaf'),state('Cancel'),helper('Pose'),'?getMSTime@@','?outMessage@Log@@')
needs(update,'?GetPathScript@WaypointMgr@@' if routes_successor else helper('Data'))
assert {46947,94343,92,2000,4,5,9859005}<=values(update)
assert {('cmp','dword ptr [rax + 0x3b1c], 5'),('mov','byte ptr [rdi + 0x94], 1'),
        ('cmp','eax, dword ptr [rdi + 0x6c]'),('cmp','ecx, 0x7d0'),
        ('cmovbe','ecx, edx'),('lea','ecx, [rdx - 1]'),('mov','dword ptr [rdi + 0x98], eax'),
        ('movups','xmmword ptr [rdi + 0x73], xmm0'),('movups','xmmword ptr [rdi + 0x83], xmm0'),
        ('cmp','qword ptr [rdi + 0x60], r15'),('mov','byte ptr [rdi + 0x95], 1')}<=set(pairs(update))
event,=sites(n,update,'?UpdateAchievementCriteria@Player@@')
assert {('mov','edx, 0x5c'),('mov','r8d, 0xb763')}<=set(pairs(update[event-5:event]))
assert sites(n,update,state('Current'))[0]<sites(n,update,state('EscortLeaf'))[0]<event
dispatch,=[i for i,op in enumerate(update) if (op.mnemonic,op.op_str)==('call','qword ptr [rax + 0x1a8]')]
assert pairs(update[dispatch-3:dispatch])==[('mov','edx, 2'),('mov','r8d, 5'),('mov','rcx, r15')]
assert event<dispatch and sites(n,update,state('EscortLeaf'))[-1]<dispatch
assert ('cmp','eax, 2') in pairs(update[dispatch:])
movement=ins(actor('MovementInform'));talks=sites(n,movement,'?Talk@CreatureAI@@')
guard=next(i for i,o in enumerate(movement) if (o.mnemonic,o.op_str)==('cmp','byte ptr [rdi + 0x3a], 0'))
assert movement[guard+1].mnemonic=='jne' and movement[talks[1]].address not in reachable(movement,movement[guard+1].operands[0].imm)
assert ('mov','byte ptr [rdi + 0x3a], 1') in pairs(movement[guard:talks[1]])
equal,=sites(n,movement,'??9ObjectGuid@@');assert movement[equal+2].mnemonic=='jne'
assert movement[talks[1]].address not in reachable(movement,movement[equal+2].operands[0].imm)
assert {94344,94345,94346,46947}<=values(movement)
credit,=sites(n,movement,'?UpdateAchievementCriteria@Player@@')
source_guard=next(i for i,o in enumerate(movement[talks[1]:credit],talks[1]) if (o.mnemonic,o.op_str)==('cmp','qword ptr [rbx + 0x3a88], 0x586d9'))
assert movement[source_guard+1].mnemonic=='je' and movement[source_guard+1].operands[0].imm==movement[credit+1].address
route=next(name for name in n['symbols'] if name.startswith('?_Do_call@') and '??SetData@npc_monk_chen_porterAI@npc_monk_chen_porter@@' in name and '<lambda_3>' in name)
route=ins(route);needs(route,'?MovePath@MotionMaster@@','?GetInstanceScript@WorldObject@@')
if routes_successor:
    assert {9859005,94348}<=values(route)
    actual_routes=ins(state('UpdateRoutes'))
    needs(actual_routes,'?MovePath@MotionMaster@@',state('Routed'))
    assert ('mov','dword ptr [r13 + 0x3b1c], 0xffffffff') in pairs(actual_routes)
else:
    assert {9859005,94347}<=values(route) and ('mov','dword ptr [rcx + 0x3b1c], 0xffffffff') in pairs(route)
for body in (update,movement,request):
 for forbidden in ('?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@'):
  assert not sites(n,body,forbidden)
assert b'Monk Chen: real9859005 last-node arrival46947/outro not acknowledged; retaining original quest attempts' in n['binary']
print('PASS actual registered Chen arrival: issued native9859005, native zero-based node5/persistent receipt, finalized spline/native endpoint0.5, original weak owner, actual46947/29070 then5, full GUID claim, once-before-six-original callbacks, epoch/monotonic-stage revalidation and real2000ms retry. Retained rescue/other Monk/512 native paths verified separately from client/full943.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'scenario':943,'event':46947,'criterion':29070,'native_final_node':5}))

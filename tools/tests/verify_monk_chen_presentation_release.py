"""Registered native Chen outro ownership and original before/after ordering.

Run retained full Chen/Jorvinax/Fel Stone inspectors against this same hash.
Native components/PE/geometry are distinct from actual client presentation.
"""
from pathlib import Path
import hashlib,json,runpy
from capstone import CS_OP_IMM
from sharasdal_erunak_entry_pe import sites,values,fields,floats

p=runpy.run_path(str(Path(__file__).with_name('verify_monk_nine_czi_timers_release.py')))
n,ins,refs,table,resolved=(p[k] for k in ('n','ins','refs','table','resolved'))
def presentation(name):return '?'+name+'@State@MonkChenPresentation@@'
def context(name):return '?'+name+'@State@MonkChenRescue@@'
def pairs(body):return [(o.mnemonic,o.op_str) for o in body]
def needs(body,*targets):
    for target in targets:assert sites(n,body,target),('Native Chen presentation dependency',target)
def reachable(body,start):
    positions={o.address:i for i,o in enumerate(body)};pending=[start];seen=set()
    while pending:
        address=pending.pop()
        if address in seen or address not in positions:continue
        seen.add(address);o=body[positions[address]]
        if o.mnemonic in ('ret','int3','ud2'):continue
        if o.mnemonic.startswith('j') and o.operands[0].type==CS_OP_IMM:pending.append(o.operands[0].imm)
        if o.mnemonic!='jmp':pending.append(o.address+o.size)
    return seen

factory=ins('?GetInstanceScript@?$GenericInstanceMapScript@Uinstance_monk_intro@@')
vt,raw=table('??_7instance_monk_intro@@6B@',140)
loader,lraw=table('??_7?$GenericInstanceMapScript@Uinstance_monk_intro@@@@6BScriptObject@@@',2)
assert vt in refs(factory) and loader in refs(ins('?AddSC_instance_class_art_scenarios@@'))
assert n['symbol']('?GetInstanceScript@?$GenericInstanceMapScript@Uinstance_monk_intro@@')==resolved(lraw)[1]
assert 0x430 in values(factory)
assert {('lea','rdi, [rsi + 0x2e0]'),('mov','qword ptr [rsi + 0x3b8], rdi'),
        ('mov','qword ptr [rsi + 0x418], rdi'),('mov','qword ptr [rsi + 0x420], rbx'),
        (('mov','dword ptr [rsi + 0x428], ebx') if sites(n,ins('?Update@instance_monk_intro@@'),'?UpdateRescue@State@MonkChenPresentation@@') else ('mov','word ptr [rsi + 0x428], bx'))}<=set(pairs(factory))
dispatch=ins('?Update@instance_monk_intro@@')
assert n['symbol']('?Update@instance_monk_intro@@') in resolved(raw)
before,after=sites(n,dispatch,presentation('Update'))
rescue,=sites(n,dispatch,context('Update'));boss,=sites(n,dispatch,'?Update@State@MonkJorvinax@@')
assert before<rescue<boss<after and dispatch[after].mnemonic=='jmp'
assert {('xor','r8d, r8d'),(('lea','rcx, [rbx + 0x418]') if sites(n,dispatch,presentation('UpdateRescue')) else ('add','rcx, 0x418'))}<=set(pairs(dispatch[:before]))
assert {('mov','r8b, 1'),('lea','rcx, [rbx + 0x418]')}<=set(pairs(dispatch[boss:after]))

owner=ins(presentation('Owner'));guide=ins(presentation('Guide'));update=ins(presentation('Update'))
needs(owner,context('Progress'),context('EscortStage'),context('EscortLeaf'),context('Current'),'??8ObjectGuid@@')
assert {('cmp','byte ptr [rcx + 0x71], 0'),('cmp','byte ptr [rcx + 0x95], 0'),('add','rcx, 0xad')}<=set(pairs(owner))
needs(guide,'?GetCreature@Map@@','?IsInWorld@Object@@','?IsPreDelete@Object@@','?FindNearestCreature@WorldObject@@','?InSamePhase@WorldObject@@','?ShouldHideFor@WorldObject@@')
assert {1014,97778,362271}<=values(guide) and 50.0 in floats(n,guide)
assert {('lea','rdx, [rcx + 0x9d]'),('cmp','dword ptr [rbx + 0xa18], 0'),('cmp','rax, rbx')}<=set(pairs(guide))
needs(update,context('Actor'),context('Progress'),context('EscortStage'),context('EscortLeaf'),context('Current'),presentation('Owner'),presentation('Guide'),'?getMSTime@@','?SendChat@CreatureTextMgr@@','?MovePoint@MotionMaster@@')
assert {('cmp','byte ptr [rcx + 0x11], 0'),('mov','qword ptr [rsi + 8], rcx'),('mov','byte ptr [rsi + 0x10], 0'),
        ('mov','byte ptr [rbx], 1'),('mov','byte ptr [rbx], 0'),('mov','edi, dword ptr [rdi + 0xc4]'),
        ('lea','r15d, [rcx - 1]'),('cmovbe','r15d, ecx'),('lock inc','dword ptr [rax + 0xc]'),
        ('movups','xmm0, xmmword ptr [rbx]'),('movups','xmmword ptr [rbp - 0x29], xmm0')}<=set(pairs(update))
assert {3000,7000,11000,16000}<=values(update)
assert {3837.48,1729.83,946.42}<=floats(n,update)
point,=sites(n,update,'?MovePoint@MotionMaster@@');chats=sites(n,update,'?SendChat@CreatureTextMgr@@')
assert len(chats)==3
assert {('or','al, 8'),('mov','byte ptr [rsi + 0x10], al'),('xor','edx, edx'),('mov','byte ptr [rsp + 0x28], 1')}<=set(pairs(update[point-14:point]))
for at,line,bit,deadline in zip(chats,(10,8,11),(1,2,4),(3000,7000,11000)):
    assert ('mov',f'r8b, {line:#x}' if line!=8 else 'r8b, 8') in pairs(update[at-5:at])
    start=max(i for i,o in enumerate(update[:at]) if (o.mnemonic,o.op_str)==('or',f'al, {bit}'))
    assert ('mov','byte ptr [rsi + 0x10], al') in pairs(update[start:at])
    assert ('cmp',f'r15d, {deadline:#x}') in pairs(update[:start])
    if line!=10:assert ('movups','xmm0, xmmword ptr [rbp - 0x29]') in pairs(update[start:at])
assert {('cmp','byte ptr [rax + 0xd1], 0'),('or','byte ptr [rsi + 0x10], 7')}<=set(pairs(update))
phase=next(i for i,o in enumerate(update) if (o.mnemonic,o.op_str)==('cmp','byte ptr [rbp + 0x77], 0'))
assert update[phase+1].mnemonic=='je'
assert update[point].address not in reachable(update,update[phase+1].operands[0].imm)
assert all(update[i].address not in reachable(update,update[phase+2].address) for i in chats)
valid_prefix='??R<lambda_2>@?1??Update@State@MonkChenPresentation@@'
assert len(sites(n,update,valid_prefix))==3
valid=ins(valid_prefix);needs(valid,context('Actor'),presentation('Owner'),'??8ObjectGuid@@')

# Every legacy queue remains native for other actors, but the exact scoped
# source equality branch must bypass each one, including both15s wrappers.
movement=ins('?MovementInform@npc_monk_chen_porterAI@npc_monk_chen_porter@@')
queues=sites(n,movement,'?AddDelayedEvent@FunctionProcessor@@');assert len(queues)==6
for at in queues:
    guard=max(i for i,o in enumerate(movement[:at]) if o.mnemonic=='cmp' and o.op_str.endswith(' + 0x3a88], 0x586d9'))
    assert {1014,103148,362201}<=values(movement[max(0,guard-14):guard+1])
    branch=movement[guard+1];assert branch.mnemonic in ('je','jne')
    equal=branch.operands[0].imm if branch.mnemonic=='je' else branch.address+branch.size
    assert movement[at].address not in reachable(movement,equal)
for body in (update,owner,guide,valid):
    for forbidden in ('?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@','?AddDelayedEvent@Unit@@','?AddDelayedEvent@FunctionProcessor@@'):
        assert not sites(n,body,forbidden),('No synthetic progress or actor-owned timer',forbidden)
print('PASS actual registered430 instance/418 presentation state: original context and Jorvinax offsets retained; native before-route3/7/11 and after-route16 dispatch, original weak owner/full source+guide, exact clocks and once/reentry flags, stale greeting retirement and unscoped six native queues. Point0 uses original normal path command, never arrival/progression credit. Client/full943 remain separate.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'instance_size':0x430,'state_offset':0x418,'authored_deadlines_ms':[3000,7000,11000,16000]}))

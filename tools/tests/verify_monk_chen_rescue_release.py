from linked_player_abi import inherited_member_offset
from linked_player_abi import inherited_operand
from linked_player_abi import player_offset,player_operand
"""Verify actual registered Morvat receipt, original quest and Chen handoff."""
from pathlib import Path
from monk_intro_instance_pe import construction,zero_alias_pairs
import hashlib,json,runpy
from capstone import CS_OP_IMM,CS_OP_MEM
from sharasdal_erunak_entry_pe import sites,values,fields,floats

p=runpy.run_path(str(Path(__file__).with_name('verify_monk_hait_click_release.py')))
n,refs,table,resolved=(p[k] for k in ('n','refs','table','resolved'))
ins=n['instructions']
def state(name):return '?'+name+'@State@MonkChenRescue@@'
def actor(name):return '?'+name+'@npc_monk_chen_porterAI@npc_monk_chen_porter@@'
def instance(name):return '?'+name+'@instance_monk_intro@@'
def pairs(body):return zero_alias_pairs(body) if body is factory and nine_successor else [(op.mnemonic,op.op_str) for op in body]
def needs(body,*names):
    for name in names:assert sites(n,body,name),('Native Chen dependency',name)
def reachable(body,start):
    positions={op.address:i for i,op in enumerate(body)};pending=[start];seen=set()
    while pending:
        address=pending.pop()
        if address in seen or address not in positions:continue
        seen.add(address);op=body[positions[address]]
        if op.mnemonic in ('ret','int3','ud2'):continue
        if op.mnemonic.startswith('j') and op.operands[0].type==CS_OP_IMM:pending.append(op.operands[0].imm)
        if op.mnemonic!='jmp':pending.append(op.address+op.size)
    return seen

vt,raw=table('??_7instance_monk_intro@@6B@',140);slots=resolved(raw)
allocation,factory,nine_successor=construction(n)
script,sraw=table('??_7?$GenericInstanceMapScript@Uinstance_monk_intro@@@@6BScriptObject@@@',2)
assert vt in refs(factory) and script in refs(ins('?AddSC_instance_class_art_scenarios@@'))
assert n['symbol']('?GetInstanceScript@?$GenericInstanceMapScript@Uinstance_monk_intro@@')==resolved(sraw)[1]
presentation_successor=0x430 in values(allocation) or nine_successor
jorvinax_successor=0x418 in values(factory) or presentation_successor
if presentation_successor:
    assert {('mov','qword ptr [rsi + 0x418], rdi'),('mov','qword ptr [rsi + 0x420], rbx'),(('mov','qword ptr [rsi + 0x428], rbx') if sites(n,ins('?Update@instance_monk_intro@@'),'?UpdateEscortSpeech@State@MonkChenPresentation@@') else ('mov','dword ptr [rsi + 0x428], ebx') if sites(n,ins('?Update@instance_monk_intro@@'),'?UpdateRescue@State@MonkChenPresentation@@') else ('mov','word ptr [rsi + 0x428], bx'))}<=set(pairs(factory))
routes_successor=0x3b8 in values(factory) or jorvinax_successor
arrival_successor=0x380 in values(factory) or routes_successor
if arrival_successor:
    # Arrival successor appends packed GUID receipts and a real-time retry;
    # the original rescue prefix remains at precisely the same offsets.
    assert {('lea','rdi, [rsi + 0x2e0]'),('mov','qword ptr [rdi], rbx'),
            ('xor','ebx, ebx'),('mov','word ptr [rdi + 0x70], bx'),
            ('mov','byte ptr [rdi + 0x72], bl'),('lea','rcx, [rdi + 0x73]'),
            ('lea','rcx, [rdi + 0x83]'),('mov','dword ptr [rdi + 0x93], ebx'),
            ('mov','dword ptr [rdi + 0x98], ebx'),('mov','byte ptr [rdi + 0x9c], bl')}<=set(pairs(factory))
    if routes_successor:
        assert {('lea','rcx, [rdi + 0x9d]'),('lea','rcx, [rdi + 0xad]'),
                ('mov','qword ptr [rdi + 0xc0], rbx'),('mov','qword ptr [rdi + 0xc8], rbx'),
                ('mov','dword ptr [rdi + 0xd0], ebx'),('mov','word ptr [rdi + 0xd4], bx') if jorvinax_successor else ('mov','byte ptr [rdi + 0xd4], bl')}<=set(pairs(factory))
else:
    assert {0x358,0x2e0,0x350}<=values(factory)|fields(factory)
    assert ('mov','qword ptr [rdi + 0x2e0], rbx') in pairs(factory)
    assert {('xor','eax, eax'),('mov','word ptr [rdi + 0x350], ax'),('mov','byte ptr [rdi + 0x352], al')}<=set(pairs(factory))
assert slots[18]==n['symbol'](instance('GetGuidData')) and slots[19]==n['symbol'](instance('SetGuidData'))
for name in ('OnCreatureCreate','CreatureDies','Update'):assert n['symbol'](instance(name)) in slots
for name,target in (('CreatureDies','Death'),('Update','Update')):
    body=ins(instance(name))
    if jorvinax_successor:
        first,=sites(n,body,state(target));second,=sites(n,body,'?'+target+'@State@MonkJorvinax@@')
        context_argument=(('lea','rcx, [rdi + 0x2e0]') if name=='Update' else ('lea','rcx, [rbx + 0x2e0]')) if n.get('monk_initial_successor') else ('lea','rcx, [rbx + 0x2e0]') if presentation_successor and name=='Update' else ('add','rcx, 0x2e0')
        later_argument=('lea','rcx, [rdi + 0x3b8]' if n.get('monk_initial_successor') and name=='Update' else 'rcx, [rbx + 0x3b8]')
        if n.get('monk_hait_defeat_successor') and name=='CreatureDies':
            # The new inlined first-Demon guard retains the same instance in
            # RBP; both existing consumers still receive their original fields.
            binding=pairs(body).index(('mov','rbp, rcx'))
            argument=pairs(body).index(('lea','rcx, [rbp + 0x3b8]'))
            assert all(not o.op_str.startswith(('rbp,','ebp,','bp,','bpl,')) for o in body[binding+1:argument])
            # The tail call restores the caller's RBP only after materializing
            # RCX; no write may change that retained argument before the jump.
            assert all(not o.op_str.startswith(('rcx,','ecx,','cx,','cl,')) for o in body[argument+1:second])
            context_argument=('lea','rcx, [rbp + 0x2e0]')
            later_argument=('lea','rcx, [rbp + 0x3b8]')
        assert first<second and context_argument in pairs(body[:first]) and later_argument in pairs(body[first:second])
        if presentation_successor and name=='Update':
            before,after=sites(n,body,'?Update@State@MonkChenPresentation@@')
            assert before<first<second<after and body[after].mnemonic=='jmp'
            assert ('xor','r8d, r8d') in pairs(body[:before]) and ('mov','r8b, 1') in pairs(body[second:after])
    else:assert pairs(body)[0]==('add','rcx, 0x2e0') and sites(n,body,state(target))==[1]
created=ins(instance('OnCreatureCreate'))
needs(created,'?OnCreatureCreate@InstanceScript@@','?GetGuidValue@Object@@','?Clear@ObjectGuid@@')
assert sites(n,created,'?OnCreatureCreate@InstanceScript@@')==([8] if n.get('monk_initial_successor') else [6] if nine_successor else [5])
if n.get('monk_initial_successor'):
    assert pairs(created[:8])==[('mov','qword ptr [rsp + 8], rbx'),('mov','qword ptr [rsp + 0x10], rbp'),('mov','qword ptr [rsp + 0x18], rsi'),('mov','qword ptr [rsp + 0x20], rdi'),('push','r14'),('sub','rsp, 0x20'),('mov','rbx, rdx'),('mov','rbp, rcx' if n.get('monk_hait_opening_successor') else 'rsi, rcx')]
elif nine_successor:
    assert pairs(created[:6])==[('mov','qword ptr [rsp + 0x18], rbx'),('mov','qword ptr [rsp + 0x20], rsi'),
                               ('push','rdi'),('sub','rsp, 0x20'),('mov','rbx, rdx'),('mov','rdi, rcx')]
assert {1014,103148,362201,97811,362229}<=values(created)
assert ('movups','xmmword ptr [rbp + 0x2e8], xmm0' if n.get('monk_hait_opening_successor') else 'xmmword ptr [rsi + 0x2e8], xmm0' if n.get('monk_initial_successor') else 'xmmword ptr [rdi + 0x2e8], xmm0') in pairs(created)
claim=ins(instance('GetGuidData'));request=ins(instance('SetGuidData'))
assert 94341 in values(claim) and ('movups','xmm0, xmmword ptr [rcx + 0x318]') in pairs(claim)
needs(claim,'?GetGuidData@InstanceScript@@')
needs(request,'??8ObjectGuid@@',state('Update'));assert 94342 in values(request)
assert ('lea','rdx, [rbx + 8]') in pairs(request) and ('xor','edx, edx') in pairs(request)

_,araw=table('??_7npc_monk_chen_porterAI@npc_monk_chen_porter@@6B@',82);aslots=resolved(araw)
chen_factory=ins('?GetAI@npc_monk_chen_porter@@')
if arrival_successor:
    assert {('mov','word ptr [rdi + 0x39], 0'),('mov','byte ptr [rdi + 0x38], 0')}<=set(pairs(chen_factory))
else:
    assert {('mov','byte ptr [rdi + 0x39], 0'),('mov','byte ptr [rdi + 0x38], 0')}<=set(pairs(chen_factory))
assert aslots[10]==n['symbol'](actor('GetData')) and aslots[15]==n['symbol'](actor('SetData'))
status=ins(actor('GetData'))
assert pairs(status)[:7]==[('xor','eax, eax'),('cmp','edx, 0x17084'),('jne',status[2].op_str),
                         ('cmp','byte ptr [rcx + 0x39], al'),('setne','al'),('inc','eax'),('ret','')]
bridge=ins(actor('SetData'));talk,=sites(n,bridge,'?Talk@CreatureAI@@')
needs(bridge,'?GetInstanceScript@WorldObject@@','?GetGuidValue@Object@@','??9ObjectGuid@@')
assert {1014,103148,362201,94341,94342}<=values(bridge)
equal,=sites(n,bridge,'??9ObjectGuid@@')
assert bridge[equal+2].mnemonic=='jne' and bridge[talk].address not in reachable(bridge,bridge[equal+2].operands[0].imm)
flag=next(i for i,op in enumerate(bridge[:talk]) if (op.mnemonic,op.op_str)==('cmp','byte ptr [rdi + 0x39], al'))
assert bridge[flag+1].mnemonic=='jne' and bridge[talk].address not in reachable(bridge,bridge[flag+1].operands[0].imm)
assert ('mov','byte ptr [rdi + 0x39], 1') in pairs(bridge[flag:talk])
assert any(op.mnemonic=='call' and op.op_str=='r9' for op in bridge[:equal])
assert ('mov','r9, qword ptr [rcx + 0x90]') in pairs(bridge[:equal])
assert ('mov','rbx, qword ptr [rax + 0x98]') in pairs(bridge[equal:])
# Only the original unscoped actor uses its old nearest-player credit path.
old_credit,=sites(n,bridge,'?UpdateAchievementCriteria@Player@@')
source_guard=next(i for i,op in enumerate(bridge[talk:old_credit],talk)
                  if op.mnemonic=='cmp' and op.op_str==inherited_operand('qword ptr [rbx + 0x3a88], 0x586d9'))
assert bridge[source_guard+1].mnemonic=='je' and bridge[source_guard+1].operands[0].imm==bridge[old_credit+1].address

progress=ins(state('Progress'));leaf=ins(state('Leaf'));death=ins(state('Death'))
needs(progress,'?GetId@Map@@','?GetInstanceId@Map@@','?GetScenario@ScenarioMgr@@','?GetScenarioId@Scenario@@')
assert {1014,943}<=values(progress) and 0x18ae8 in fields(progress)
needs(leaf,state('Progress'),'?GetAchievementMgr@Scenario@@','?GetCriteriaProgressMap@')
assert 29047 in values(leaf) and ('cmp','qword ptr [rcx + 0x28], 0') in pairs(leaf)
needs(death,state('Progress'),'?GetCurrentStep@Scenario@@','?GetCreature@Map@@','?IsInWorld@Object@@','?IsPreDelete@Object@@',
      '?CanContact@Player@@','?InSamePhase@WorldObject@@','?GetQuestRewardStatus@Player@@','?GetQuestStatus@Player@@',
      '?FindQuestSlot@Player@@','?AddQuestDelayedEvent@Player@@','?getQuestStatus@Player@@',
      '?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@',state('Update'))
assert {97811,362229,12103,3,25,10}<=values(death)
assert ('cmp','rax, rbp') in pairs(death) and ('cmp','dword ptr [rbp + 0xa18], 0') in pairs(death)
assert {('movups','xmmword ptr [r15 + 0x18], xmm0'),('movups','xmmword ptr [r15 + 0x28], xmm0'),
        ('inc','qword ptr [r15 + 0x60]'),('mov','byte ptr [r15 + 0x70], 1')}<=set(pairs(death))
assert sites(n,death,'?AddQuestDelayedEvent@Player@@')[0]<sites(n,death,'?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')[0]<sites(n,death,state('Update'))[0]
retained=ins(state('Retained'));current=ins(state('Current'));source=ins(state('Actor'));content=ins(state('Content'))
needs(retained,'?FindPlayer@ObjectAccessor@@','?getQuestStatus@Player@@','?GetQuestStatus@Player@@','?GetQuestRewardStatus@Player@@','?FindQuestSlot@Player@@')
assert {12103,3,25,10}<=values(retained)
assert ('lock cmpxchg','dword ptr [rdx + 8], ecx') in pairs(retained) and ('cmp','qword ptr [r8 + 0x18], rax') in pairs(retained)
assert ('cmp','qword ptr [rdi + 8], rax') in pairs(retained)
needs(source,'?GetCreature@Map@@','?IsInWorld@Object@@','?IsPreDelete@Object@@')
assert {1014,103148,362201}<=values(source) and ('cmp','dword ptr [rbx + 0xa18], 0') in pairs(source)
needs(current,state('Retained'),state('Actor'),'?GetCreature@Map@@','?IsInWorld@Object@@','?CanContact@Player@@','?InSamePhase@WorldObject@@','?IsWithinDistInMap@WorldObject@@')
assert 100.0 in floats(n,current) and {inherited_member_offset(0x4ff0),player_offset(0x6848),player_offset(0x6849),player_offset(0x6851)}<=fields(current)
if jorvinax_successor:
    needs(current,'?ShouldHideFor@WorldObject@@')
    assert 0x478 in fields(ins('?ShouldHideFor@WorldObject@@'))
else:assert 0x478 in fields(current)
assert {('mov','rcx, qword ptr [rax + 0x9c8]'),('mov','rax, qword ptr [rax + 0x9c0]')}<=set(pairs(content))

update=ins(state('Update'));event,=sites(n,update,'?UpdateAchievementCriteria@Player@@')
needs(update,state('Progress'),state('Retained'),state('Current'),state('Leaf'),state('Content'),state('Actor'),'?GetCurrentStep@Scenario@@','?outMessage@Log@@',state('Cancel'))
assert {2000,94340,46956,92,3,4}<=values(update) and ('mov','dword ptr [rdi + 0x68], 0x7d0') in pairs(update)
assert [('mov','edx, 0x5c'),('mov','r8d, 0xb76c'),('mov','rcx, rbx')]==pairs(update[event-3:event])
assert ('cmp','eax, 3') in pairs(update[event-15:event]) and sites(n,update,state('Leaf'))[0]<event
zero,epoch_register=('r12b','rsi') if arrival_successor else ('0','r14')
assert pairs(update[event+1:event+5])==[('cmp',f'byte ptr [rdi + 0x70], {zero}'),('je',update[event+2].op_str),('cmp',f'qword ptr [rdi + 0x60], {epoch_register}'),('jne',update[event+4].op_str)]
if arrival_successor:assert ('xor','r12d, r12d') in pairs(update[:event])
dispatch,=[i for i,op in enumerate(update) if op.mnemonic=='call' and op.op_str=='qword ptr [rax + 0x78]']
assert event<dispatch and {('cmp','eax, 4'),('cmp','eax, 1'),('mov','edx, 0x17084'),
                         ('movups','xmm0, xmmword ptr [rdi + 8]'),('movups','xmmword ptr [rdi + 0x38], xmm0')}<=set(pairs(update[event:dispatch]))
assert pairs(update[dispatch-3:dispatch])==[('mov','edx, 1'),('mov','r8d, edx'),('mov',f'rcx, {epoch_register}')]
assert {('cmp',f'qword ptr [rdi + 0x60], {epoch_register}'),('cmp','eax, 2'),('mov','word ptr [rdi + 0x70], 0x100')}<=set(pairs(update[dispatch:]))
for body in (death,update,bridge):
    for forbidden in ('?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@'):
        assert not sites(n,body,forbidden)
assert b'Monk Chen: genuine Morvat death46956/escort not acknowledged; retaining original quest attempts' in n['binary']
print('PASS actual registered1014 instance/vtable/state2e0 and original base create/GUID delegation; genuine362229 death/native owner priority/original weak12103 attempt, actual943/3/29047 leaf/46956 then stage4/full GUID claim/current or suspended Chen, native status/once-before-original three timers, epoch revalidation and quest-scoped2s retries. Retained317 NPC/receipt behavior is separate from client/full943/all36.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'scenario':943,'death_spawn':362229,'chen_spawn':362201,'criterion':29047,'event':46956}))

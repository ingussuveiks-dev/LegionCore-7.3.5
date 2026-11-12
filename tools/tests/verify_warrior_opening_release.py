from linked_player_abi import inherited_operand
"""Instance-owned native admission/factory acknowledgement before Ragnvald battle."""
from pathlib import Path
import hashlib,json,runpy
from sharasdal_erunak_entry_pe import sites,values,fields
from warrior_instance_factory_pe import selected_instance_factory,instance_guid_instructions

p=runpy.run_path(str(Path(__file__).with_name('verify_warrior_first_wave_release.py')))
n,ins,refs,table,resolved,reachable=(p[k] for k in ('n','ins','refs','table','resolved','reachable'))
def pairs(body):return [(op.mnemonic,op.op_str) for op in body]
def needs(body,*targets):
    for target in targets:assert sites(n,body,target),('Native opening dependency',target)
scope='instance_broken_shore_InstanceMapScript@instance_broken_shore@@'
opening=lambda name:'?'+name+'@State@WarriorOpening@@'
vt,raw=table('??_7'+scope+'6B@',80);slots=resolved(raw)
assert vt in refs(ins(selected_instance_factory(n,ins)))
for slot,name in ((0x90,'GetGuidData'),(0xb0,'GetData'),(0xb8,'SetData')):
    assert slots[slot//8]==n['symbol']('?'+name+'@'+scope)
update=ins('?Update@'+scope)
first,=sites(n,update,opening('Update'));reinforcement,=sites(n,update,'?Update@State@WarriorReinforcement@@');wave,=sites(n,update,'?Update@State@WarriorFirstWave@@')
assert first<reinforcement<wave
observe=ins('?OnCreatureCreate@'+scope)
assert {97167,361094}<=values(observe)
needs(observe,opening('Retire'),'?GetGuidValue@Object@@')

request=ins('?GetData@'+scope);claim=ins('?SetData@'+scope);guid=instance_guid_instructions(n,ins)
assert {93321,93322}<=values(request) and 93323 in values(claim) and 93324 in values(guid)
assert {('lea','rbx, [rcx + 0x380]'),('mov','eax, dword ptr [rbx + 0x48]')}<=set(pairs(request))
for operand,jump in (('byte ptr [rbx + 0x50], 0','je'),('byte ptr [rbx + 0x52], 0','je'),('byte ptr [rbx + 0x53], 0','jne')):
    at=next(i for i,row in enumerate(request) if row.mnemonic=='cmp' and row.op_str==operand)
    assert request[at+1].mnemonic==jump
    reject=request[at+1].operands[0].imm
    epoch=next(row.address for row in request if row.op_str=='eax, dword ptr [rbx + 0x48]')
    assert epoch not in reachable(request,reject)
needs(request,opening('Admitted'),opening('Current'))
admit,=sites(n,request,opening('Admitted'));current,=sites(n,request,opening('Current'))
assert admit<current and ('xor','edx, edx') in pairs(request[current-3:current])
assert {('cmp','byte ptr [rcx + 0x3d3], 0'),('mov','eax, dword ptr [rcx + 0x3c8]')}<=set(pairs(request))
mark=next(i for i,row in enumerate(claim) if row.mnemonic=='mov' and row.op_str=='byte ptr [rdi + 0x53], 1')
assert pairs(claim[mark+1:mark+2])==[('mov','byte ptr [rdi + 0x50], 0')]
needs(claim[:mark],opening('Admitted'),opening('Current'))
for operand,jump in (('ebx, ebx','je'),('byte ptr [rdi + 0x50], 0','je'),('byte ptr [rdi + 0x52], 0','je'),('byte ptr [rdi + 0x53], 0','jne'),('dword ptr [rdi + 0x48], ebx','jne')):
    at=next(i for i,row in enumerate(claim[:mark]) if row.mnemonic in ('test','cmp') and row.op_str==operand)
    assert claim[at+1].mnemonic==jump and claim[mark].address not in reachable(claim,claim[at+1].operands[0].imm)
assert {('movups','xmm0, xmmword ptr [rcx + 0x388]'),('movups','xmmword ptr [rdx], xmm0')}<=set(pairs(guid))

admitted=ins(opening('Admitted'))
needs(admitted,opening('Progress'),'?GetCurrentStep@Scenario@@','?GetAchievementMgr@Scenario@@')
assert {1,45769}<=values(admitted)
assert ('cmp','qword ptr [rcx + 0x28], 0') in pairs(admitted)
progress=ins(opening('Progress'));assert {1500,933}<=values(progress);needs(progress,'?GetScenario@ScenarioMgr@@')
source=ins(opening('Source'));assert {97167,361094}<=values(source)
needs(source,'?GetCreature@Map@@','?IsInWorld@Object@@','?IsPreDelete@Object@@')
original=ins(opening('Retained'))
needs(original,'?FindPlayer@ObjectAccessor@@','?getQuestStatus@Player@@','?Quest@MalgalorFinale@@','?GetQuestStatus@Player@@','?GetReqKillOrCastCurrentCount@Player@@')
assert {('mov','rdx, qword ptr [rsi + 0x40]'),('lock cmpxchg','dword ptr [rdx + 8], ecx'),
        ('cmp','qword ptr [r8 + 0x18], rax'),('cmp','qword ptr [rdi + 8], rax')}<=set(pairs(original))
assert 103661 in values(original)
needs(ins(opening('Current')),'?Original@MalgalorFinale@@',opening('Source'),opening('Retained'),'?InSamePhase@WorldObject@@')
approach=ins(opening('Approach'))
needs(approach,'?Quest@MalgalorFinale@@','?GetQuestStatus@Player@@','?GetReqKillOrCastCurrentCount@Player@@','?CanContact@Player@@','?IsInWorld@Object@@','?InSamePhase@WorldObject@@','?canSeeOrDetect@WorldObject@@','?HasReactState@Creature@@','?CanSeeEvenInPassiveMode@CreatureAI@@')
assert {1,3,103661,0x460888}<=values(approach)
assert {('test','rbx, rbx'),('cmp','byte ptr [rdx + 0x858], 0'),('cmp','dword ptr [rdi + 0xa18], 0')}<=set(pairs(approach))

speak=ins(opening('Speak'));create,=sites(n,speak,'?CreateConversation@Conversation@@');remove,=sites(n,speak,'?Remove@Conversation@@')
needs(speak,'?IsInWorld@Object@@','?GetConversation@ObjectAccessor@@','?GetCreatures@Conversation@@','?GetCreature@Conversation@@','?canSeeOrDetect@WorldObject@@')
assert pairs(speak[create+1:create+2])==[('test','al, al')] and speak[create+2].mnemonic=='jne'
assert {1386,97166,361091}<=values(speak)
assert ('cmp',inherited_operand('qword ptr [rax + 0x3a88], 0x58283')) in pairs(speak)
view,=sites(n,speak,'?GetCreatures@Conversation@@');actors=sites(n,speak,'?GetCreature@Conversation@@')
assert view<actors[0]<actors[1]
count=next(i for i,row in enumerate(speak[view:actors[0]],view) if row.mnemonic=='cmp' and row.op_str=='rax, 6')
assert speak[count+1].mnemonic=='jae' and speak[remove].address in reachable(speak,speak[count+1].operands[0].imm)
assert {('mov','rbx, qword ptr [r15]'),('mov','rbx, qword ptr [r15 + 8]'),
        ('mov','rbx, qword ptr [r14]'),('mov','rbx, qword ptr [r14 + 8]')}<=set(pairs(speak))
assert len(sites(n,speak,'?GetLowPart@ObjectGuid@@'))==len(sites(n,speak,'?GetHighPart@ObjectGuid@@'))==2
for at in (i for i,row in enumerate(speak) if row.mnemonic=='cmp' and row.op_str=='rbx, rax'):
    assert speak[at+1].mnemonic=='jne' and speak[remove].address in reachable(speak,speak[at+1].operands[0].imm)

body=ins(opening('Update'));event,=sites(n,body,'?UpdateAchievementCriteria@Player@@');dialogue,=sites(n,body,opening('Speak'))
needs(body,opening('Approach'),'?AddQuestDelayedEvent@Player@@',opening('Retained'),opening('Retire'),opening('Current'),opening('Greet'))
assert {46877,1386,93320,(-93320)&0xffffffff,1000}<=values(body)
assert {('movups','xmmword ptr [rbx + 0x18], xmm6'),('mov','dword ptr [rbx + 0x28], r15d'),
        ('mov','dword ptr [rbx + 0x2c], eax'),('lock inc','dword ptr [rcx + 0xc]')}<=set(pairs(body))
assert event<dialogue
assert sites(n,body[event+1:dialogue],opening('Current')) and sites(n,body[event+1:dialogue],opening('Admitted'))
action,=[i for i,row in enumerate(body) if row.mnemonic=='call' and row.op_str=='qword ptr [rax + 0x48]']
assert pairs(body[dialogue+1:dialogue+4])==[('mov','byte ptr [rbx + 0x52], al'),('test','al, al'),('je',body[dialogue+3].op_str)]
assert body[action].address not in reachable(body,body[dialogue+3].operands[0].imm)
assert sites(n,body[dialogue+1:action],opening('Current')) and sites(n,body[dialogue+1:action],opening('Source'))
assert {('mov','edx, 0x16c88'),('call','qword ptr [rax + 0x50]'),('cmp','eax, 1'),('mov','edx, 0xfffe9378')}<=set(pairs(body[dialogue:action]))
greet=ins(opening('Greet'));assert 770 in values(greet);needs(greet,opening('Approach'),opening('Speak'))
for method in (body,speak,claim):
    for forbidden in ('?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@'):
        assert not sites(n,method,forbidden)
print('PASS registered native933 instance-owned opening/source/weak quest attempt, native45769 step1 receipt, original Player/group1386 true/live-map/complete original actor GUID acknowledgement before queue. Native visibility/class/quest/flight/contact/phase gates,40yd770 independent retry, fresh AI after factory, reserved ready/request/claim/started vtable handoff and actual pending/epoch exactly-once CFG verified. Actual Ragnvald2s AI-owned callback retains full original instance/source object identity and payload; no quest/item/teleport grants. Native components/ASAN differ from full933/client traversal.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'native_events':[46877],'native_conversations':[1386,770],'actor_delay_ms':2000,'poll_ms':1000}))

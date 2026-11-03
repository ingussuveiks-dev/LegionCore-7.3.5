"""Registered Chen original4/5 NPC actions, before the native8s route.

Run retained rescue/Jorvinax/Fel Stone inspectors against this same hash.
This selected PE check is separate from native ASAN and client traversal.
"""
from pathlib import Path
import hashlib,json,runpy
from sharasdal_erunak_entry_pe import sites,values,fields,floats

p=runpy.run_path(str(Path(__file__).with_name('verify_monk_chen_presentation_release.py')))
n,ins,pairs,needs,reachable=(p[k] for k in ('n','ins','pairs','needs','reachable'))
pres=p['presentation'];ctx=p['context'];dispatch=p['dispatch'];factory=p['factory']
early=ins(pres('UpdateRescue'));student=ins(pres('Student'))
valid_names=[key for key in n['symbols'] if key.startswith('??R<lambda_1>@') and '??UpdateRescue@State@MonkChenPresentation@@' in key]
assert len(valid_names)==1
valid_name=valid_names[0];valid=ins(valid_name)
at,=sites(n,dispatch,pres('UpdateRescue'));late=sites(n,dispatch,pres('Update'))
assert at<late[0]<sites(n,dispatch,ctx('Update'))[0]<late[1]
assert (('lea','rcx, [rdi + 0x418]' if n.get('monk_initial_successor') else 'rcx, [rbx + 0x418]') if p['nine_successor'] else ('add','rcx, 0x418')) in pairs(dispatch[:at])
escort_speech=p['escort_speech']
assert (('mov','qword ptr [rsi + 0x428], rbx') if escort_speech else ('mov','dword ptr [rsi + 0x428], ebx')) in pairs(factory)
needs(early,ctx('Progress'),ctx('Actor'),ctx('Leaf'),ctx('Retained'),'?GetCurrentStep@Scenario@@',
      '?ShouldHideFor@WorldObject@@','?getMSTime@@','?GetCreatureListWithEntryInGrid@WorldObject@@',
      pres('Student'),'?GetCreature@Map@@','??8ObjectGuid@@','?urand@@','?MoveFollow@MotionMaster@@','?SendChat@CreatureTextMgr@@',valid_name)
assert {('cmp','byte ptr [rcx + 0x13], 0'),('mov','byte ptr [rsi], 1'),('mov','byte ptr [rsi], 0'),
        ('mov','ebx, dword ptr [rbx + 0xc0]'),('lea','edi, [rcx - 1]'),('cmovbe','edi, ecx'),
        ('cmp','eax, 4'),('movups','xmm0, xmmword ptr [rbx + 8]'),
        ('movups','xmmword ptr [rdx], xmm0'),('add','qword ptr [rbp - 0x71], 0x10')}<=set(pairs(early))
assert ({('mov','qword ptr [rcx + 8], rax'),('mov','byte ptr [rcx + 0x10], 0'),('mov','byte ptr [rcx + 0x12], 0'),
         ('and','al, 3'),('cmp','al, 3'),('cmp','byte ptr [rdx + 0x71], 0'),('cmp','byte ptr [rdx + 0x93], 0'),
         ('cmp','byte ptr [rdx + 0x95], 0'),('or','r8b, 2'),('mov','byte ptr [rcx + 0x12], r8b')}
        if escort_speech else {('mov','qword ptr [r12 + 8], rdx'),('mov','byte ptr [r12 + 0x10], 0'),
         ('mov','byte ptr [r12 + 0x12], 0'),('cmp','byte ptr [r12 + 0x12], 3'),('cmp','byte ptr [rcx + 0x71], 0'),
         ('cmp','byte ptr [rcx + 0x93], 0'),('cmp','byte ptr [rcx + 0x95], 0'),('mov','byte ptr [r12 + 0x12], 3')})<=set(pairs(early))
assert {4443,5000,98074}<=values(early) and 70.0 in floats(n,early)
chats=sites(n,early,'?SendChat@CreatureTextMgr@@');follow,=sites(n,early,'?MoveFollow@MotionMaster@@')
maps=sites(n,early,'?GetCreature@Map@@');students=sites(n,early,pres('Student'));valids=sites(n,early,valid_name)
assert len(chats)==2 and len(maps)==2 and len(students)==3 and len(valids)==4
assert chats[0]<valids[0]<students[0]<valids[1]<maps[0]<students[1]<follow<valids[2]<maps[1]<students[2]<chats[1]<valids[3]
for bit,deadline,limit in ((1,4443,chats[0]),(2,5000,students[0])):
    reserve=next(i for i,o in enumerate(early[:limit]) if (o.mnemonic,o.op_str)==('or',f'al, {bit}'))
    assert ('cmp',f'edi, {deadline:#x}') in pairs(early[:reserve])
    assert ('mov','byte ptr [r12 + 0x12], al') in pairs(early[reserve:limit])
assert ('mov','r8b, 4') in pairs(early[chats[0]-5:chats[0]])
assert ('xor','r8d, r8d') in pairs(early[chats[1]-5:chats[1]])
random,=sites(n,early,'?urand@@')
assert {('mov','ecx, 1'),('mov','edx, 4')}<=set(pairs(early[random-3:random]))
assert {('mov','dword ptr [rsp + 0x20], 0'),('mov','rdx, r14')}<=set(pairs(early[random:follow]))
assert -1.571 in floats(n,early), 'Original negative native PET_FOLLOW_ANGLE'
needs(student,'?IsInWorld@Object@@','?IsPreDelete@Object@@','?IsEmpty@ObjectGuid@@',
      '?InSamePhase@WorldObject@@','?ShouldHideFor@WorldObject@@','?IsWithinDistInMap@WorldObject@@')
assert {98074,362259,2}<=values(student) and -362263 in values(student) and 70.0 in floats(n,student)
assert {('cmp','qword ptr [rbx + 8], rcx'),('cmp','dword ptr [rbx + 0xa18], 0'),
        ('mov','edx, 0x18'),('mov','edx, 0x20'),('mov','rdx, rsi'),
        ('mov','rdx, rbp'),('xor','r9d, r9d'),('mov','rax, qword ptr [rbx + 0x3a88]')}<=set(pairs(student))
needs(valid,ctx('Progress'),ctx('Actor'),ctx('Leaf'),ctx('Retained'),'??8ObjectGuid@@','?ShouldHideFor@WorldObject@@')
# Keep all three original callbacks for other actors, with an exact scoped
# equality branch around each queue; do not substitute an early route request.
setdata=ins('?SetData@npc_monk_chen_porterAI@npc_monk_chen_porter@@')
queues=sites(n,setdata,'?AddDelayedEvent@FunctionProcessor@@');assert len(queues)==3
for q in queues:
    guard=max(i for i,o in enumerate(setdata[:q]) if o.mnemonic=='cmp' and o.op_str.endswith(' + 0x3a88], 0x586d9'))
    assert {1014,103148,362201}<=values(setdata[max(0,guard-14):guard+1])
    branch=setdata[guard+1];assert branch.mnemonic in ('je','jne')
    equal=branch.operands[0].imm if branch.mnemonic=='je' else branch.address+branch.size
    assert setdata[q].address not in reachable(setdata,equal)
for body in (early,student,valid):
    for forbidden in ('?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@','?AddDelayedEvent@Unit@@','?AddDelayedEvent@FunctionProcessor@@'):
        assert not sites(n,body,forbidden),('No new progression or AI-owned callback',forbidden)
print(f"PASS registered{0x498 if n.get('monk_initial_successor') else 0x490 if p['nine_successor'] else 0x430:x} instance/418 voice-bound4.443/5 actions before8s route: padding flags, genuine leaf/weak attempt/full Chen GUID/epoch, native clock/once/reentry, four original98074 spawn guards/empty owner/mutual phase/private/70yd2D, full GUID snapshot/dedupe/reread after native random follow/default0; all three unscoped queues retained. Client/full943/all36 remain separate.")
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'instance_size':0x498 if n.get('monk_initial_successor') else 0x490 if p['nine_successor'] else 0x430,'state_offset':0x418,'deadlines_ms':[4443,5000,8000]}))

"""Actual Hait-owned AI work and GUID-bound vehicle cleanup in the linked PE."""
from pathlib import Path
import hashlib
import json
import runpy
import struct
from capstone import CS_OP_MEM
from sharasdal_erunak_entry_pe import sites, values
from pe_creature_ai_slots import callback_slot
from monk_intro_clock_pe import verify_clock

p=runpy.run_path(str(Path(__file__).with_name('verify_monk_chen_timers_release.py')))
n,refs,table,resolved=p['n'],p['refs'],p['table'],p['resolved']
ins=n['instructions'];suffix='@npc_master_haitAI@npc_master_hait@@'
def method(name): return '?'+name+suffix
actor_table,raw=table('??_7npc_master_haitAI@npc_master_hait@@6B@',82)
script_table,script_raw=table('??_7npc_master_hait@@6BScriptObject@@@',16)
factory=ins('?GetAI@npc_master_hait@@')
assert actor_table in refs(factory) and sites(n,factory,'??0FunctionProcessor@@')
assert n['symbol']('?GetAI@npc_master_hait@@') in resolved(script_raw)
assert script_table in refs(ins('?AddSC_monk_intro_scenario@@'))
slots=resolved(raw)
assert slots[0]==n['symbol']('??_Gnpc_master_haitAI@npc_master_hait@@')
assert slots[0x110//8]==n['symbol'](method('UpdateLifecycle'))
assert all(n['symbol'](method(name)) in slots for name in ('MovementInform','SetData','DoAction','OnSpellClick','UpdateAI'))
life=verify_clock(n,method('UpdateLifecycle'),0x38);destroy=ins('??_Gnpc_master_haitAI@npc_master_hait@@')
destruct,=sites(n,destroy,'??1FunctionProcessor@@')
assert (destroy[destruct-1].mnemonic,destroy[destruct-1].op_str) in (('add','rcx, 0x38'),('lea','rcx, [rbx + 0x38]'))
assert not sites(n,ins(method('UpdateAI')),'?Update@FunctionProcessor@@')

def queue(body,expected):
    queued=sites(n,body,'?AddDelayedEvent@FunctionProcessor@@')
    assert len(queued)==len(expected)
    for at,(owner,delay) in zip(queued,expected):
        previous=[(op.mnemonic,op.op_str) for op in body[at-6:at]]
        target=owner if isinstance(owner,tuple) else ('lea',owner)
        assert target in previous and ('mov',f'edx, {delay:#x}') in previous,(owner,delay,previous)

data=ins(method('SetData'));move=ins(method('MovementInform'));action=ins(method('DoAction'))
defeat_successor=any(name.startswith('?UpdateDefeat@State@MonkHaitOpening@@') for name in n['symbols'])
if defeat_successor:
    # The scoped guard keeps the AI in RDI, and the one native Chuan lookup
    # returns into RBX. Prove those definitions before checking old queues.
    assert ('mov','rdi, rcx') in [(o.mnemonic,o.op_str) for o in data]
    found,=sites(n,data,'?FindNearestCreature@WorldObject@@')
    assert (data[found+1].mnemonic,data[found+1].op_str)==('mov','rbx, rax')
    queue(data,[('rcx, [rbx + 0x36a0]',2000),('rcx, [rbx + 0x36a0]',4000),('rcx, [rbx + 0x36a0]',7000),('rcx, [rdi + 0x38]',10000)])
else:
    queue(data,[('rcx, [rdi + 0x36a0]',2000),('rcx, [rdi + 0x36a0]',4000),('rcx, [rdi + 0x36a0]',7000),('rcx, [rbx + 0x38]',10000)])
jorvinax_successor=94350 in values(move)
move_owner='rdi' if jorvinax_successor else 'rbx'
queue(move,[(f'rcx, [{move_owner} + 0x38]',3000),(f'rcx, [{move_owner} + 0x38]',6000)])
opening_successor=any(name.startswith('?Accept@State@MonkHaitOpening@@') for name in n['symbols'])
if opening_successor:
    assert ('mov','rdi, rcx') in [(op.mnemonic,op.op_str) for op in action]
    assert {97778,1014,362105}<=values(action)
    queue(action,[('rcx, [rdi + 0x38]',delay) for delay in (1000,4000,7000,10000)])
else:
    queue(action,[(('add','rcx, 0x38'),1000)]+[('rcx, [rbx + 0x38]',delay) for delay in (4000,7000,10000)])
outer=ins('??R<lambda_4>@?4??DoAction'+suffix)
queue(outer,[('rcx, [rdi + 0x36a0]',20000),('rcx, [rdi + 0x36a0]',23000),('rcx, [rbx + 0x36a0]',8000),('rcx, [rbx + 0x36a0]',15000)])
finds=sites(n,outer,'?FindNearestCreature@WorldObject@@');assert len(finds)==2 and {98001,98011}<=values(outer)
assert (outer[finds[0]+1].mnemonic,outer[finds[0]+1].op_str)==('mov','rbx, rax')
assert (outer[finds[1]+1].mnemonic,outer[finds[1]+1].op_str)==('mov','rdi, rax'),'Both cleanup timers belong to the actual Demon'
getguid=sites(n,outer,'?GetGuidValue@Object@@');assert len(getguid)==2
assert [(outer[at+1].mnemonic,outer[at+1].op_str) for at in getguid]==[('movups','xmm7, xmmword ptr [rax]'),('movups','xmm6, xmmword ptr [rax]')]

helper=ins(method('AttackDemon'))
queue(helper,[('rcx, [rbx + 0x36a0]',500)])
assert sites(n,helper,'?GetCreature@ObjectAccessor@@') and sites(n,helper,'?ShouldLog@Log@@') and sites(n,helper,'?outMessage@Log@@')
guards=[i for i,op in enumerate(helper) if op.mnemonic=='cmp' and op.op_str in ('dword ptr [rbx + 0xa18], 0','dword ptr [rax + 0xa18], 0')]
assert len(guards)==2 and all(helper[i+1].mnemonic=='jne' for i in guards)
assert len({helper[i+1].operands[0].imm for i in guards})==1,'Both dead actors stop the actual helper'
ai_slot=callback_slot(n,'?AttackStart@SmartAI@@')
virtual=[i for i,op in enumerate(helper) if op.mnemonic=='call' and op.operands[0].type==CS_OP_MEM and op.operands[0].mem.disp==ai_slot*8]
assert len(virtual)==1
prefix=[(op.mnemonic,op.op_str) for op in helper[:virtual[0]]]
assert ('mov','rcx, qword ptr [rbx + 0x9c0]') in prefix and ('test','rcx, rcx') in prefix
assert ('test','dil, dil') in [(op.mnemonic,op.op_str) for op in helper]
assert b'Monk intro: demon %s has no AI for its delayed attack after 10 retries' in n['binary']

callbacks=[key for key in n['symbols'] if key.startswith('?_Do_call@') and 'npc_master_haitAI@' in key]
assert len(callbacks)==15
parents=[data,move,action,outer,helper]
for callback in callbacks:
    assert any(n['symbol'](callback) in resolved(struct.unpack_from('<8Q',n['binary'],n['offset'](address))) for parent in parents for address in refs(parent)),('Selected callback not actually installed',callback)
bodies={key:ins(key) for key in callbacks}
flight,=[body for body in bodies.values() if {227140,193268}<=values(body)]
cleanup,=[body for body in bodies.values() if sites(n,body,'?RemoveAllPassengers@Vehicle@@')]
retry,=[body for body in bodies.values() if sites(n,body,method('AttackDemon')) and not sites(n,body,'?RemoveAllPassengers@Vehicle@@')]
assert len(sites(n,flight,'?GetCreature@ObjectAccessor@@'))==1 and len(sites(n,flight,'?CastSpell@Unit@@'))==2
assert ('movups','xmm0, xmmword ptr [rcx + 0x10]') in [(op.mnemonic,op.op_str) for op in flight]
assert len(sites(n,cleanup,'?GetCreature@ObjectAccessor@@'))==2 and 185627 in values(cleanup)
passengers,=sites(n,cleanup,'?RemoveAllPassengers@Vehicle@@');removeflags,=sites(n,cleanup,'?RemoveFlag@Object@@');attack,=sites(n,cleanup,method('AttackDemon'))
assert passengers<min(sites(n,cleanup,'?GetCreature@ObjectAccessor@@'))<removeflags<attack
assert 0x380 in values(cleanup) and 10 in values(cleanup)
assert ('movups','xmm0, xmmword ptr [rbx + 0x10]') in [(op.mnemonic,op.op_str) for op in cleanup]
assert ('movups','xmm0, xmmword ptr [rbx + 0x20]') in [(op.mnemonic,op.op_str) for op in cleanup]
for at in sites(n,cleanup,'?GetCreature@ObjectAccessor@@'):
    branch=next(op for op in cleanup[at+1:at+6] if op.mnemonic=='je')
    assert branch.operands[0].imm<=cleanup[removeflags-3].address,'Absent optional actors cannot skip vehicle flag cleanup'
assert len(sites(n,retry,method('AttackDemon')))==1 and ('dec','r8b') in [(op.mnemonic,op.op_str) for op in retry]
assert ('movups','xmm0, xmmword ptr [rcx + 0x10]') in [(op.mnemonic,op.op_str) for op in retry]
texts=[body for body in bodies.values() if sites(n,body,'?SendChat@CreatureTextMgr@@') and not sites(n,body,'?GetCreature@ObjectAccessor@@')]
assert len(texts)==5
for body in texts:
    assert not sites(n,body,'?Talk@CreatureAI@@') and not any(arg.type==CS_OP_MEM and arg.mem.disp==0x9c0 for op in body for arg in op.operands)
for body in (helper,flight,cleanup,retry):
    for forbidden in ('?CompleteQuest@Player@@','?AddItem@Player@@','?KilledMonsterCredit@Player@@','?UpdateAchievementCriteria@Player@@','?TeleportTo@Player@@'):
        assert not sites(n,body,forbidden)
print('PASS actual registered Hait factory/lifecycle110/destructor38, seven AI timers/five Unit-self native texts/all15 callback vtables; full GUIDs and actual Demon-owned20/23 passenger+flag cleanup independent of missing other actors, native live/current-AI AttackStart and finite500ms/10 retry diagnostic. Native parent owners/delays and retained Nine/Chen/retry PE verified; client/full943/admission remain separate.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'selected_callbacks':15,'native_attack_slot':ai_slot}))

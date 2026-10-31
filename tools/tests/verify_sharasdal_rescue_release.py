"""Follow the native Breaking Bonds loader, effect and AI-owned presentation."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM,CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

grash=runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_grash_healing_release.py')))
n=grash['n'];instructions=n['instructions']
table=grash['old']['previous']['selected_tables']['npc_adel_waterspeaker']
actor='@npc_adel_waterspeakerAI@npc_adel_waterspeaker@@'
spell='@spell_sharasdal_breaking_bonds@@'
def addresses(prefix):
    result={n['resolve'](address) for name,address in n['symbols'].items() if name.startswith(prefix)}
    assert result,prefix
    return result
def sites(body,prefix):
    wanted=addresses(prefix)
    return [i for i,x in enumerate(body) if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM and n['resolve'](x.operands[0].imm) in wanted]
def callees(body):
    return {n['resolve'](x.operands[0].imm) for x in body if x.mnemonic in ('call','jmp') and x.operands[0].type==CS_OP_IMM}
def values(body):return {operand.imm for x in body for operand in x.operands if operand.type==CS_OP_IMM}
def references(body):
    return [x.address+x.size+operand.mem.disp for x in body for operand in x.operands if operand.type==CS_OP_MEM and operand.mem.base==X86_REG_RIP]
def table_contains(address,targets,count=16):
    return targets<=set(struct.unpack_from('<'+str(count)+'Q',n['binary'],n['offset'](address)))

for method,slot in [('Reset',0x28),('UpdateLifecycle',0x110),('HealReceived',0xb0)]:
    assert struct.unpack_from('<Q',n['binary'],table+slot)[0]==n['symbol']('?'+method+actor)
assert struct.unpack_from('<Q',n['binary'],table+0x220)[0]==n['symbol']('?OnSpellClick@CreatureAI@@')
assert instructions('?OnSpellClick@CreatureAI@@')[0].mnemonic=='ret'
click=instructions('?HandleSpellClick@Unit@@')
assert any(x.mnemonic=='call' and x.operands[0].type==CS_OP_MEM and x.operands[0].mem.disp==0x220 for x in click)

getter='?GetSpellScript@?$GenericSpellScriptLoader@Vspell_sharasdal_breaking_bonds@@@@'
assert any(table_contains(address,{n['symbol'](getter)},2) and struct.unpack_from('<Q',n['binary'],n['offset'](address)+8)[0]==n['symbol'](getter)
           for address in references(instructions('?AddSC_Sharasdal@@')))
assert any(table_contains(address,{n['symbol']('?'+method+spell) for method in ('Register','Validate','Load')})
           for address in references(instructions(getter)))
registered=set(references(instructions('?Register'+spell)))|values(instructions('?Register'+spell))
for method in ('Check','BeforeEvent','AfterEvent'):assert n['symbol']('?'+method+spell) in registered
assert 61 in values(instructions('?Register'+spell))
assert {208171,61,49816}<=values(instructions('?Validate'+spell))

load=instructions('?Load'+spell);ready=instructions('?Ready'+spell);receiver=instructions('?Receiver'+spell)
assert sites(load,'?GetOriginalTarget@SpellScript@@') and sites(load,'?AddQuestDelayedEvent@Player@@') and 40341 in values(load)
assert sites(ready,'?GetCreature@ObjectAccessor@@') and sites(ready,'?getQuestStatus@Player@@')
assert sites(ready,'?Receiver'+spell) and sites(ready,'?Eligible@SharasdalRescue@@')
assert {47123,47197,0x1000000}<=values(ready)
assert {0x9c0,0x9c8}<={operand.mem.disp for x in receiver for operand in x.operands if operand.type==CS_OP_MEM}
before=instructions('?BeforeEvent'+spell);after=instructions('?AfterEvent'+spell)
assert sites(before,'?Ready'+spell) and sites(before,'?PreventHitDefaultEffect@SpellScript@@')
assert sites(after,'?Ready'+spell) and sites(after,'?AcceptRescue'+actor) and 7000 in values(after)
assert sites(after,'?getMSTime@@')
assert {104853,1600,1066}<=values(instructions('?World@SharasdalRescue@@'))

accept=instructions('?AcceptRescue'+actor);life=instructions('?UpdateLifecycle'+actor);reset=instructions('?Reset'+actor)
assert 47123 in values(accept) and sites(accept,'?Receipt@SharasdalGrash@@') and sites(accept,'?RemoveFlag@Object@@')
recover,=sites(life,'?Recover@SharasdalCompanion@@');clock,=sites(life,'?Update@FunctionProcessor@@')
queued=sites(life,'?AddFunction@FunctionProcessor@@')
assert len(queued)==2 and recover<clock<min(queued) and {2000,1000,47123,47197}<=values(life)
assert life[clock-1].op_str=='rcx, [rdi + 0xf0]' and life[clock+1].op_str=='rbx, qword ptr [rdi + 0xe0]'
assert sites(reset,'?KillAllFunctions@FunctionProcessor@@')
thunks=[name for name in n['symbols'] if name.startswith('?_Do_call@') and '??UpdateLifecycle@npc_adel_waterspeakerAI@npc_adel_waterspeaker@@' in name]
assert len(thunks)==2
for name in thunks:
    assert any(table_contains(address,{n['symbol'](name)},8) for address in references(life)),name
pose=next(instructions(name) for name in thunks if '<lambda_1>' in name)
notify_thunk=next(instructions(name) for name in thunks if '<lambda_2>' in name)
assert sites(notify_thunk,'?NotifyGrash'+actor)
assert sites(pose,'?Receipt@SharasdalGrash@@') and sites(pose,'?SetByteValue@Object@@') and sites(pose,'?SetAnimKitId@Unit@@')
assert sites(pose,'?HandleEmoteCommand@Unit@@') and {47123,53}<=values(pose)
claimed=next(i for i,x in enumerate(pose) if x.mnemonic=='mov' and x.op_str=='byte ptr [rbx + 0x172], 1')
assert claimed<sites(pose,'?SetByteValue@Object@@')[0]
notify=instructions('?NotifyGrash'+actor)
assert {47123,47197,105298,19483,30}<=values(notify)
assert sites(notify,'?SetFlag@Object@@') and sites(notify,'?Talk@CreatureAI@@QEAAXEVObjectGuid@@')
assert b'Sharasdal rescue cannot resolve lower Grash after thirty attempts; retaining reset/re-entry recovery' in n['binary']

for body in [load,ready,before,after,accept,life,reset,pose,notify]:
    for prefix in ('?UpdateAchievementCriteria@Player@@','?AddItem@Player@@','?CompleteQuest@Player@@','?AddDelayedEvent@Unit@@'):
        forbidden={n['resolve'](address) for name,address in n['symbols'].items() if name.startswith(prefix)}
        assert not forbidden&callees(body),prefix
native_event=instructions('?EffectSendEvent@Spell@@')
assert len(sites(native_event,'?UpdateAchievementCriteria@Player@@'))==2
assert {73,92}<=values(native_event)
print('PASS actual registered208171 SpellScript loader/vtable/three hooks/61+49816 validation, original-target GUID and40341 quest/AI leases, current+suspended receiver, native guard/rescue receipts and7s minimum. Selected raw click callback is empty; AI-owned delay/callback tables, reset token/clock/reentry guard, once pose53 and bounded lower-Grash role update are present. Native EffectSendEvent alone owns73/92 progress; no manual49816/49957/item/quest grant or Unit-owned raw callback in new paths. SQL binding/native tests and client traversal remain separate')

"""Verify selected final Vigfus communion/combat gates and scaled damage rejection."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_thresholds_release.py')))
native = previous['native']
instructions = previous['instructions']
suffix = '@npc_vigfus_bladewind_lastAI@npc_vigfus_bladewind_last@@'
ready = '?IsEncounterReady' + suffix
selection = '?CanAIAttack' + suffix
attack = '?AttackStart' + suffix
post = '?DamageTakenAfterScaling' + suffix
update = '?UpdateAI' + suffix
body = instructions(ready)
values = {operand.imm for entry in body for operand in entry.operands if operand.type == CS_OP_IMM}
assert {98602, 1511, 944, 5} <= values, 'Encounter gate lost exact actor/map/scenario/stage scope'
for dependency in ('?GetScenario@ScenarioMgr@@', '?GetScenarioId@Scenario@@', '?GetCurrentStep@Scenario@@'):
    assert native['symbol'](dependency) in native['calls'](ready), f'Missing live encounter dependency: {dependency}'
for value in (98602, 1511):
    comparison = next(index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.operands[-1].type == CS_OP_IMM and entry.operands[-1].imm == value)
    branch = body[comparison + 1]
    assert branch.mnemonic == 'jne', 'Unrelated actor/map must retain ordinary admission'
    target = next(entry for entry in body if entry.address == branch.operands[0].imm)
    assert target.mnemonic == 'mov' and target.op_str == 'al, 1', 'Out-of-scope reuse is blocked'
scenario_call = next(index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol']('?GetScenario@ScenarioMgr@@'))
for displacement, rejected in ((0xc6, 'je'), (0xc7, 'jne')):
    check = next(index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.op_str == f'byte ptr [rbx + {displacement:#x}], 0')
    branch = body[check + 1]
    assert check < scenario_call and branch.mnemonic == rejected, 'Missing introDone/pending combat admission'
    target = next(entry for entry in body if entry.address == branch.operands[0].imm)
    assert target.mnemonic == 'xor' and target.op_str == 'al, al', 'Pre-communion combat is still admitted'
assert native['symbol'](ready) == native['symbol'](selection) or native['symbol'](ready) in native['calls'](selection), 'Selection does not execute the actual live stage gate'

update_body = instructions(update)
def update_call(prefix):
    return next(index for index, entry in enumerate(update_body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol'](prefix))
intro_tick = update_call('?UpdateIntro' + suffix)
spectre_tick = update_call('?UpdateSpectreRush' + suffix)
readiness = update_call(ready)
victim = update_call('?UpdateVictim@CreatureAI@@')
assert intro_tick < spectre_tick < readiness < victim, 'Intro/retry is not retained before combat admission and victim selection'
assert update_body[readiness + 1].op_str == 'al, al', 'UpdateAI ignores readiness'
branch = update_body[readiness + 2]
assert branch.mnemonic == 'je' and branch.operands[0].imm > update_body[victim].address, 'Pre-communion UpdateAI still enters victim selection/evade'
target = next(index for index, entry in enumerate(update_body) if entry.address == branch.operands[0].imm)
assert all(entry.mnemonic in ('mov', 'add', 'pop', 'ret') for entry in update_body[target:]), 'Rejected UpdateAI still executes encounter timers or melee'

attack_body = instructions(attack)
ready_call = next(index for index, entry in enumerate(attack_body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol'](ready))
base_call = next(index for index, entry in enumerate(attack_body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol']('?AttackStart@ScriptedAI@@'))
assert ready_call < base_call and attack_body[ready_call + 1].op_str == 'al, al', 'Scripted engagement does not test the gate result'
branch = attack_body[ready_call + 2]
assert branch.mnemonic == 'je' and branch.operands[0].imm > attack_body[base_call].address, 'Rejected scripted engagement still reaches native AttackStart'

post_body = instructions(post)
gate_call = next(index for index, entry in enumerate(post_body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == native['symbol'](ready))
damage_register = next(entry.op_str.split(', ')[0] for entry in post_body[:gate_call] if entry.mnemonic == 'mov' and entry.op_str.endswith(', r8'))
assert post_body[gate_call + 1].op_str == 'al, al' and post_body[gate_call + 2].mnemonic == 'jne', 'Wrong-stage damage does not take the rejection branch'
zero, store, leave = post_body[gate_call + 3:gate_call + 6]
assert zero.mnemonic == 'xor' and zero.op_str == 'edi, edi', 'Wrong-stage damage is not zeroed'
assert store.mnemonic == 'mov' and store.op_str == f'dword ptr [{damage_register}], edi', 'Final scaled damage reference is not rejected'
assert leave.mnemonic == 'jmp' and leave.operands[0].imm > post_body[gate_call + 2].operands[0].imm, 'Rejected damage continues presentation/combat'

tables = []
for entry in instructions('?GetAI@npc_vigfus_bladewind_last@@'):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            offset = native['offset'](entry.address + entry.size + operand.mem.disp)
            slots = struct.unpack_from('<128Q', native['binary'], offset)
            if slots[0xa8 // 8] == native['symbol'](post) and slots[0x10 // 8] == native['symbol'](attack):
                assert native['resolve'](slots[1]) == native['symbol'](selection), 'Selected CanAIAttack thunk does not reach the live gate'
                assert native['symbol'](update) in slots, 'Factory does not select the protected encounter tick'
                tables.append(slots)
assert len(tables) == 1, 'Final factory does not select all three combat guards'
slot = 8
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == slot for entry in instructions('?canCreatureAttack@Creature@@')), 'Native creature selection misses selected CanAIAttack slot'
slot = tables[0].index(native['symbol'](attack)) * 8
victim_body = instructions('?UpdateVictim@CreatureAI@@')
assert any(entry.mnemonic == 'mov' and entry.op_str == f'r8, qword ptr [rcx + {slot:#x}]' and victim_body[index + 2].mnemonic == 'call' and victim_body[index + 2].op_str == 'r8' for index, entry in enumerate(victim_body[:-2])), 'Native UpdateVictim misses selected AttackStart slot'
assert native['symbol']('?Kill@Unit@@') in native['calls']('?DealDamage@Unit@@'), 'Native damage/death consumer missing'
print('PASS: canonical final98602 factory selects live1511/944/5 plus completed nonpending communion for native selection, AttackStart, post-scaled damage and UpdateAI before victim dispatch/timers/melee; intro and spectre retry still execute first, unrelated reuse retained. Earlier cast/proc/threat effects, direct Kill, cinematic timing and client traversal remain separate.')

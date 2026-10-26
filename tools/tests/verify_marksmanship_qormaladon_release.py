"""Verify the selected Qormaladon AI, inherited death and actual condition callback."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, instructions = previous['native'], previous['instructions']
gate = '?Update@Gate@MarksmanshipInquisitor@@'
condition = '?OnConditionCheck@condition_marksmanship_qormaladon_ready@@'


def values(prefix):
    return {operand.imm for instruction in instructions(prefix)
            for operand in instruction.operands if operand.type == CS_OP_IMM}


for prefix in (gate, condition):
    assert {101269, 1489, 972, 5, 256} <= values(prefix), 'Actual stage, actor or immunity check missing'
    for dependency in ('?GetScenario@ScenarioMgr@@', '?GetScenarioId@Scenario@@', '?GetCurrentStep@Scenario@@', '?HasFlag@Object@@'):
        assert native['symbol'](dependency) in native['calls'](prefix), f'Missing {prefix} -> {dependency}'
assert {2} <= values(gate)
for dependency in ('?CombatStop@Unit@@', '?SetReactState@Creature@@', '?RemoveFlag@Object@@', '?SetFlag@Object@@'):
    assert native['symbol'](dependency) in native['calls'](gate), f'Missing gate cleanup: {dependency}'
for method in ('InitializeAI', 'Reset', 'UpdateAI', 'SetData', 'AttackStart', 'DamageTaken'):
    wrapper = f'?{method}@npc_marksmanship_qormaladon@@'
    targets = native['calls'](wrapper)
    assert native['symbol'](gate) in targets, f'{method} misses stage admission'
    assert native['symbol'](f'?{method}@SmartAI@@') in targets, f'{method} loses native SmartAI behavior'
assert any(instruction.mnemonic == 'mov' and len(instruction.operands) == 2 and
           instruction.operands[0].type == CS_OP_MEM and instruction.operands[0].size == 4 and
           instruction.operands[1].type == CS_OP_IMM and instruction.operands[1].imm == 0
           for instruction in instructions('?DamageTaken@npc_marksmanship_qormaladon@@')), 'Blocked damage not zeroed'
set_data_calls = [native['resolve'](operand.imm)
                  for instruction in instructions('?SetData@npc_marksmanship_qormaladon@@')
                  if instruction.mnemonic in {'call', 'jmp'}
                  for operand in instruction.operands if operand.type == CS_OP_IMM]
assert set_data_calls.index(native['symbol']('?SetData@SmartAI@@')) < set_data_calls.index(native['symbol'](gate)), 'Unlock must process native flag removal before releasing gate'
factory = '?GetAI@?$GenericCreatureScript@Unpc_marksmanship_qormaladon@@@@'
tables = []
for instruction in instructions(factory):
    for operand in instruction.operands:
        if instruction.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            at = native['offset'](instruction.address + instruction.size + operand.mem.disp)
            slots = struct.unpack_from('<80Q', native['binary'], at)
            if native['symbol']('?DamageTaken@npc_marksmanship_qormaladon@@') in slots:
                tables.append(slots)
assert len(tables) == 1, 'Generic creature factory does not select repaired AI'
slots = tables[0]
for method in ('InitializeAI', 'Reset', 'UpdateAI', 'SetData', 'AttackStart', 'DamageTaken'):
    assert native['symbol'](f'?{method}@npc_marksmanship_qormaladon@@') in slots, f'Unselected wrapper {method}'
assert slots[0x130 // 8] == native['symbol']('?JustDied@SmartAI@@'), 'Native death/escort/boss cleanup replaced'
assert native['symbol']('?OnConditionCheck@ScriptMgr@@') in native['calls']('?Meets@Condition@@'), 'Native condition dispatcher misses custom readiness guard'
assert b'npc_marksmanship_qormaladon' in native['binary'] and b'condition_marksmanship_qormaladon_ready' in native['binary']
assert native['symbol']('?AddSC_artifact_acquisition@@') in native['calls']('?AddLegionScripts@@'), 'Artifact registration not reached'
print(f'PASS {native["args"].release}: selected Qormaladon factory, stage/immunity guard, six SmartAI delegates, immediate native unlock, inherited death cleanup and condition dispatcher. Not a client combat/visual traversal.')

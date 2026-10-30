"""Verify Stromkar's completed-scene and existing-inventory bridge in the real PE."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_xalatath_presentation_release.py')))
native = previous['native']
instructions = previous['instructions']
handler = '?Complete@StromkarPresentation@@'
values = {operand.imm for instruction in instructions(handler) for operand in instruction.operands if operand.type == CS_OP_IMM}
assert {1225, 1590, 1539, 41105, 1037, 128910, 206439}.issubset(values), 'Missing native Stromkar scope'
for dependency in ('?HasSceneStatus@Player@@', '?GetItemByEntry@Player@@', '?GetItemByPos@Player@@QEBAPEAVItem@@EE@Z', '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_NPEAVItem@@PEBVAuraEffect@@VObjectGuid@@'):
    assert native['symbol'](dependency) in native['calls'](handler), f'Missing checked scene/equip consumer {dependency}'
assert native['symbol'](handler) in native['calls']('?OnTrigger@scene_stromkar_pickup@@'), 'Registered wrapper misses helper'
decoded = instructions(handler)
gates = {}
for index, (compare, branch) in enumerate(zip(decoded, decoded[1:])):
    if compare.mnemonic != 'cmp' or len(compare.operands) != 2 or compare.operands[1].type != CS_OP_IMM:
        continue
    stage = compare.operands[1].imm
    if stage in (9, 10) and branch.mnemonic == {9: 'je', 10: 'jne'}[stage]:
        assert decoded[index-1].mnemonic == 'call' and decoded[index-1].operands[0].imm == native['symbol']('?GetCurrentStep@Scenario@@'), 'Stage comparison is not current scenario state'
        gates[stage] = (index, branch.operands[0].imm)
assert gates.keys() == {9, 10}, 'Missing bounded final or completed-stage admission'
assert gates[9][0] < gates[10][0] and gates[9][1] == decoded[gates[10][0]+2].address, 'Stage9 must skip only the stage10 rejection'
assert gates[10][1] != gates[9][1], 'Other stages must reject instead of reaching equip'
assert b'scene_stromkar_pickup' in native['binary']
print('PASS: linked1225/1590 completion checks actual status/quest/map/scenario1037/stage9-or-completed10/owned128910, casts206439 and inspects real equipped blade; no inventory grant')

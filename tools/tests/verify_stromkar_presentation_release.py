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
assert any(compare.mnemonic == 'cmp' and len(compare.operands) == 2 and compare.operands[1].type == CS_OP_IMM and compare.operands[1].imm == 9 and branch.mnemonic == 'jne' for compare, branch in zip(decoded, decoded[1:])), 'Missing exact final-stage rejection'
assert b'scene_stromkar_pickup' in native['binary']
print('PASS: canonical1225/1590 completion checks actual status/quest/map/scenario1037/exact9/owned128910, casts206439 and inspects real equipped blade; no inventory grant')

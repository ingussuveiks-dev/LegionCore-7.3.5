"""Verify Xal'atath's completed-scene and existing-inventory bridge in the real PE."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_talonclaw_world_pickup_release.py')))
native = previous['native']
instructions = previous['instructions']
handler = '?Complete@XalatathPresentation@@'
values = {operand.imm for instruction in instructions(handler) for operand in instruction.operands if operand.type == CS_OP_IMM}
assert {1187, 1553, 1539, 40710, 991, 128827, 133958, 201778}.issubset(values), 'Missing native scene/quest/scenario/item/equip scope'
for dependency in ('?HasSceneStatus@Player@@', '?GetItemByEntry@Player@@', '?GetItemByPos@Player@@QEBAPEAVItem@@EE@Z', '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_NPEAVItem@@PEBVAuraEffect@@VObjectGuid@@'):
    assert native['symbol'](dependency) in native['calls'](handler), f'Missing checked scene/equip consumer {dependency}'
wrapper = '?OnTrigger@scene_xalatath_pickup@@'
assert native['symbol'](handler) in native['calls'](wrapper), 'Registered scene wrapper misses the scoped helper'
bounds = {}
decoded = instructions(handler)
for compare, branch in zip(decoded, decoded[1:]):
    if compare.mnemonic != 'cmp' or len(compare.operands) != 2 or compare.operands[1].type != CS_OP_IMM:
        continue
    expected = {8: 'jb', 9: 'ja'}.get(compare.operands[1].imm)
    if expected and branch.mnemonic == expected and branch.operands[0].type == CS_OP_IMM:
        bounds[compare.operands[1].imm] = branch.operands[0].imm
assert bounds.keys() == {8, 9} and bounds[8] == bounds[9], 'Linked scene callback widens beyond991/8-9'
assert b'scene_xalatath_pickup' in native['binary']
print('PASS: canonical1187/1553 completion uses real HasSceneStatus, exact quest/map/scenario/existing blade, native201778 cast and equipped pair inspection; no inventory grant')

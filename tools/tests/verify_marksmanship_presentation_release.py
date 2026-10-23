"""Inspect the canonical native Thas'dorah scene-to-existing-weapon bridge."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_talonclaw_deferred_scene_release.py')))
instructions = previous['previous']['instructions']
native = previous['previous']['native']


def values(prefix):
    return {operand.imm for instruction in instructions(prefix)
            for operand in instruction.operands if operand.type == CS_OP_IMM}


apply = '?AfterApply@spell_marksmanship_presentation@@'
remove = '?AfterRemove@spell_marksmanship_presentation@@'
finish = '?Finish@State@MarksmanshipPresentation@@'
update = '?Update@State@MarksmanshipPresentation@@'
current = '?Current@State@MarksmanshipPresentation@@'
assert {972, 7, 8}.issubset(values(apply)), 'Missing final/completed Marksmanship scenario admission'
assert {1489, 40419, 128826}.issubset(values(current)), 'Wrong quest, map or existing weapon'
assert native['symbol'](finish) in native['calls'](remove), 'Aura removal loses its checked native finish'
assert {1169, 200262}.issubset(values(finish)), 'Missing native scene/equip bridge'
assert 20000 in values(update), 'Missing bounded scene cleanup'
assert native['symbol']('?HasSceneStatus@Player@@') in native['calls'](finish)
assert native['symbol']('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_NPEAVItem@@PEBVAuraEffect@@VObjectGuid@@') in native['calls'](finish)
assert native['symbol']('?Remove@AuraScript@@') in native['calls']('?Update@spell_marksmanship_presentation@@')
bounds = {}
decoded = instructions(apply)
for compare, branch in zip(decoded, decoded[1:]):
    if compare.mnemonic != 'cmp' or len(compare.operands) != 2 or compare.operands[1].type != CS_OP_IMM:
        continue
    expected = {7: 'jb', 8: 'ja'}.get(compare.operands[1].imm)
    if expected and branch.mnemonic == expected and branch.operands[0].type == CS_OP_IMM:
        bounds[compare.operands[1].imm] = branch.operands[0].imm
assert bounds.keys() == {7, 8} and bounds[7] == bounds[8], 'Linked aura admission widens beyond steps7/8'
assert b'spell_marksmanship_presentation' in native['binary']
print('PASS: canonical Release contains scoped Thas\'dorah1169 -> native200262 equip, exact7/8 admission and bounded aura cleanup')

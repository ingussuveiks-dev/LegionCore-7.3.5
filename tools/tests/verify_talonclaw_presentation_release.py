"""Inspect the linked Survival scene hooks, not just their registration string."""
from pathlib import Path
import bisect
import runpy
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM

native = runpy.run_path(str(Path(__file__).with_name('verify_scene_lifecycle_release.py')))['native']
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True


def immediates(prefix):
    address = native['symbol'](prefix)
    index = bisect.bisect_right(native['starts'], address - native['base']) - 1
    begin, end, unwind = native['functions'][index]
    assert begin <= address - native['base'] < end, f'Missing unwind bounds: {prefix}'
    origin = native['root'](begin, unwind)
    for first, last, data in native['functions'][index + 1:]:
        if first != end or native['root'](first, data) != origin:
            break
        end = last
    start = native['offset'](address)
    code = native['binary'][start:start + native['base'] + end - address]
    return {operand.imm for instruction in decoder.disasm(code, address)
            for operand in instruction.operands if operand.type == CS_OP_IMM}


apply = '?AfterApply@spell_talonclaw_presentation@@'
remove = '?AfterRemove@spell_talonclaw_presentation@@'
update = '?Update@State@TalonclawPresentation@@'
current = '?Current@State@TalonclawPresentation@@'
assert {973, 12}.issubset(immediates(apply)), 'Wrong scenario/final-stage admission'
assert {1511, 40385, 128808}.issubset(immediates(current)), 'Wrong map, quest or existing weapon admission'
assert {1220, 197651}.issubset(immediates(remove)), 'Wrong completed scene or native equip spell'
assert 20000 in immediates(update), 'Missing bounded presentation fallback'
assert native['symbol']('?HasSceneStatus@Player@@') in native['calls'](remove)
assert native['symbol']('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_NPEAVItem@@PEBVAuraEffect@@VObjectGuid@@') in native['calls'](remove)
assert native['symbol']('?Remove@AuraScript@@') in native['calls']('?Update@spell_talonclaw_presentation@@')
assert b'spell_talonclaw_presentation' in native['binary']
print('PASS: actual Release Talonclaw hooks retain final-stage owned-item admission, native equip197651, scene1220 and bounded aura cleanup')

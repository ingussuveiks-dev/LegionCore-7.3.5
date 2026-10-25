"""Verify Earthwarder scene completion reaches the existing-item equip handler."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_earthwarder_real_loot_release.py')))
native = previous['native']
instructions = previous['instructions']
handler = '?Complete@EarthwarderPresentation@@'
values = {operand.imm for entry in instructions(handler) for operand in entry.operands if operand.type == CS_OP_IMM}
assert {1215, 1575, 1495, 39191, 909, 128289, 128288, 208562}.issubset(values), 'Missing native Earthwarder scene/equip scope'
for dependency in ('?HasSceneStatus@Player@@', '?GetItemByEntry@Player@@', '?GetItemByPos@Player@@QEBAPEAVItem@@EE@Z',
                   '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_NPEAVItem@@PEBVAuraEffect@@VObjectGuid@@'):
    assert native['symbol'](dependency) in native['calls'](handler), f'Missing checked scene/equip consumer: {dependency}'
assert native['symbol']('?AddItem@Player@@') not in native['calls'](handler), 'Scene creates an item instead of equipping it'
assert native['symbol'](handler) in native['calls']('?OnTrigger@scene_earthwarder_pickup@@'), 'Scene wrapper bypasses the helper'
assert native['symbol']('??0SceneTriggerScript@@') in native['calls']('?AddSC_EarthWarder@@'), 'Wrong script registry'
tables = []
for entry in instructions('?AddSC_EarthWarder@@'):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            address = entry.address + entry.size + operand.mem.disp
            offset = native['offset'](address)
            if struct.unpack_from('<Q', native['binary'], offset + 8)[0] == native['symbol']('?OnTrigger@scene_earthwarder_pickup@@'):
                tables.append(address)
assert len(tables) == 1, 'Inlined scene constructor does not install the repaired callback'
bounds = {}
decoded = instructions(handler)
for compare, branch in zip(decoded, decoded[1:]):
    if compare.mnemonic != 'cmp' or len(compare.operands) != 2 or compare.operands[1].type != CS_OP_IMM:
        continue
    expected = {5: 'jb', 6: 'ja'}.get(compare.operands[1].imm)
    if expected and branch.mnemonic == expected and branch.operands[0].type == CS_OP_IMM:
        bounds[compare.operands[1].imm] = branch.operands[0].imm
assert bounds.keys() == {5, 6} and bounds[5] == bounds[6], 'Scene callback widens beyond909/5-6'
positions = [index for index, item in enumerate(decoded) if item.mnemonic == 'call' and item.operands[0].type == CS_OP_IMM
             and native['resolve'](item.operands[0].imm) == native['symbol']('?GetItemByPos@Player@@QEBAPEAVItem@@EE@Z')]
assert len(positions) == 2
for position, slot in zip(positions, (16, 15)):
    assert any(item.op_str == f'r8b, {hex(slot)}' for item in decoded[position - 4:position]), 'Shield/child equipment slots reversed'
for dependency in ('?GetItemByEntry@Player@@', '?CanEquipItem@Player@@', '?CanEquipChildItem@Player@@', '?SwapItem@Player@@', '?SendEquipError@Player@@'):
    assert native['symbol'](dependency) in native['calls']('?EffectForceEquipItem@Spell@@'), f'Native equipment path missing: {dependency}'
assert native['symbol']('?RewardPlayerAndGroupAtEvent@Player@@') in native['calls']('?EffectKillCredit@Spell@@'), 'Native effect90 credit semantics changed'
assert native['symbol']('?TrigerScene@Player@@') in native['calls']('?SceneCompleted@Player@@'), 'Native scene completion bridge missing'
assert native['symbol']('?OnSceneTrigger@ScriptMgr@@') in native['calls']('?TrigerScene@Player@@'), 'Native scene script dispatch missing'
print('PASS: canonical registered1215/1575 completion checks actual status/live quest/map/scenario909/5-6/existing128289, casts native208562 and inspects linked128288; native equip/errors and group-effect consumers retained. Not full spell/group/client execution.')

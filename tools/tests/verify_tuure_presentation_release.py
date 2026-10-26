"""Verify registered T'uure scene-to-existing-staff bridge in the actual Release PE."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

capture = runpy.run_path(str(Path(__file__).with_name('verify_devourer_capture_visual_release.py')))
native, instructions = capture['native'], capture['instructions']
handler, wrapper = '?Complete@TuurePresentation@@', '?OnTrigger@scene_tuure_pickup@@'
body = instructions(handler)
values = {op.imm for ins in body for op in ins.operands if op.type == CS_OP_IMM}
assert {1265, 1565, 1489, 41993, 1085, 6, 47656, 128825, 210734} <= values
targets = native['calls'](handler)
for prefix in ('?HasSceneStatus@Player@@', '?GetItemByEntry@Player@@',
               '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_NPEAVItem@@PEBVAuraEffect@@VObjectGuid@@'):
    assert native['symbol'](prefix) in targets, f'Missing native presentation dependency: {prefix}'
assert native['symbol'](handler) in native['calls'](wrapper)
for prefix in ('?AddItem@Player@@', '?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@'):
    addresses = {native['resolve'](value) for name, value in native['symbols'].items() if name.startswith(prefix)}
    assert not targets & addresses, f'Equip bridge adds a direct grant: {prefix}'
text = (native['args'].release / 'worldserver.map').read_text(errors='replace')
match = re.search(r'\?\?_7scene_tuure_pickup@@6B@\s+([0-9a-fA-F]{16})', text)
assert match
table = int(match[1], 16)
assert struct.unpack_from('<Q', native['binary'], native['offset'](table) + 8)[0] == native['symbol'](wrapper)
assert any(ins.mnemonic == 'lea' and any(op.type == CS_OP_MEM and op.mem.base == X86_REG_RIP and
           ins.address + ins.size + op.mem.disp == table for op in ins.operands)
           for ins in instructions('?AddSC_Tuure@@')), 'Tuure registration does not install the actual scene vtable'
assert native['symbol']('??0SceneTriggerScript@@') in native['calls']('?AddSC_Tuure@@')
print('PASS actual Tuure registration/scene vtable ->1265/1565 Complete -> real staff/native210734, active41993/map1489/1085 step6/boss leaf47656; no direct item, quest or scenario grant. Native scene/force-equip host covers dispatch; native pickup/exit is separately regressed; actual client remains pending.')

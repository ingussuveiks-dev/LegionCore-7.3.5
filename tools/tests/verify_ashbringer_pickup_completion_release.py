"""Verify the selected Ashbringer pickup omits premature completion credit."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_ashbringer_balnazzar_surrender_release.py')))
native, instructions = previous['native'], previous['instructions']
callback = '?OnLootStateChanged@go_bs_the_ashbringer@@'
body = instructions(callback)


def sites(prefix):
    target = native['symbol'](prefix)
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == target]


credit, = sites('?DoUpdateAchievementCriteria@InstanceScript@@')
assert all(any(entry.op_str == operand for entry in body[credit - 7:credit]) for operand in ('edx, 0x5c', 'r8d, 0xb432', 'r9d, r9d'))
assert body[0].op_str == 'r9, r9' and body[1].mnemonic == 'je' and body[1].operands[0].imm == body[-1].address
assert any(entry.op_str == 'byte ptr [r9 + 0x16], 4' for entry in body[:credit])
instance, = sites('?GetInstanceScript@WorldObject@@')
stage, = sites('?getScenarionStep@InstanceScript@@')
owned, = sites('?HasItemCount@Player@@')
assert instance < stage < owned < credit
assert body[instance + 2].op_str == 'rax, rax' and body[instance + 3].mnemonic == 'je'
assert body[stage + 1].op_str == 'eax, 3' and body[stage + 2].mnemonic == 'jne'
rejected = body[stage + 2].operands[0].imm
assert rejected > body[credit].address and body[instance + 3].operands[0].imm == rejected
assert body[stage + 3].op_str == 'esi, eax' and body[stage + 4].mnemonic == 'je'
assert body[stage + 4].operands[0].imm == body[owned + 3].address
assert body[owned + 1].op_str == 'al, al' and body[owned + 2].mnemonic == 'je' and body[owned + 2].operands[0].imm == rejected
assert all(any(entry.op_str == operand for entry in body[owned - 6:owned]) for operand in ('edx, 0x1d892', 'r8d, 1', 'r9d, r9d'))
nearest, = sites('?FindNearestCreature@WorldObject@@')
despawn, = sites('?DespawnOrUnsummon@Creature@@')
cast, = sites('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N')
delete, = sites('?Delete@GameObject@@')
assert credit < nearest < despawn < cast < delete
assert any(entry.op_str == 'edx, 0x1941e' for entry in body[nearest - 4:nearest])
distance = body[nearest - 4]
assert distance.mnemonic == 'movss' and distance.operands[1].mem.base == X86_REG_RIP
assert struct.unpack_from('<f', native['binary'], native['offset'](distance.address + distance.size + distance.operands[1].mem.disp))[0] == 10.0
assert body[nearest + 1].op_str == 'rax, rax' and body[nearest + 2].mnemonic == 'je' and body[nearest + 2].operands[0].imm == body[despawn + 1].address
assert all(any(entry.op_str == operand for entry in body[cast - 11:cast]) for operand in ('r8d, 0x2c272', 'r9b, 1', 'rdx, rbx', 'rcx, rbx'))
assert body[cast + 1].op_str == 'esi, 2' and body[cast + 2].mnemonic == 'jne' and body[cast + 2].operands[0].imm == rejected
assert len([entry for entry in body if entry.mnemonic == 'call']) == 8

tables = []
for entry in instructions('?AddSC_paladin_broken_shore@@'):
    if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x50)[0] == native['symbol'](callback):
            tables.append(address)
assert len(tables) == 1
dispatch = instructions('?OnGameObjectLootStateChanged@ScriptMgr@@')
loaded = next(index for index, entry in enumerate(dispatch) if entry.op_str == 'r10, qword ptr [rcx + 0x50]')
assert dispatch[loaded + 2].mnemonic == 'call' and dispatch[loaded + 2].op_str == 'r10'
assert native['symbol']('?OnGameObjectLootStateChanged@ScriptMgr@@') in native['calls']('?SetLootState@GameObject@@')
update = instructions('?UpdateCurrentStep@Scenario@@')
reward = next(index for index, entry in enumerate(update) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
              and native['resolve'](entry.operands[0].imm) == native['symbol']('?Reward@Scenario@@'))
assert any(entry.op_str == 'r11b, r9b' for entry in update[reward - 18:reward])
print('PASS selected pickup slot50/native SetLootState dispatch retains Player/instance/stage3, completed-or-owned admission, sole type92/46130, 103454/10yd cleanup, self180850 triggered and GO_ACTIVATED2 Delete; no direct152/775 or extra call. Native later Reward retains nonbonus-count comparison. Not actual retrieval storage/equip, whole Reward or client/VFX traversal.')

"""Verify Eliza's selected death callback and its native owner/progression bridge."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, instructions = previous['native'], previous['instructions']
method = '?JustDied@npc_dread_admiral_elizaAI@npc_dread_admiral_eliza@@'
body = instructions(method)


def sites(prefix):
    target = native['symbol'](prefix)
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and
            entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == target]


owner, = sites('?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')
credit, = sites('?UpdateAchievementCriteria@Player@@')
talk, = sites('?Talk@CreatureAI@@QEAAXE')
assert owner < credit < talk, 'Native player credit must precede unchanged death reveal'
assert any(entry.mnemonic == 'test' and entry.op_str == 'rdx, rdx' and body[index + 1].mnemonic == 'je'
           and body[index + 1].operands[0].imm > body[owner].address for index, entry in enumerate(body[:owner])), 'Missing null-killer guard'
assert body[owner + 1].mnemonic == 'test' and body[owner + 1].op_str == 'rax, rax'
assert body[owner + 2].mnemonic == 'je' and body[owner + 2].operands[0].imm > body[credit].address
assert body[owner + 2].operands[0].imm <= body[talk].address, 'Unresolved owner must retain death speech/reveal'
values = {operand.imm for entry in body for operand in entry.operands if operand.type == CS_OP_IMM}
assert {92, 48582, 4, 112817, 72343, 254087, 86400} <= values, 'Original progression/reveal payload changed'
tables = []
for entry in instructions('?GetAI@npc_dread_admiral_eliza@@'):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            at = native['offset'](entry.address + entry.size + operand.mem.disp)
            slots = struct.unpack_from('<80Q', native['binary'], at)
            if slots[0x130 // 8] == native['symbol'](method):
                tables.append(at)
assert len(tables) == 1, 'Eliza factory does not select the repaired death callback'
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x130
           for entry in instructions('?Kill@Unit@@')), 'Native death dispatcher misses actual callback slot'
scenario = {native['resolve'](address) for name, address in native['symbols'].items() if name.startswith('?UpdateAchievementCriteria@Scenario@@')}
assert scenario & native['calls']('?UpdateAchievementCriteria@Player@@')
assert b'npc_dread_admiral_eliza' in native['binary']
print(f'PASS {native["args"].release}: selected Eliza JustDied130, native null-safe owner48582 and unresolved-owner path preserving Talk4/112817/display72343/254087. Native Player->Scenario retained; not client combat/loot/visual proof.')

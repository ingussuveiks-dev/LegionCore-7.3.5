"""Verify the selected Merlis factory/death callback and native owner attribution."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, instructions = previous['native'], previous['instructions']
method = '?JustDied@npc_merlis_malaganAI@npc_merlis_malagan@@'
body = instructions(method)


def sites(prefix):
    target = native['symbol'](prefix)
    return [i for i, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == target]


owner, = sites('?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')
despawn, = sites('?DespawnAll@SummonList@@')
credit, = sites('?UpdateAchievementCriteria@Player@@')
conversation, = sites('?CreateConversation@Conversation@@')
assert owner < despawn < credit < conversation, 'Resolve killer before cleanup can retire it'
assert any(entry.op_str == 'rdx, rdx' and entry.mnemonic == 'test'
           and body[i + 1].mnemonic == 'je' and body[i + 1].operands[0].imm > body[owner].address
           for i, entry in enumerate(body[:owner])), 'Missing null-killer guard'
assert body[owner + 1].op_str == 'rdi, rax', 'Native resolved player must own progression and speech'
guard = next(i for i, entry in enumerate(body[despawn:credit], despawn) if entry.mnemonic == 'test' and entry.op_str == 'rdi, rdi')
assert body[guard + 1].mnemonic == 'je' and body[guard + 1].operands[0].imm > body[conversation].address
assert all(any(entry.op_str == operand for entry in body[guard:credit]) for operand in ('edx, 0x5c', 'r8d, 0xc99f', 'rcx, rdi'))
assert all(any(entry.op_str == operand for entry in body[credit:conversation]) for operand in ('r9, rdi', 'r8d, 0x990', 'rbx, [rdi + 0xd0]'))
assert any(entry.mnemonic == 'test' and entry.op_str == 'al, al' and body[i + 1].mnemonic == 'jne'
           for i, entry in enumerate(body[conversation + 1:], conversation + 1)), 'Failed conversation must keep its cleanup branch'
assert sites('?GetCreatureListWithEntryInGrid@WorldObject@@')[0] > conversation, 'Add cleanup must survive absent player'
tables = []
for entry in instructions('?GetAI@npc_merlis_malagan@@'):
    if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        slots = struct.unpack_from('<80Q', native['binary'], native['offset'](address))
        if slots[0x130 // 8] == native['symbol'](method):
            tables.append(address)
assert len(tables) == 1, 'Actual Merlis factory does not select repaired JustDied130'
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x130
           for entry in instructions('?Kill@Unit@@')), 'Native death dispatcher no longer uses selected callback slot'
scenario_targets = {native['resolve'](address) for name, address in native['symbols'].items()
                    if name.startswith('?UpdateAchievementCriteria@Scenario@@')}
assert scenario_targets & native['calls']('?UpdateAchievementCriteria@Player@@')
print('PASS supplied Release Merlis factory/JustDied130 and native Kill dispatcher: null-safe owner lookup BEFORE summon cleanup; same resolved player for51615 and2448; nonplayer/failed-conversation branches retain actor/add cleanup. Player still reaches native shared scenario progression. Staged path is not canonical deployment or client combat/loot/VFX proof.')

"""Verify the actual scoped Prustaga post-scaling wrapper and health comparison."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_titanstrike_pickup_admission_release.py')))
native = previous['native']
instructions = previous['instructions']
protect = '?Protect@TitanstrikePrustaga@@'
wrapper = '?DamageTakenAfterScaling@npc_titanstrike_final_prustaga@@'
body = instructions(protect)
values = {operand.imm for entry in body for operand in entry.operands if operand.type == CS_OP_IMM}
assert {106730, 1609, 1099, 1}.issubset(values), 'Wrong Prustaga protection scope'
assert native['symbol'](protect) in native['calls'](wrapper), 'Actual post-scaling override does not reach protection'
targets = native['calls'](protect)
for prefix in ('?GetScenario@ScenarioMgr@@', '?GetScenarioId@Scenario@@', '?GetHealth@Unit@@QEBA_KPEAV1@@', '?SetHealth@Unit@@'):
    assert native['symbol'](prefix) in targets, f'Missing live protection dependency: {prefix}'
health = native['symbol']('?GetHealth@Unit@@QEBA_KPEAV1@@')
position = next(index for index, entry in enumerate(body) if entry.mnemonic == 'call' and
                entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == health)
assert body[position + 1].mnemonic == 'cmp' and body[position + 1].op_str == 'rbx, rax'
assert body[position + 2].mnemonic == 'jb', 'Nonlethal final damage must leave native damage unchanged'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'dword ptr [rsi], r14d' for entry in body[position + 3:position + 6]), 'Lethal final damage is not zeroed'
assert any(entry.mnemonic == 'xor' and entry.op_str == 'r14d, r14d' for entry in body[position + 3:position + 6])
factory = '?GetAI@?$GenericCreatureScript@Unpc_titanstrike_final_prustaga@@@@'
assert native['symbol'](factory) != native['symbol']('?GetAI@?$GenericCreatureScript@Unpc_titanstrike_prustaga@@@@'), 'Final encounter factory collided with the earlier rune controller'
assert native['symbol']('?SetData@SmartAI@@') in native['calls']('?SetData@npc_titanstrike_prustaga@@'), 'Earlier rune-controller delegation disappeared'
assert native['symbol']('??0SmartAI@@') in native['calls'](factory), 'Factory does not construct the native SmartAI base'
tables = []
for entry in instructions(factory):
    if entry.mnemonic != 'lea':
        continue
    for operand in entry.operands:
        if operand.type != CS_OP_MEM or operand.mem.base != X86_REG_RIP:
            continue
        address = entry.address + entry.size + operand.mem.disp
        position = native['offset'](address)
        if position + 0xb0 <= len(native['binary']) and struct.unpack_from('<Q', native['binary'], position + 0xa8)[0] == native['symbol'](wrapper):
            tables.append(position)
assert len(tables) == 1, 'Actual factory does not install the post-scaling override at virtual slot0xa8'
assert struct.unpack_from('<Q', native['binary'], tables[0] + 0xa0)[0] == native['symbol']('?DamageTaken@SmartAI@@'), 'Wrapper replaces native pre-scaling SmartAI behavior'
assert b'npc_titanstrike_final_prustaga' in native['binary'], 'Factory registration name absent'
runpy.run_path(str(Path(__file__).with_name('damage_scaling_release_check.py')))['check'](native, instructions)
print('PASS: deployed Prustaga post-scaling vtable wrapper reaches actual106730/1609/1099 scope, compares final damage with target-relative health and zeros only lethal damage before SetHealth(1); native scaling and registry name remain linked')

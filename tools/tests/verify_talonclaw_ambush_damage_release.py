"""Check the linked ambush factory, native damage delegation and scaled floor."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_prustaga_damage_release.py')))
native = previous['native']
instructions = previous['instructions']
protect = '?Protect@TalonclawAmbush@@'
wrapper = '?DamageTakenAfterScaling@npc_talonclaw_ambush_dakarr@@'
before = '?DamageTaken@npc_talonclaw_ambush_dakarr@@'
factory = '?GetAI@?$GenericCreatureScript@Unpc_talonclaw_ambush_dakarr@@@@'
body = instructions(protect)
values = {operand.imm for entry in body for operand in entry.operands if operand.type == CS_OP_IMM}
assert {100376, 1511, 973, 9, 10, 70, 60, 1}.issubset(values), 'Ambush identity/stage/floor scope changed'
assert native['symbol'](protect) in native['calls'](wrapper), 'Post-scaling override does not reach ambush guard'
assert native['symbol']('?DamageTaken@SmartAI@@') in native['calls'](before), 'Native pre-scaling damage events were replaced'
for prefix in ('?GetScenario@ScenarioMgr@@', '?GetScenarioId@Scenario@@', '?GetCurrentStep@Scenario@@',
               '?GetHealth@Unit@@QEBA_KPEAV1@@', '?CountPctFromMaxHealth@Unit@@', '?SetHealth@Unit@@'):
    assert native['symbol'](prefix) in native['calls'](protect), f'Missing scaled guard dependency: {prefix}'
assert native['symbol']('?GetMaxHealth@Unit@@QEBA_KPEAV1@@') in native['calls']('?CountPctFromMaxHealth@Unit@@'), 'Floor ignores target-relative maximum health'
health = native['symbol']('?GetHealth@Unit@@QEBA_KPEAV1@@')
position = next(index for index, entry in enumerate(body) if entry.mnemonic == 'call' and
                entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == health)
sequence = [(entry.mnemonic, entry.op_str) for entry in body[position + 1:position + 12]]
assert ('mov', 'rbx, rax') in sequence and ('mov', 'r8, r12') in sequence, 'Health/floor must share the actual attacker'
assert ('sub', 'rbx, rax') in sequence and ('cmp', 'rcx, rbx') in sequence, 'Final damage is not compared with remaining health above floor'
assert any(entry.mnemonic == 'jb' for entry in body[position + 1:position + 14]), 'Ordinary non-crossing damage no longer returns unchanged'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'dword ptr [r15], ebx' for entry in body[position + 1:position + 16]), 'Crossing damage is not zeroed'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'dword ptr [r14], eax' for entry in body[:position]), 'Encounter provenance is not retained'
assert any(entry.mnemonic == 'cmp' and entry.op_str == 'ebx, eax' for entry in body[:position]), 'Changed-stage stale actors are not guarded'
tables = []
for entry in instructions(factory):
    if entry.mnemonic != 'lea':
        continue
    for operand in entry.operands:
        if operand.type != CS_OP_MEM or operand.mem.base != X86_REG_RIP:
            continue
        address = entry.address + entry.size + operand.mem.disp
        offset = native['offset'](address)
        if offset + 0xb0 <= len(native['binary']) and struct.unpack_from('<Q', native['binary'], offset + 0xa8)[0] == native['symbol'](wrapper):
            tables.append(offset)
assert len(tables) == 1, 'Ambush factory does not install the actual post-scaling override'
assert struct.unpack_from('<Q', native['binary'], tables[0] + 0xa0)[0] == native['symbol'](before), 'Factory omits provenance capture before native damage'
assert native['symbol']('??0SmartAI@@') in native['calls'](factory), 'Factory does not construct native SmartAI'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'dword ptr [rdi + 0x200], 0' for entry in instructions(factory)), 'New actor retains stale encounter state'
assert any(entry.mnemonic == 'lea' and entry.op_str == 'r9, [rcx + 0x200]' for entry in instructions(wrapper)), 'Post hook reads another provenance field'
assert native['symbol'](factory) != native['symbol']('?GetAI@?$GenericCreatureScript@Unpc_talonclaw_dakarr@@@@'), 'Earlier and final Dakarr factories collided'
assert b'npc_talonclaw_ambush_dakarr' in native['binary'], 'Ambush registry name absent'
print('PASS: canonical factory installs both ambush overrides, delegates native SmartAI damage, retains encounter provenance and applies target-relative70/60 floors after normalization; final Dakarr and Prustaga factories remain separate')

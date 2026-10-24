"""Inspect the selected soul-attack AI, actual owner gates and retry acknowledgement."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_xalatath_farthing_release.py')))
native = previous['native']
instructions = previous['instructions']
target = '?Target@XalatathSoulAttack@@'
factory = '?GetAI@?$GenericCreatureScript@Unpc_xalatath_soul_attack@@@@'
update = '?UpdateAI@npc_xalatath_soul_attack@@'
summoned = '?IsSummonedBy@npc_xalatath_soul_attack@@'
hit = '?SpellHitTarget@npc_xalatath_soul_attack@@'
reset = '?Reset@npc_xalatath_farthing@@'
output = Path(__file__).resolve().parents[2] / '.codex/xalatath-soul-attack-disassembly.log'
with output.open('w', encoding='utf-8') as stream:
    for prefix in (factory, target, summoned, hit, update, reset):
        stream.write(prefix + '\n')
        for entry in instructions(prefix):
            stream.write(f'{entry.address:x}: {entry.mnemonic} {entry.op_str}\n')

for caller, callee in ((factory, '??0SmartAI@@'), (summoned, target), (summoned, '?IsSummonedBy@SmartAI@@'),
                       (hit, target), (hit, '?SpellHitTarget@SmartAI@@'), (update, target), (update, '?UpdateAI@SmartAI@@'),
                       (target, '?GetSummoner@TempSummon@@'), (target, '?GetScenario@ScenarioMgr@@'),
                       (target, '?GetScenarioId@Scenario@@'), (target, '?GetCurrentStep@Scenario@@')):
    assert native['symbol'](callee) in native['calls'](caller), f'Missing actual native bridge: {caller} -> {callee}'

def immediates(prefix):
    return {operand.imm for entry in instructions(prefix) for operand in entry.operands if operand.type == CS_OP_IMM}

assert {102247, 101148, 1539, 991, 7, 61204}.issubset(immediates(target)), 'Actual owner/stage/aura scope missing'
assert {5000, 201899}.issubset(immediates(update)), 'Actual retry interval/spell missing'
assert 201899 in immediates(hit), 'Other spells can acknowledge the soul attack'
hit_body = instructions(hit)
owner_match = next(index for index, entry in enumerate(hit_body) if entry.mnemonic == 'cmp' and entry.op_str == 'rbx, rax')
assert hit_body[owner_match + 1].mnemonic == 'jne', 'A foreign hit can acknowledge the attack'
generation_gate = next(index for index, entry in enumerate(hit_body) if entry.mnemonic == 'cmp' and entry.op_str == 'dword ptr [rdi + 0x204], ecx')
assert generation_gate > owner_match and hit_body[generation_gate + 1].mnemonic == 'jne', 'Old-generation hit can acknowledge a new ending'
acknowledge = hit_body[generation_gate + 2]
assert acknowledge.mnemonic == 'mov' and acknowledge.op_str == 'byte ptr [rdi + 0x200], 1', 'Actual hit does not store acknowledgement'
update_body = instructions(update)
acknowledged_gate = next(index for index, entry in enumerate(update_body) if entry.mnemonic == 'cmp' and entry.op_str == 'byte ptr [rbx + 0x200], 0')
assert update_body[acknowledged_gate + 1].mnemonic == 'jne', 'Acknowledged attack still retries'
assert not any(entry.mnemonic == 'mov' and entry.op_str.startswith('byte ptr [rbx + 0x200],') for entry in update_body), 'Cast acceptance is being substituted for actual hit acknowledgement'
generation_retry = next(index for index, entry in enumerate(update_body) if entry.mnemonic == 'cmp' and entry.op_str == 'dword ptr [rbx + 0x204], edx')
assert update_body[generation_retry + 1].mnemonic == 'je' and update_body[generation_retry + 2].op_str == 'byte ptr [rbx + 0x201], 0', 'Old-generation retry remains active'
reset_body = instructions(reset)
assert reset_body[0].mnemonic == 'inc' and reset_body[0].op_str == 'dword ptr [rcx + 0x200]', 'Actual reset does not advance Farthing generation'
assert reset_body[1].mnemonic == 'jmp' and native['resolve'](reset_body[1].operands[0].imm) == native['symbol']('?Reset@SmartAI@@'), 'Scoped reset omits native reset'
spell_argument = next(index for index, entry in enumerate(update_body) if entry.mnemonic == 'mov' and entry.op_str == 'r8d, 0x314ab')
assert [entry.op_str for entry in update_body[spell_argument + 1:spell_argument + 3]] == ['rdx, rdi', 'rcx, qword ptr [rbx + 0x10]'], 'Retry no longer casts from soul to exact owner'
assert any(entry.mnemonic == 'xor' and entry.op_str == 'r9d, r9d' for entry in update_body[spell_argument - 5:spell_argument]), 'Retry silently changed to triggered cast'
owner_policy = instructions(target)
stage_gate = next(index for index, entry in enumerate(owner_policy) if entry.mnemonic == 'cmp' and entry.op_str == 'eax, 7')
assert owner_policy[stage_gate + 1].mnemonic == 'je' and owner_policy[stage_gate + 2].op_str == 'rbx, rsi', 'Stage mismatch does not return null'
expected = {native['symbol'](prefix) for prefix in (update, summoned, hit)}
tables = []
for entry in instructions(factory):
    for operand in entry.operands:
        if entry.mnemonic != 'lea' or operand.type != CS_OP_MEM or operand.mem.base != X86_REG_RIP:
            continue
        address = entry.address + entry.size + operand.mem.disp
        offset = native['offset'](address)
        if offset + 0x400 > len(native['binary']):
            continue
        values = struct.unpack_from('<128Q', native['binary'], offset)
        if expected.issubset(values):
            tables.append((offset, values))
assert len(tables) == 1, 'Factory does not install all three actual overrides'
for slot, prefix in ((0x18, update), (0x148, summoned), (0x178, hit)):
    assert tables[0][1][slot // 8] == native['symbol'](prefix), f'Wrong selected virtual slot: {prefix}'
assert native['symbol']('?SpellHit@SmartAI@@') in tables[0][1], 'Inherited native incoming-hit dispatch replaced'
assert native['symbol']('?Reset@SmartAI@@') in tables[0][1], 'Native reset replaced'
assert b'npc_xalatath_soul_attack' in native['binary']

with output.open('a', encoding='utf-8') as stream:
    for prefix in (update, summoned, hit):
        stream.write(f'{prefix}: vtable slot {tables[0][1].index(native["symbol"](prefix))*8:#x}\n')
print('PASS: canonical soul-attack factory selects owner-scoped summon/hit/retry overrides, spell201899/5s normal cast and real-hit acknowledgement; Farthing native-delegated reset generation cancels old future retries; actual branches/vtables recorded')

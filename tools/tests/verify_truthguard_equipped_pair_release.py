"""Check actual paired equipment before the registered Truthguard progression callback."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_truthguard_real_loot_release.py')))
native = previous['native']
instructions = previous['instructions']
helper = '?EquipPair@TruthguardAcquisition@@'
body = instructions(helper)


def calls(prefix, entries=body):
    return [index for index, entry in enumerate(entries) if entry.mnemonic == 'call'
            and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == native['symbol'](prefix)]


for dependency in ('?GetItemByEntry@Player@@', '?GetChildItem@Item@@', '?GetChildItemByGuid@Player@@',
                   '?GetGuidValue@Object@@', '?HasFlag@Object@@', '?IsEquipped@Item@@', '?CanEquipItem@Player@@',
                   '?CanEquipChildItem@Player@@', '?SwapItem@Player@@', '?EquipChildItem@Player@@', '?SendEquipError@Player@@'):
    assert calls(dependency), f'Paired delivery lost native dependency: {dependency}'
assert not calls('?AddItem@Player@@') and not calls('?StoreNewItem@Player@@'), 'Script creates replacement equipment'
lookup = calls('?GetItemByEntry@Player@@')[0]
assert any(entry.op_str == 'edx, 0x1f762' for entry in body[:lookup])
assert any(entry.op_str == 'r8d, r8d' and entry.mnemonic == 'xor' for entry in body[:lookup]), 'Parent lookup includes bank'
invalid = next(entry.operands[0].imm for index, entry in enumerate(body) if entry.mnemonic == 'je'
               and body[index - 1].op_str == 'rax, rax')
assert any(entry.mnemonic == 'cmp' and entry.op_str == 'eax, 0x1f763' and body[index + 1].mnemonic == 'jne'
           and body[index + 1].operands[0].imm == invalid for index, entry in enumerate(body[:-1]))
assert any(entry.mnemonic == 'je' and entry.operands[0].imm == invalid
           for entry in body[calls('?HasFlag@Object@@')[0] + 1:calls('?IsEquipped@Item@@')[0]])
assert any(entry.op_str == 'edx, 0x14' for entry in body[:calls('?GetGuidValue@Object@@')[0]]), 'Child creator field missing'
assert any(entry.op_str == 'r8d, 0x80000' for entry in body[:calls('?HasFlag@Object@@')[0]]), 'Native CHILD flag missing'
guid_checks = calls('?GetGuidValue@Object@@')
assert any(entry.mnemonic == 'jne' and entry.operands[0].imm == invalid
           for entry in body[guid_checks[0] + 1:calls('?HasFlag@Object@@')[0]]), 'Creator mismatch is not rejected'
error = calls('?SendEquipError@Player@@')[0]
error_branch = None
for position in calls('?CanEquipItem@Player@@') + calls('?CanEquipChildItem@Player@@'):
    assert body[position + 1].op_str == 'al, al' and body[position + 2].mnemonic == 'jne'
    target = body[position + 2].operands[0].imm
    if error_branch is None:
        error_branch = target
    assert target == error_branch and target < body[error].address
positions = calls('?GetItemByPos@Player@@QEBAPEAVItem@@EE@Z')
assert len(positions) == 2 and calls('?SwapItem@Player@@')[0] < positions[0]
for position, slot, pointer in zip(positions, ('0x10', '0xf'), ('rdi', 'r14')):
    assert any(entry.op_str == f'r8b, {slot}' for entry in body[position - 3:position])
    assert body[position + 1].mnemonic == 'cmp' and body[position + 1].op_str == f'rax, {pointer}'
    assert body[position + 2].mnemonic == 'jne'
assert body[positions[0] + 2].operands[0].imm == body[positions[1] + 2].operands[0].imm
assert body[positions[1] + 3].op_str == 'al, 1', 'No true result after both exact slot pointers'
callback = instructions('?OnLootStateChanged@go_sr_truthguard@@')
gate = calls(helper, callback)[0]
credit = calls('?DoUpdateAchievementCriteria@InstanceScript@@', callback)[0]
cast_gate = calls('?CastCredit@TruthguardAcquisition@@', callback)[0]
assert gate < cast_gate < credit
for position in (gate, cast_gate):
    assert callback[position + 1].op_str == 'al, al' and callback[position + 2].mnemonic == 'je'
retry_address = callback[gate + 2].operands[0].imm
assert callback[cast_gate + 2].operands[0].imm == retry_address
assert retry_address > callback[credit].address
delay = calls('?AddDelayedEvent@FunctionProcessor@@', callback)[0]
assert credit < delay and any(entry.op_str == 'edx, 1' for entry in callback[delay - 4:delay])
retry_start = next(index for index, entry in enumerate(callback) if entry.address == retry_address)
prohibited = {native['symbol'](prefix) for prefix in ('?DoUpdateAchievementCriteria@InstanceScript@@',
              '?CastCredit@TruthguardAcquisition@@', '?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N', '?DespawnOrUnsummon@Creature@@')}
assert not any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
               and native['resolve'](entry.operands[0].imm) in prohibited
               for entry in callback[retry_start:]), 'Retry branch reaches progression/cast'
lambdas = [name for name in native['symbols'] if name.startswith('?_Do_call@') and 'OnLootStateChanged@go_sr_truthguard@@' in name]
assert len(lambdas) == 1
tables = []
for entry in callback[retry_start:delay]:
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            address = entry.address + entry.size + operand.mem.disp
            if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x10)[0] == native['symbol'](lambdas[0]):
                tables.append(address)
assert len(tables) == 1, 'Scheduled closure does not select the checked retry callback'
retry = instructions(lambdas[0])
state = calls('?getLootState@GameObject@@', retry)[0]
assert retry[state + 1].op_str == 'eax, 3' and retry[state + 2].mnemonic == 'jne'
assert native['symbol']('?SetLootState@GameObject@@') in native['calls'](lambdas[0])
assert any(entry.op_str == 'edx, 1' for entry in retry), 'Retry does not restore READY'
assert native['symbol']('?Update@FunctionProcessor@@') in native['calls']('?Update@GameObject@@')
print('PASS: installed registered Truthguard callback gates progression on native linked/creator/CHILD parent-offhand and child-mainhand delivery; failure retains native errors and schedules scoped READY retry. Cast/scene acceptance, real player inventory and client remain separate.')

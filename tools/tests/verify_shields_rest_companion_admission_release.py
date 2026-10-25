"""Verify actual shared-map step dispatch only summons Truthguard companions in1082."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_guards_release.py')))
native = previous['native']
instructions = previous['instructions']
handler = '?onScenarionNextStep@instance_shields_rest_InstanceMapScript@instance_shields_rest@@'
body = instructions(handler)


def positions(prefix):
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == native['symbol'](prefix)]


scenario = positions('?GetScenario@ScenarioMgr@@')[0]
identity = positions('?GetScenarioId@Scenario@@')[0]
step = positions('?GetCurrentStep@Scenario@@')[0]
assert positions('?GetInstanceId@Map@@')[0] < scenario < identity < step
assert body[scenario + 2].op_str == 'rax, rax' and body[scenario + 3].mnemonic == 'je'
rejected = body[scenario + 3].operands[0].imm
assert body[identity + 1].op_str == 'eax, 0x43a' and body[identity + 2].mnemonic == 'jne'
assert body[identity + 2].operands[0].imm == rejected
assert any(entry.op_str == 'edi, edx' for entry in body[:scenario]), 'Notification step argument lost'
assert body[step + 1].op_str == 'eax, edi' and body[step + 2].mnemonic == 'jne'
assert body[step + 2].operands[0].imm == rejected
assert body[step + 3].op_str == 'edi, 1' and body[step + 4].mnemonic == 'jne'
assert body[step + 4].operands[0].imm == rejected
casts = positions('?DoCastSpellOnPlayers@InstanceScript@@')
assert len(casts) == 2 and step < casts[0] < casts[1]
for position, spell in zip(casts, (210036, 210043)):
    assert any(entry.op_str == f'edx, {hex(spell)}' for entry in body[position - 2:position])
exit_index = next(index for index, entry in enumerate(body) if entry.address == rejected)
assert all(entry.mnemonic in ('mov', 'add', 'pop', 'ret') for entry in body[exit_index:])
assert previous['slots'][0x118 // 8] == native['symbol'](handler), 'Actual shared instance factory selects another step handler'
for prefix in ('?UpdateCurrentStep@Scenario@@', '?SetCurrentStep@Scenario@@'):
    dispatch = instructions(prefix)
    assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x118
               for entry in dispatch), f'Native scenario dispatcher no longer selects this virtual slot: {prefix}'
cast_dispatch = instructions('?DoCastSpellOnPlayers@InstanceScript@@')
assert native['symbol']('?ApplyOnEveryPlayer@Map@@') in native['calls']('?DoCastSpellOnPlayers@InstanceScript@@')
closures = [name for name in native['symbols'] if name.startswith('?_Do_call@') and 'DoCastSpellOnPlayers@InstanceScript@@' in name]
assert len(closures) == 1
tables = []
for entry in cast_dispatch:
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            address = entry.address + entry.size + operand.mem.disp
            if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x10)[0] == native['symbol'](closures[0]):
                tables.append(address)
assert len(tables) == 1, 'Native all-player dispatch does not select checked casting closure'
assert native['symbol']('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N') in native['calls'](closures[0])
casting = instructions(closures[0])
assert any(entry.op_str == 'rdx, rax' for entry in casting) and any(entry.op_str == 'rcx, rax' for entry in casting)
assert not any('OnPlyerEnter@instance_shields_rest_InstanceMapScript' in name for name in native['symbols'])
print('PASS: installed shared instance step slot0x118 requires live1082/current matching step1 before native210036/210043 player-self casts;909/1068/missing/stale contexts reject before any cast. Group recipients, actual companion lifecycle and client remain separate.')

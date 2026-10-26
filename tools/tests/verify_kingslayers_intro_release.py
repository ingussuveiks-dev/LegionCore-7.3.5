"""Verify the selected Garona intro AI, GUID resolution and owned event queue."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, instructions = previous['native'], previous['instructions']
scope = 'npc_garona_halforcenAI@npc_garona_halforcen@@'
factory = instructions('?GetAI@npc_garona_halforcen@@')
los = instructions('?MoveInLineOfSight@' + scope)
update = instructions('?UpdateAI@' + scope)


def sites(body, prefix):
    target = native['symbol'](prefix)
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == target]


tables = []
for entry in factory:
    if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        slots = struct.unpack_from('<80Q', native['binary'], native['offset'](address))
        if slots[0x18 // 8] == native['symbol']('?UpdateAI@' + scope) and slots[0x270 // 8] == native['symbol']('?MoveInLineOfSight@' + scope):
            tables.append(slots)
assert len(tables) == 1, 'Factory does not select repaired LOS and UpdateAI'
assert any(entry.op_str == 'ecx, 0x70' for entry in factory)
schedule = sites(los, '?ScheduleEvent@EventMap@@QEAAXII')
assert len(schedule) == 2
for position, delay in zip(schedule, ('r8d, 0x3a98', 'r8d, 0x7530')):
    assert any(entry.op_str == delay for entry in los[position - 6:position])
    assert any(entry.op_str == 'rcx, [rbx + 0x50]' for entry in los[position - 6:position])
assert any(entry.mnemonic == 'movups' and entry.op_str == 'xmmword ptr [rbx + 0x39], xmm0' for entry in los)
assert not sites(los, '?AddDelayedEvent@FunctionProcessor@@')
execute = sites(update, '?ExecuteEvent@EventMap@@')
assert len(execute) == 2 and any(entry.mnemonic == 'jne' and entry.operands[0].type == CS_OP_IMM
                                 and entry.operands[0].imm < entry.address for entry in update[execute[-1]:])
lookup = sites(update, '?GetPlayer@ObjectAccessor@@')
assert len(lookup) == 2
for index, position in enumerate(lookup):
    end = lookup[index + 1] if index + 1 < len(lookup) else len(update)
    segment = update[position:end]
    for prefix in ('??8ObjectGuid@@', '?CanContact@Player@@', '?InSamePhase@WorldObject@@'):
        check, = sites(segment, prefix)
        assert segment[check + 1].op_str == 'al, al' and segment[check + 2].mnemonic == 'je'
    assert any(entry.op_str == 'qword ptr [rdi + 8], rax' and entry.mnemonic == 'cmp' for entry in segment)
    assert any(entry.op_str == 'xmm0, xmmword ptr [rsi + 0x39]' for entry in update[position - 7:position])
credits = sites(update, '?UpdateAchievementCriteria@Player@@')
assert len(credits) == 2
for position, criterion in zip(credits, ('r8d, 0xc8da', 'r8d, 0xcdc0')):
    assert all(any(entry.op_str == operand for entry in update[position - 8:position])
               for operand in (criterion, 'edx, 0x5c', 'rcx, rdi'))
assert {2377, 2384}.issubset({operand.imm for entry in update for operand in entry.operands if operand.type == CS_OP_IMM})
print('PASS selected Garona factory/LOS270/Update18: full128-bit recipient GUID, AI-owned EventMap50 with15000/30000ms, repeated event drain and two fresh native player lookups with full-GUID/contact/map/phase guards before personal conversation/51418/52672. Not deployed unless the supplied path is canonical; no client, quest-attempt, loot/equip or VFX certification.')

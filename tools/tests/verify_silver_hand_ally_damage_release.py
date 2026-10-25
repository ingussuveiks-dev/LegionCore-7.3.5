"""Verify four selected Silver Hand final-damage and wound/heal implementations."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_truthguard_surrender_release.py')))
native, instructions = previous['native'], previous['instructions']


def calls(body, prefix):
    target = native['symbol'](prefix)
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call'
            and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == target]


for script, speech in (('npc_travar_first', (9, 10)), ('npc_travar_second', (1, 2)),
                       ('npc_travar_third', (2, 3)), ('npc_efrin_and_duval', (0, 1))):
    callback = '?DamageTakenAfterScaling@' + script + 'AI@'
    body = instructions(callback)
    health = calls(body, '?GetHealth@Unit@@QEBA_KPEAV1@@')[0]
    assert not any(entry.operands and entry.operands[0].type == CS_OP_REG
                   and entry.reg_name(entry.operands[0].reg) in ('rdx', 'edx', 'dx', 'dl') for entry in body[:health])
    assert body[health + 1].op_str == 'rbx, rax' and body[health + 1].mnemonic == 'cmp'
    assert body[health + 2].mnemonic == 'jb'
    assert [entry.op_str for entry in body[health + 3:health + 5]] == ['eax, eax', 'dword ptr [rsi], eax']
    latch = body[health + 5]
    assert latch.mnemonic == 'cmp' and latch.operands[0].type == CS_OP_MEM and latch.op_str.endswith(', al')
    flag = latch.operands[0].mem.disp
    assert body[health + 6].mnemonic == 'jne' and body[health + 6].operands[0].imm == body[health + 2].operands[0].imm
    claim = next(index for index, entry in enumerate(body) if entry.op_str == f'byte ptr [rdi + {hex(flag)}], 1')
    aura = calls(body, '?AddAura@Unit@@QEAAPEAVAura@@I')[0]
    assert health + 6 < claim < aura
    assert any(entry.op_str == 'edx, 0x33829' for entry in body[claim - 4:aura])
    assert any(entry.op_str == 'r8, rcx' for entry in body[claim - 5:aura])
    talks = calls(body, '?Talk@CreatureAI@@QEAAXE')
    assert len(talks) == 2 and aura < talks[0] < talks[1]
    for index, value in zip(talks, speech):
        expected = 'edx, edx' if value == 0 else f'dl, {hex(value) if value >= 10 else value}'
        assert any(entry.op_str == expected for entry in body[index - 6:index])
    assert any(entry.mnemonic == 'or' and entry.op_str == 'dword ptr [rax + 0x3140], 8' for entry in body[talks[-1]:])
    assert len([entry for entry in body if entry.mnemonic == 'call']) == 4, 'Unexpected extra damage/credit side effect'

    tables = []
    for entry in instructions('?GetAI@' + script + '@@'):
        for operand in entry.operands:
            if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
                position = native['offset'](entry.address + entry.size + operand.mem.disp)
                if struct.unpack_from('<Q', native['binary'], position + 0xa8)[0] == native['symbol'](callback):
                    tables.append(position)
    assert len(tables) == 1, 'Selected factory must install the actual final-damage slot'
    assert struct.unpack_from('<Q', native['binary'], tables[0] + 0xa0)[0] == native['symbol']('?DamageTaken@UnitAI@@')
    heal_name = '?HealReceived@' + script + 'AI@'
    assert struct.unpack_from('<Q', native['binary'], tables[0] + 0xb0)[0] == native['symbol'](heal_name)
    heal = instructions(heal_name)
    percentage = calls(heal, '?GetHealthPct@Unit@@')[0]
    compare = heal[percentage + 1]
    assert compare.mnemonic == 'comiss' and compare.operands[1].type == CS_OP_MEM
    memory = compare.operands[1].mem
    assert memory.base == X86_REG_RIP
    assert struct.unpack_from('<f', native['binary'], native['offset'](compare.address + compare.size + memory.disp))[0] == 40.0
    assert heal[percentage + 2].mnemonic == 'jb'
    removal = calls(heal, '?RemoveAura@Unit@@QEAAXIVObjectGuid@@')[0]
    assert any(entry.op_str == 'edx, 0x33829' for entry in heal[percentage:removal])
    assert any(entry.op_str == 'dword ptr [rax + 0x3140], 0xfffffff7' and entry.mnemonic == 'and' for entry in heal[:removal])
    assert any(entry.op_str == f'byte ptr [rbx + {hex(flag)}], 0' for entry in heal[removal:])

print('PASS: all four installed Silver Hand factories select final-damage slot0xa8 and inherited empty raw slot0xa0; actual attacker-relative health, lethal-only zero-before-latch, native210985 self aura, exact Talk9/10,1/2,2/3,0/1 and stunned bit retained. Selected heal slot0xb0 retains pre-heal40pct, aura/stun/latch release. Native primary/suspended scaling dispatch independently checked; no credit, rendering or full client traversal proof.')

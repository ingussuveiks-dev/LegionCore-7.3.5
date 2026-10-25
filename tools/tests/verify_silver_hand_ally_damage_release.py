"""Verify four selected Silver Hand final-damage and wound/heal implementations."""
from pathlib import Path
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_truthguard_surrender_release.py')))
native, instructions = previous['native'], previous['instructions']
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True


def calls(body, prefix):
    target = native['symbol'](prefix)
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call'
            and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == target]


def leaf(prefix, length):
    address = native['symbol'](prefix)
    return list(decoder.disasm(native['binary'][native['offset'](address):native['offset'](address) + length], address))


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
    combined = script in ('npc_travar_second', 'npc_efrin_and_duval')
    pending = {'npc_travar_first': 0x5d, 'npc_travar_second': 0x59, 'npc_travar_third': 0x5a, 'npc_efrin_and_duval': 0x59}[script]
    assert not combined or pending == flag + 1
    claim = next(index for index, entry in enumerate(body) if entry.op_str == f'{"word" if combined else "byte"} ptr [rdi + {hex(flag)}], 1')
    if not combined:
        assert body[claim - 1].op_str == f'byte ptr [rdi + {hex(pending)}], al' and body[claim - 1].mnemonic == 'mov'
    aura = calls(body, '?AddAura@Unit@@QEAAPEAVAura@@I')[0]
    assert health + 6 < claim < aura
    assert any(entry.op_str == 'edx, 0x33829' for entry in body[claim - 4:aura])
    assert any(entry.op_str == 'r8, rcx' for entry in body[health + 6:aura])
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
    heal = leaf(heal_name, 17)
    assert [(entry.mnemonic, entry.op_str) for entry in (heal[0], heal[2], heal[4], heal[5])] == [
        ('cmp', f'byte ptr [rcx + {hex(flag)}], 0'), ('cmp', 'dword ptr [r8], 0'),
        ('mov', f'byte ptr [rcx + {hex(pending)}], 1'), ('ret', '')]
    assert all(heal[index].mnemonic == 'je' and heal[index].operands[0].imm == heal[-1].address for index in (1, 3))
    lifecycle = '?UpdateLifecycle@' + script + 'AI@'
    assert struct.unpack_from('<Q', native['binary'], tables[0] + 0x110)[0] == native['symbol'](lifecycle)
    if script in ('npc_travar_first', 'npc_travar_third'):
        adapter = instructions(lifecycle)
        recover, = calls(adapter, '?Recover@SilverHandCompanion@@')
        for operands in (f'r8, [rcx + {hex(flag)}]', f'rdx, [rcx + {hex(pending)}]', 'rcx, qword ptr [rcx + 0x20]'):
            assert any(entry.op_str == operands for entry in adapter[:recover])
        assert adapter[-1].mnemonic == 'jmp' and native['resolve'](adapter[-1].operands[0].imm) == native['symbol']('?Update@FunctionProcessor@@')
        entry_offset = 0xf0 if script == 'npc_travar_first' else 0xe0
        assert any(entry.op_str == f'rcx, [rbx + {entry_offset:#x}]' for entry in adapter[recover + 1:])
    else:
        adapter = leaf(lifecycle, 17)
        assert [(entry.mnemonic, entry.op_str) for entry in adapter[:3]] == [
            ('lea', f'r8, [rcx + {hex(flag)}]'), ('lea', f'rdx, [rcx + {hex(pending)}]'), ('mov', 'rcx, qword ptr [rcx + 0x20]')]
        assert adapter[3].mnemonic == 'jmp' and native['resolve'](adapter[3].operands[0].imm) == native['symbol']('?Recover@SilverHandCompanion@@')
    reset_name = '?Reset@' + script + 'AI@'
    assert struct.unpack_from('<Q', native['binary'], tables[0] + 0x28)[0] == native['symbol'](reset_name)
    reset = instructions(reset_name)
    assert len(calls(reset, '?Reset@EventMap@@')) == 1
    assert any(entry.op_str == f'byte ptr [rbx + {hex(pending)}], 0' for entry in reset)
    factory = instructions('?GetAI@' + script + '@@')
    assert any(entry.mnemonic == 'xor' and entry.op_str == 'ebp, ebp' for entry in factory)
    assert any(entry.op_str == f'byte ptr [rsi + {hex(pending)}], bpl' or
               (combined and entry.op_str == 'word ptr [rsi + 0x58], bp') for entry in factory)

recovery = instructions('?Recover@SilverHandCompanion@@')
assert recovery[3].op_str == 'byte ptr [rdx], 0' and recovery[3].mnemonic == 'cmp'
assert recovery[6].mnemonic == 'je'
assert recovery[7].op_str == 'byte ptr [rdx], 0' and recovery[7].mnemonic == 'mov'
assert recovery[8].op_str == 'byte ptr [r8], 0' and recovery[8].mnemonic == 'cmp'
assert recovery[9].mnemonic == 'je'
world, = calls(recovery, '?IsInWorld@Object@@')
deleted, = calls(recovery, '?IsPreDelete@Object@@')
percentage, = calls(recovery, '?GetHealthPct@Unit@@')
assert 9 < world < deleted < percentage
for position, branch in ((world, 'je'), (deleted, 'jne')):
    assert recovery[position + 1].op_str == 'al, al' and recovery[position + 2].mnemonic == branch
    assert recovery[position + 2].operands[0].imm == recovery[6].operands[0].imm
assert any(entry.mnemonic == 'cmp' and entry.op_str == 'dword ptr [rbx + 0xa18], 0' for entry in recovery[deleted:percentage])
assert any(entry.mnemonic == 'jne' and entry.operands[0].imm == recovery[6].operands[0].imm for entry in recovery[deleted + 3:percentage])
threshold = recovery[percentage + 1]
assert threshold.mnemonic == 'movss' and threshold.operands[1].mem.base == X86_REG_RIP
assert struct.unpack_from('<f', native['binary'], native['offset'](threshold.address + threshold.size + threshold.operands[1].mem.disp))[0] == 40.0
assert recovery[percentage + 2].op_str == 'xmm1, xmm0' and recovery[percentage + 2].mnemonic == 'comiss'
assert recovery[percentage + 3].mnemonic == 'ja' and recovery[percentage + 3].operands[0].imm == recovery[6].operands[0].imm
assert recovery[percentage + 4].op_str == 'byte ptr [rdi], 0'
removal, = calls(recovery, '?RemoveAura@Unit@@QEAAXIVObjectGuid@@')
assert any(entry.op_str == 'edx, 0x33829' for entry in recovery[percentage:removal])
assert any(entry.op_str == 'dword ptr [rbx + 0x3140], 0xfffffff7' and entry.mnemonic == 'and' for entry in recovery[percentage:removal])
assert len([entry for entry in recovery if entry.mnemonic == 'call']) == 4
heal = instructions('?DealHeal@Unit@@')
received, = [index for index, entry in enumerate(heal) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0xb0]
done, = [index for index, entry in enumerate(heal) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0xb8]
apply, = calls(heal, '?ModifyHealth@Unit@@')
assert received < done < apply
creature = instructions('?Update@Creature@@')
ticks = [index for index, entry in enumerate(creature) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x110]
assert len(ticks) == 2
assert any(entry.op_str == 'byte ptr [rsi + 0x3a9d], 1' for entry in creature[ticks[0] - 10:ticks[0]])
assert creature[ticks[1] + 1].op_str == 'byte ptr [rsi + 0x3a9d], 0'
assert any(entry.op_str == 'rcx, qword ptr [rsi + 0x9c8]' for entry in creature[ticks[0] - 8:ticks[0]])
assert any(entry.op_str == 'rcx, qword ptr [rsi + 0x9c0]' for entry in creature[ticks[0]:ticks[1]])
assert min(calls(creature, '?Update@Unit@@')) > ticks[1]

print('PASS: all four installed Silver Hand factories retain native final-damage protection/speech/self210985/stun and select positive-heal17-byte leaf0xb0 plus lifecycle0x110 recovery (first/third also update separate entry queues). Heal pending state is initialized/reset/re-wound; shared recovery consumes it before live/death/actual40pct gates, clears wound latch before original aura/stun cleanup. Native DealHeal received/done/application order and locked primary/disabled lifecycle are executable. No heal-related delayed Player/AI callback, artificial healing, credit, rendering or full client traversal proof.')

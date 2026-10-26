"""Verify last Travard's selected AI-owned wave timer in the canonical PE."""
from pathlib import Path
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, instructions = previous['native'], previous['instructions']


def sites(body, prefix):
    target = native['symbol'](prefix)
    return [index for index, entry in enumerate(body) if entry.mnemonic in ('call', 'jmp')
            and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == target]


def leaf(prefix, size):
    address = native['symbol'](prefix)
    decoder = Cs(CS_ARCH_X86, CS_MODE_64)
    decoder.detail = True
    body = list(decoder.disasm(native['binary'][native['offset'](address):native['offset'](address) + size], address))
    assert sum(entry.size for entry in body) == size
    return body


los_name = '?MoveInLineOfSight@npc_travar_lastAI@npc_travar_last@@'
lifecycle_name = '?UpdateLifecycle@npc_travar_lastAI@'
destructor_name = '??_Gnpc_travar_lastAI@'
factory = instructions('?GetAI@npc_travar_last@@')
los = instructions(los_name)
selected = []
for entry in factory:
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        slots = struct.unpack_from('<80Q', native['binary'], native['offset'](address))
        if slots[0x270 // 8] == native['symbol'](los_name):
            selected.append(slots)
slots, = selected
assert slots[0x110 // 8] == native['symbol'](lifecycle_name) and slots[0] == native['symbol'](destructor_name)
construct, = sites(factory, '??0FunctionProcessor@@')
assert factory[construct - 1].op_str == 'rcx, [rdi + 0x48]'
assert all(any(entry.op_str == operands for entry in factory) for operands in
           ('ecx, 0xd8', 'word ptr [rdi + 0x38], 0', 'dword ptr [rdi + 0x3c], 0x3e8', 'byte ptr [rdi + 0x40], 0'))
arm, = sites(los, '?AddDelayedEvent@FunctionProcessor@@')
assert any(entry.op_str == 'rcx, [rsi + 0x48]' for entry in los[arm - 10:arm])
assert los[arm - 1].op_str == 'edx, 0x1388'
assert not sites(los, '?AddDelayedEvent@Unit@@')
assert any(entry.op_str == 'byte ptr [rdx + 0x16], 4' for entry in los[:arm])
assert any(entry.op_str == 'word ptr [rsi + 0x38], 0x101' for entry in los[:arm])
assert any(entry.op_str == 'qword ptr [rsp + 0x68], rsi' for entry in los[arm - 10:arm])
closure, = [name for name in native['symbols'] if name.startswith('?_Do_call@') and 'MoveInLineOfSight@npc_travar_lastAI@npc_travar_last@@' in name]
tables = []
for index, entry in enumerate(los[:arm]):
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x10)[0] == native['symbol'](closure):
            assert los[index + 1].op_str == 'qword ptr [rsp + 0x60], rax'
            tables.append(address)
assert len(tables) == 1
callback = leaf(closure, 11)
assert [(entry.mnemonic, entry.op_str) for entry in callback[:2]] == [('mov', 'rcx, qword ptr [rcx + 8]'), ('mov', 'dl, 1')]
assert callback[2].mnemonic == 'jmp' and callback[2].operands[0].imm == native['symbol']('?SummonAdds@npc_travar_lastAI@')
lifecycle = leaf(lifecycle_name, 9)
assert lifecycle[0].mnemonic == 'add' and lifecycle[0].op_str == 'rcx, 0x48'
assert lifecycle[1].mnemonic == 'jmp' and native['resolve'](lifecycle[1].operands[0].imm) == native['symbol']('?Update@FunctionProcessor@@')
destroy = instructions(destructor_name)
release, = sites(destroy, '??1FunctionProcessor@@')
assert destroy[release - 1].mnemonic == 'lea' and destroy[release - 1].op_str == 'rcx, [rdi + 0x48]'
creature = instructions('?Update@Creature@@')
ticks = [index for index, entry in enumerate(creature) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x110]
assert len(ticks) == 2
assert any(entry.op_str == 'byte ptr [rsi + 0x3a9d], 1' for entry in creature[ticks[0] - 10:ticks[0]])
assert creature[ticks[1] + 1].op_str == 'byte ptr [rsi + 0x3a9d], 0'
assert any(entry.op_str == 'rcx, qword ptr [rsi + 0x9c8]' for entry in creature[ticks[0] - 8:ticks[0]])
assert any(entry.op_str == 'rcx, qword ptr [rsi + 0x9c0]' for entry in creature[ticks[0]:ticks[1]])
assert min(sites(creature, '?Update@Unit@@')) > ticks[1]
print('PASS: canonical last Travard factory selects LOS0x270, lifecycle0x110 and destructor0; actual LOS captures AI and queues5000ms at AI+0x48, selected11-byte closure invokes original SummonAdds(1), selected9-byte lifecycle drains the same native queue under primary/disabled AI lock, and destructor releases it. Original late admission/criteria/wave recovery and client traversal are not certified.')

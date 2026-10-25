"""Verify exact pending-join identity removal in the installed deleting destructor."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_truthguard_ride_cast_release.py')))
native, instructions = previous['native'], previous['instructions']
destructor = '??_GVehicleJoinEvent@@'
body = instructions(destructor)

def locate(operand, mnemonic=None):
    return next(index for index, entry in enumerate(body) if entry.op_str == operand and (not mnemonic or entry.mnemonic == mnemonic))

owner = locate('rdi, rcx', 'mov')
kit = locate('rbx, qword ptr [rax + 0x30e0]', 'mov')
mutex = locate('rbp, [rbx + 0x70]', 'lea')
pending = locate('rax, [rbx + 0x60]', 'lea')
identity = locate('qword ptr [rdx + 0x10], rdi', 'cmp')
count = locate('qword ptr [rax + 8]', 'dec')
advance = locate('rdx, r8', 'mov')
assert owner < kit < mutex < pending < identity < count < advance
assert body[identity + 1].mnemonic == 'jne' and body[identity + 1].operands[0].imm == body[advance].address
assert body[advance + 1].op_str == 'r8, r9' and body[advance + 2].mnemonic == 'jne'
assert body[advance + 2].operands[0].imm == body[identity - 1].address
assert sum(entry.mnemonic == 'dec' and entry.op_str == 'qword ptr [rax + 8]' for entry in body) == 1
assert not any(entry.mnemonic == 'mov' and entry.op_str in ('qword ptr [rbx + 0x68], 0', 'qword ptr [rax + 8], 0') for entry in body)
free = locate('edx, 0x18', 'mov')
unlock = next(index for index in range(free, len(body)) if body[index].op_str == 'rcx, rbp')
assert identity < free < unlock and body[free + 2].mnemonic == 'call' and body[unlock + 1].mnemonic == 'call'
assert body[free + 1].op_str == 'rcx, rsi'
assert body[mutex + 2].mnemonic == 'call', 'Original registry mutex no longer acquired'

table = re.search(r'\s\?\?_7VehicleJoinEvent@@6B@\s+([0-9a-fA-F]{16})', native['args'].release.joinpath('worldserver.map').read_text(errors='replace'))
assert table, 'Selected native join-event vtable missing'
slots = struct.unpack_from('<4Q', native['binary'], native['offset'](int(table[1], 16)))
assert native['resolve'](slots[0]) == native['symbol'](destructor)
assert native['resolve'](slots[1]) == native['symbol']('?Execute@VehicleJoinEvent@@')
assert native['resolve'](slots[3]) == native['symbol']('?Abort@VehicleJoinEvent@@')
passenger = instructions('?RemovePendingEventsForPassenger@Vehicle@@')
assert any(entry.op_str == 'qword ptr [rax + 0x50], rbp' for entry in passenger)
assert any(entry.op_str == 'byte ptr [rax + 8], 1' for entry in passenger)
assert any(entry.mnemonic == 'dec' and entry.op_str == 'qword ptr [rdi + 0x68]' for entry in passenger)
assert any(entry.mnemonic == 'jmp' and entry.operands[0].type == CS_OP_IMM for entry in passenger)
print('PASS: actual native join-event vtable selects deleting destructor with current-kit/mutex/list lookup and event-this pointer comparison before matched-node unlink/count/free; mismatch advances without clearing unrelated joins. Native passenger cancellation and existing artifact/ride paths retained. Not accepted-join, renderer or client traversal proof.')

"""Verify usable-seat admission before installed passenger publication/cancellation."""
from pathlib import Path
import runpy
from capstone import Cs, CS_ARCH_X86, CS_MODE_64

previous = runpy.run_path(str(Path(__file__).with_name('verify_vehicle_join_failure_release.py')))
native, body = previous['native'], previous['body']
locate, gate = previous['locate'], previous['seat_gate']
publication, failure = previous['publication'], previous['failure']
assert [entry.op_str for entry in body[gate - 2:gate]] == ['rcx, qword ptr [rdi + 0x58]', 'rcx, qword ptr [rcx + 0x28]']
assert body[gate + 1].op_str == 'al, al' and body[gate + 2].mnemonic == 'je'
count = locate(body, 'dword ptr [rax + 0x18], 0', 'cmp')
assert gate < count < publication and body[count - 1].op_str == 'rax, qword ptr [rdi + 0x40]'
assert body[count + 1].mnemonic == 'je' and body[count + 1].operands[0].imm == failure
assert body[gate + 2].operands[0].imm == body[count + 2].address
cancellation = locate(body, 'byte ptr [rdx + 8], 1', 'mov')
assert count < cancellation < publication
decrement = locate(body, 'dword ptr [rax + 0x18]', 'dec')
assert publication < decrement and body[decrement - 1].op_str == 'rax, qword ptr [rdi + 0x40]'
address = native['symbol']('?CanEnterOrExit@VehicleSeatEntry@@')
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
predicate = []
for entry in decoder.disasm(native['binary'][native['offset'](address):native['offset'](address) + 32], address):
    predicate.append(entry)
    if entry.mnemonic == 'ret':
        break
assert predicate[-1].mnemonic == 'ret'
assert any(entry.op_str == 'eax, dword ptr [rcx + 4]' for entry in predicate)
assert any(entry.mnemonic == 'shr' and entry.op_str == 'eax, 0x19' for entry in predicate)
assert any(entry.mnemonic == 'and' and entry.op_str in ('al, 1', 'eax, 1') for entry in predicate)
print('PASS: selected installed native CanEnterOrExit bit0x02000000 gates zero UsableSeatNum before pending cancellation, passenger/seat publication or decrement; non-enterable Truthguard seats bypass the count. Existing Abort/true deletion and success dispatch retained; charm/admission-prestate/client remain separate.')

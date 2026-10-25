"""Verify scoped foreign control admission and preserved native fatal diagnostic."""
from pathlib import Path
import runpy
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_vehicle_usable_seat_admission_release.py')))
native, body = previous['native'], previous['body']
locate, call = previous['locate'], previous['previous']['call']
publication = previous['publication']
failure = previous['failure']
target = locate(body, 'byte ptr [rcx + 0x16], 3', 'cmp')
player = locate(body, 'rsi, rsi', 'test')
control = locate(body, 'dword ptr [rdx + 4], 0x800', 'test')
owner = locate(body, 'edx, 0x18', 'mov')
present = call(body, '??BObjectGuid@@QEBA_NXZ')
different = call(body, '??9ObjectGuid@@')
cancellation = locate(body, 'byte ptr [rdx + 8], 1', 'mov')
assert target < player < control < owner < present < different < cancellation < publication
assert body[target - 1].op_str == 'rcx, qword ptr [rdi + 0x48]'
allowed = body[target + 1].operands[0].imm
for index, condition in ((target, 'jne'), (player, 'je'), (control, 'je'), (present + 1, 'je'), (different + 1, 'je')):
    assert body[index + 1].mnemonic == condition and body[index + 1].operands[0].imm == allowed
assert [entry.op_str for entry in body[owner + 1:owner + 4]] == [hex(native['symbol']('?GetGuidValue@Object@@')), 'xmm0, xmmword ptr [rax]', 'xmmword ptr [rsp + 0x60], xmm0']
assert body[present - 1].op_str == 'rcx, [rsp + 0x60]'
assert [entry.op_str for entry in body[different - 2:different]] == ['rdx, rax', 'rcx, [rsp + 0x60]']
assert body[different - 4].op_str == 'rcx, qword ptr [rdi + 0x50]'
diagnostic = call(body, '?outMessage@Log@@')
assert different < diagnostic < next(index for index, entry in enumerate(body) if entry.address == failure)
assert body[diagnostic - 3].op_str == 'r8d, 6'
message = b'Unit::SetCharmedBy: %u (GUID %u) has already been charmed but %u (GUID %u) is trying to charm it!'
references = []
for entry in body[different:diagnostic]:
    if entry.mnemonic != 'lea':
        continue
    for operand in entry.operands:
        if operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            position = native['offset'](entry.address + entry.size + operand.mem.disp)
            if native['binary'][position:position + len(message)] == message:
                references.append(entry.address)
assert len(references) == 1, 'Foreign rejection must reference the retained actual fatal message'
assert any(entry.mnemonic == 'je' and entry.operands[0].type == CS_OP_IMM and entry.operands[0].imm == failure for entry in body[different:diagnostic])
assert not any(entry.mnemonic.startswith('j') and entry.operands[0].type == CS_OP_IMM and entry.operands[0].imm == allowed for entry in body[different + 3:diagnostic])

decoder = Cs(CS_ARCH_X86, CS_MODE_64)
def leaf(prefix):
    address = native['symbol'](prefix)
    output = []
    for entry in decoder.disasm(native['binary'][native['offset'](address):native['offset'](address) + 64], address):
        if entry.mnemonic == 'int3':
            break
        output.append(entry)
    return output

comparison = leaf('??9ObjectGuid@@')
nonempty = leaf('??BObjectGuid@@QEBA_NXZ')
for operand in ('rax, qword ptr [rdx]', 'qword ptr [rcx], rax', 'rax, qword ptr [rdx + 8]', 'qword ptr [rcx + 8], rax'):
    assert any(entry.op_str == operand for entry in comparison), 'Native GUID comparison lost a64-bit half'
assert any('qword ptr [rcx + 8]' in entry.op_str for entry in nonempty)
assert any('qword ptr [rcx]' in entry.op_str for entry in nonempty)
print('PASS: selected Creature/Player/CAN_CONTROL preflight reads actual CHARMED_BY24 and compares native128-bit GUIDs before cancellation/publication; empty/same-owner/non-control/non-Player branches bypass it. Original fatal6 diagnostic and Abort/true deletion retained. Same-owner and post-callback failures, actual charm binding and client remain separate.')

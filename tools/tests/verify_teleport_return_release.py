"""Certify scalar/overload return forwarding in the actual selected Release."""
from pathlib import Path
import runpy
from capstone import CS_OP_REG
from sharasdal_erunak_entry_pe import sites

previous = runpy.run_path(str(Path(__file__).with_name('verify_transport_passengers_release.py')))
n = previous['n']
scalar = '?TeleportTo@Player@@QEAA_NIMMMMII@Z'
body = n['instructions'](scalar)
call, = sites(n, body, '?SafeTeleport@Player@@')
assert body[call + 1].mnemonic == 'jmp'
epilogue = body[call + 1].operands[0].imm
tail = body[next(i for i, op in enumerate(body) if op.address == epilogue):]
assert any(x.mnemonic == 'ret' for x in tail)
assert not any(x.operands and x.operands[0].type == CS_OP_REG and x.reg_name(x.operands[0].reg) in ('al', 'ax', 'eax', 'rax') for x in tail), 'Delegated result overwritten before return'
# The compatibility early-success path remains separate from delegated result.
assert any(x.mnemonic == 'mov' and x.op_str == 'al, 1' and body[call].address < x.address < epilogue for x in body)
for prefix in ('?TeleportTo@Player@@QEAA_NAEBVWorldLocation@@', '?TeleportTo@Player@@QEAA_NII@Z', '?TeleportTo@Player@@QEAA_NIPEBUPosition@@'):
    overload = n['instructions'](prefix)
    at, = sites(n, overload, scalar)
    tail = overload[at + 1:]
    # SafeLoc's absent-record branch returns false; the forwarded success path
    # returns immediately without writing al/eax, like the other overloads.
    for op in tail:
        if op.mnemonic == 'ret':break
        assert not (op.operands and op.operands[0].type == CS_OP_REG and op.reg_name(op.operands[0].reg) in ('al', 'ax', 'eax', 'rax'))
    else:raise AssertionError('Overload result has no immediate return')
print('PASS selected Release: scalar returns SafeTeleport result unchanged; all three overloads forward it, compatibility early success remains separate')

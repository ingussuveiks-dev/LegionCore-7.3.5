"""Bind Hunter cemetery selection to the actual linked native stage branch."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

p = runpy.run_path(str(Path(__file__).with_name('verify_tuure_graveyard_release.py')))
n, body, sites, at = (p[k] for k in ('n', 'body', 'sites', 'at'))
mark = p['marksman']
assert mark is not None, 'Release lacks stage-aware Marksmanship cemetery'
step, = sites('?GetCurrentStep@Scenario@@')
assert step == mark + 3
assert [(o.mnemonic, o.op_str) for o in body[step + 1:step + 4]] == [('cmp', 'eax, 5'), ('sbb', 'edx, edx'), ('add', 'edx, 0x153a')]
lookup = at('rdi, qword ptr [rax + rdx*8]', 'mov')
identity = at('dword ptr [rdi], edx', 'cmp')
failure = body[identity + 1].operands[0].imm
assert body[identity + 1].mnemonic == 'jne' and body[lookup + 2].operands[0].imm == failure
assert body[lookup - 2].mnemonic == 'jae' and body[lookup - 2].operands[0].imm == failure
mapcheck = next(i for i, o in enumerate(body) if i > identity and o.mnemonic == 'cmp' and o.op_str == 'word ptr [rdi + 0x1c], si')
assert body[mapcheck - 1].op_str == 'esi, 0x5d1' and body[mapcheck + 1].operands[0].imm == failure
coord = p['coordinates'][1]
assert step < lookup < identity < mapcheck < coord
assert any(o.mnemonic == 'je' and o.operands[0].imm == failure for o in body[mapcheck + 2:coord])
assert body[coord + 2].mnemonic == 'jne' and body[coord + 2].operands[0].imm == body[p['coord'] + 3].address, 'Validated Hunter source must reach the same checked native XYZ/facing stores'
assert body[coord + 3].address == failure
for register, expected in (('xmm6', 3.141592653589793), ('xmm7', 180.0)):
    o = next(o for o in body[mapcheck:coord] if o.mnemonic == 'movss' and o.op_str.startswith(register + ',') and 'rip' in o.op_str)
    a = next(a for a in o.operands if a.type == CS_OP_MEM and a.mem.base == X86_REG_RIP)
    value, = struct.unpack_from('<f', n['binary'], n['offset'](o.address + o.size + a.mem.disp))
    assert abs(value - expected) < 1e-5
refused = body[p['refused']].address
assert any(o.mnemonic == 'je' and o.operands[0].imm == refused for o in body[coord:])
assert b'Marksmanship graveyard: invalid native WorldSafeLocs %u for instance %u' in n['binary']
print('PASS linked Marksmanship cemetery:actual972 guard,GetCurrentStep>=5 chooses5434 else5433,indexed native source identity/map/zero/finite guards,radians and shared proven stores,diagnostic/null failure,registered dispatch and Holy branch retained;no direct resurrection/progression. Native47927 boundary and real release/ACK tested separately;full client remains separate.')

"""Bind Hodir cemetery selection to the actual linked native stage branch."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

p = runpy.run_path(str(Path(__file__).with_name('verify_blood_graveyard_release.py')))
n, body, sites, at = (p[k] for k in ('n', 'body', 'sites', 'at'))
holy = p['holy']
hodir = holy['hodir']
assert hodir is not None, 'Release lacks Hodir landing-aware cemetery recovery'
step = next(i for i in sites('?GetCurrentStep@Scenario@@') if hodir < i <= hodir + 3)
assert [(o.mnemonic, o.op_str) for o in body[step + 1:step + 4]] == [('cmp', 'eax, 3'), ('sbb', 'edx, edx'), ('add', 'edx, 0x1617')]

data_reg, map_reg, map_word = (holy[k] for k in ('data_reg', 'map_reg', 'map_word'))
lookup = next(i for i, o in enumerate(body) if i > step and o.mnemonic == 'mov' and o.op_str == data_reg + ', qword ptr [rax + rdx*8]')
identity = next(i for i, o in enumerate(body) if i > lookup and o.mnemonic == 'cmp' and o.op_str == 'dword ptr [' + data_reg + '], edx')
failure = body[identity + 1].operands[0].imm
assert body[identity + 1].mnemonic == 'jne' and body[lookup + 2].operands[0].imm == failure
assert body[lookup - 2].mnemonic == 'jae' and body[lookup - 2].operands[0].imm == failure
mapcheck = next(i for i, o in enumerate(body) if i > identity and o.mnemonic == 'cmp' and o.op_str == 'word ptr [' + data_reg + ' + 0x1c], ' + map_word)
assert body[mapcheck - 1].op_str == map_reg + ', 0x5d1' and body[mapcheck + 1].operands[0].imm == failure
coord = holy['coordinates'][4]
assert step < lookup < identity < mapcheck < coord
assert any(o.mnemonic == 'je' and o.operands[0].imm == failure for o in body[mapcheck + 2:coord])
assert body[coord + 2].mnemonic == 'jne' and body[coord + 2].operands[0].imm == body[holy['coord'] + 3].address, 'Validated Hodir source must reach the same checked native XYZ/facing stores'
assert body[coord + 3].address == failure
for register, expected in (('xmm6', 3.141592653589793), ('xmm7', 180.0)):
    o = next(o for o in body[mapcheck:coord] if o.mnemonic == 'movss' and o.op_str.startswith(register + ',') and 'rip' in o.op_str)
    a = next(a for a in o.operands if a.type == CS_OP_MEM and a.mem.base == X86_REG_RIP)
    value, = struct.unpack_from('<f', n['binary'], n['offset'](o.address + o.size + a.mem.disp))
    assert abs(value - expected) < 1e-5
refused = body[holy['refused']].address
assert any(o.mnemonic == 'je' and o.operands[0].imm == refused for o in body[coord:])
assert b'Hodir graveyard: invalid native WorldSafeLocs %u for instance %u' in n['binary']
print('PASS linked Hodir cemetery:actual registered1146 guard,GetCurrentStep>=3 chooses5655 else5654,indexed native identity/map/zero/finite guards,radians and proven shared stores,diagnostic/null failure;Holy/MM/Blood dispatch and progression exclusions retained. Native landing boundary/release/ACK tested separately;full client remains separate.')

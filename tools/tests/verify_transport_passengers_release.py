"""Selected x64 passenger snapshot/identity/continuation proof in Release."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM
from sharasdal_erunak_entry_pe import sites

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
body = n['instructions']('?TeleportTransport@Transport@@')
pairs = [(x.mnemonic, x.op_str) for x in body]
identities = sites(n, body, '??9ObjectGuid@@')
assert len(identities) == 2
assert ('movups', 'xmmword ptr [rdx + 8], xmm0') in pairs, 'Snapshot must copy full GUID after pointer'
for at, iterator in zip(identities, ('rdi', 'rsi')):
    assert body[at - 1].op_str == 'rcx, rax'
    assert sites(n, body[max(0, at - 5):at], '?GetGuidValue@Object@@')
    assert ('lea', f'rdx, [{iterator} + 8]') in pairs[at - 4:at]
    # All three set-find rejection edges must skip the dereference/travel and
    # reach the independent vector increment. Identity mismatch does the same.
    assert body[at + 1].mnemonic == 'test' and body[at + 2].mnemonic == 'jne'
    advance = body[at + 2].operands[0].imm
    target = next(x for x in body if x.address == advance)
    assert (target.mnemonic, target.op_str) == ('add', f'{iterator}, 0x18')
    rejected = [x for x in body[max(0, at - 35):at]
                if x.mnemonic in ('je', 'jb', 'jne') and x.operands[0].type == CS_OP_IMM and x.operands[0].imm == advance]
    assert len(rejected) == 3, 'Missing membership-before-dereference rejection'
far, = sites(n, body, '?TeleportTo@Player@@QEAA_NIMMMMII@Z')
near, = sites(n, body, '?NearTeleportTo@Unit@@QEAAXAEBUPosition@@')
assert identities[0] < far < identities[1] < near
assert body[far + 1].op_str == 'al, al' and body[far + 2].mnemonic == 'jne'
assert sites(n, body[far + 2:identities[1]], '?RemovePassenger@Transport@@')
print('PASS selected Release: both passenger branches traverse 24-byte pointer/full128-GUID snapshots; set membership precedes dereference, mismatch skips to vector advance, rejected travel removes passenger')

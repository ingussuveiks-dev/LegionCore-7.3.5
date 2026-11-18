"""Verify the nullable InstanceMap RTTI receipt in the actual final link."""
from pathlib import Path
import runpy

from sharasdal_erunak_entry_pe import sites

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
body = n['instructions']('?GetInstanceScript@WorldObject@@')
assert sites(n, body, '?IsDungeon@Map@@')
# The live script pointer load must follow a tested dynamic-cast result. The
# failed RTTI, missing-map and non-dungeon paths return the same null value.
load, = [i for i, op in enumerate(body)
         if op.mnemonic == 'mov' and op.op_str == 'rax, qword ptr [rax + 0x18ba0]']
assert [(op.mnemonic, op.op_str) for op in body[load - 3:load - 1]] == [
    ('call', body[load - 3].op_str), ('test', 'rax, rax')]
assert body[load - 1].mnemonic == 'je'
empty = body[load - 1].operands[0].imm
assert sum(op.mnemonic == 'je' and op.operands[0].imm == empty for op in body) == 3
null, = [op for op in body if op.address == empty]
assert (null.mnemonic, null.op_str) == ('xor', 'eax, eax')
print('PASS linked WorldObject instance getter retains live dungeon script '
      'lookup and returns null for missing map, dungeon parent or base-map '
      'destruction after a failed InstanceMap dynamic cast.')

"""Check scoped native Sanguine ending dispatch in the final x64 PE."""
from pathlib import Path
import runpy

from sharasdal_erunak_entry_pe import sites, values, fields

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
body = n['instructions']('?AffectUnit@AreaTrigger@@')
assert {8688, 226489, 226510, 226512, 3, 32} <= values(body)
assert {0x710, 0x712} <= fields(body)  # actual unload/remove receipts
assert sites(n, body, '?GetUInt32Value@Object@@')
dispatch, = sites(n, body, '?DoAction@AreaTrigger@@')
# An ending native action that also receives REMOVE(32) skips its earlier
# moment. All other actions and final REMOVE reach the ordinary DoAction.
moment, = [i for i, op in enumerate(body)
           if op.mnemonic == 'cmp' and op.op_str == 'ebp, 0x20']
assert body[moment + 1].mnemonic == 'jne'
assert body[moment + 1].operands[0].imm == body[dispatch + 1].address
assert moment < dispatch
for constant in (8688, 226489, 3, 226512):
    at, = [i for i, op in enumerate(body)
           if op.mnemonic == 'cmp' and constant in values([op])]
    assert body[at + 1].mnemonic == 'jne'
    assert body[at + 1].operands[0].imm == body[dispatch - 3].address
print('PASS linked Sanguine8688/226489 child226510/226512 remove-stack3 '
      'dispatch waits for final REMOVE32 only during unload/remove. Other '
      'actions and ordinary leave retain native DoAction. Native and client '
      'behavior checks remain separate.')

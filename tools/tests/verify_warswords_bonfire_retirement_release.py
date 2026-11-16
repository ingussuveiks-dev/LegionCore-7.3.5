"""Check consumed-bonfire interaction in the linked Release, including AI slots."""
from pathlib import Path
import hashlib
import runpy
import struct
from capstone import CS_OP_MEM

# Retain the completed-use dispatch, eligibility, exactly-once credit and fog
# checks, using the same actual PE/unwind reader and matching map timestamp.
p = runpy.run_path(str(Path(__file__).with_name('verify_warswords_bonfire_release.py')))
n, ins, sites, vals = (p[k] for k in ('n', 'ins', 'sites', 'vals'))
suffix = '@go_mystic_bonfireAI@go_mystic_bonfire@@'
sync = '?SyncInteraction' + suffix
body = ins(sync)
assert {245188, 1511, 134117, 944} <= vals(body), 'Retirement scope changed'
step, = sites(body, '?GetCurrentStep@Scenario@@')
assert sites(body, '?GetScenario@ScenarioMgr@@')[0] < step
assert body[step + 1].mnemonic == 'test' and body[step + 2].mnemonic == 'je'
reset_address = body[step + 2].operands[0].imm
has, = sites(body, '?HasFlag@Object@@')
set_flag, = sites(body, '?SetFlag@Object@@')
remove, = sites(body, '?RemoveFlag@Object@@')
ready, = sites(body, '?SetGoState@GameObject@@')
assert step < has < set_flag < remove < ready
assert body[remove].address >= reset_address > body[set_flag].address
for at in (has, set_flag, remove):
    assert body[at-2].op_str == 'edx, 0x11'
    assert body[at-1].op_str == 'r8d, 0x10', 'Wrong native NOT_SELECTABLE flag'
assert body[ready-1].op_str == 'edx, 1', 'Reset must restore authored READY'
claim = next(x for x in body[set_flag+1:remove] if x.mnemonic == 'mov'
             and x.operands[0].type == CS_OP_MEM and x.operands[0].size == 1)
assert claim.op_str.endswith(', 1')
owned = next(i for i, x in enumerate(body) if x.address == reset_address)
assert body[owned].mnemonic == 'cmp' and body[owned+1].mnemonic == 'je'
assert body[owned].operands[0].mem.disp == claim.operands[0].mem.disp
clear = body[ready+1]
assert clear.mnemonic == 'mov' and clear.op_str.endswith(', 0')
assert clear.operands[0].mem.disp == claim.operands[0].mem.disp
for forbidden in ('?AddItem@Player@@', '?UpdateAchievementCriteria@Player@@',
                  '?CompleteQuest@Player@@', '?TeleportTo@Player@@'):
    targets = {n['resolve'](a) for name, a in n['symbols'].items() if name.startswith(forbidden)}
    assert not targets & n['calls'](sync), 'Synchronization grants progression'
hello = ins('?GossipHello' + suffix)
ack, = sites(hello, '?UpdateAchievementCriteria@Player@@')
retire, = sites(hello, sync)
assert ack < retire
assert sites(hello[ack+1:retire], '?GetCurrentStep@Scenario@@'), 'Retirement lost progression acknowledgement'
# MSVC folds the three identical bodies. Check both actual virtual slots in
# the factory-selected table, rather than merely finding unused symbols.
table, = p['tables']
for slot, method in ((1, '?UpdateAI'), (4, '?Reset')):
    target = struct.unpack_from('<Q', n['binary'], table + slot*8)[0]
    assert n['resolve'](target) == n['symbol'](method + suffix) == n['symbol'](sync)
print('PASS linked bonfire retirement: scoped post-ack NOT_SELECTABLE, owned-only READY reset; factory UpdateAI/Reset synchronize late loads; no progression grants; SHA256=' + hashlib.sha256(n['binary']).hexdigest().upper())

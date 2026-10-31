"""Verify actual rejected-login routing, native inventory proof and exit pose."""
from pathlib import Path
import runpy
import struct

from sharasdal_erunak_entry_pe import sites, values
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
ins = n['instructions']
body = ins('?ExitCompletedSheilunLogin@Player@@')
assert {1541, 10, 1007, 41003, 102479, 128937, 5710, 1514} <= values(body)
for target in ('?IsQuestRewarded@Player@@', '?GetQuestStatus@Player@@', '?GetQuestObjectiveData@Player@@',
               '?GetScenario@ScenarioMgr@@', '?GetMap@Scenario@@', '?GetScenarioId@Scenario@@',
               '?GetItemByEntry@Player@@', '?GetOwnerGUID@Item@@', '??9ObjectGuid@@', '?GetCount@Item@@',
               '?IsValidMapCoord@MapManager@@', '?TeleportTo@Player@@'):
    assert sites(n, body, target), target
inventory, = sites(n, body, '?GetItemByEntry@Player@@')
owner, = sites(n, body, '?GetOwnerGUID@Item@@')
compare, = sites(n, body, '??9ObjectGuid@@')
count, = sites(n, body, '?GetCount@Item@@')
coord, = sites(n, body, '?IsValidMapCoord@MapManager@@')
teleport, = sites(n, body, '?TeleportTo@Player@@')
assert inventory < owner < compare < count < coord < teleport
assert body[inventory - 3].op_str == 'r8d, r8d', 'GetItemByEntry must exclude bank'
failure = body[teleport + 2].operands[0].imm
assert body[teleport + 1].op_str == 'al, al' and body[teleport + 2].mnemonic == 'je'
for at, branch in ((compare, 'jne'), (count, 'je'), (coord, 'je')):
    assert body[at + 2].mnemonic == branch and body[at + 2].operands[0].imm == failure
assert any(x.mnemonic == 'mov' and x.op_str == 'rbx, qword ptr [rax + 0xb270]' for x in body[count:coord]), 'Actual native5710 table row'
assert any(x.mnemonic == 'cmp' and x.op_str == 'word ptr [rbx + 0x1c], cx' for x in body[count:coord]), 'Actual destination MapID'
multiplier, = [x for x in body[count:coord] if x.mnemonic == 'mulss']
operand = multiplier.operands[1]
assert operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP
factor, = struct.unpack_from('<f', n['binary'], n['offset'](multiplier.address + multiplier.size + operand.mem.disp))
assert abs(factor - 0.017453292519943295) < 1e-9, 'Native degree-to-radian conversion'
assert any(x.op_str == 'qword ptr [rdi + 0x6bd8], rsi' for x in body[teleport + 3:]), 'Clear login IDs only after accepted transfer'
assert any(x.op_str == 'word ptr [rdi + 0x6ba8], si' for x in body[teleport + 3:]), 'Clear consumed selector'
for forbidden in ('?AddItem@Player@@', '?KilledMonsterCredit@Player@@', '?UpdateAchievementCriteria@Player@@',
                  '?CompleteQuest@Player@@', '?SetQuestObjectiveData@Player@@', '?DestroyItem@Player@@'):
    assert not sites(n, body, forbidden), forbidden

callers = []
for name in n['symbols']:
    if 'HandlePlayerLogin@WorldSession@@' not in name:
        continue
    try:
        caller = ins(name)
    except (ValueError, IndexError):
        continue
    calls = sites(n, caller, '?ExitCompletedSheilunLogin@Player@@')
    if calls:
        callers.append((caller, calls))
(caller, calls), = callers
recovery, = calls
dark, = sites(n, caller, '?ExitCompletedDarkRidersLogin@Player@@')
shore, = sites(n, caller, '?ExitCompletedBrokenShoreLogin@Player@@')
admission, = sites(n, caller, '?CheckInstanceLoginValid@Player@@')
assert admission < dark < shore < recovery
for at in (admission, dark, shore, recovery):
    assert caller[at + 1].op_str == 'al, al' and caller[at + 2].mnemonic == 'jne'
    assert caller[at + 2].operands[0].imm == caller[recovery + 2].operands[0].imm, 'Preserve accepted login and existing recoveries'
assert caller[recovery + 2].operands[0].imm > caller[recovery + 3].address, 'Successful recovery skips native error/fallback'
print('PASS linked PE: actual delayed post-AddPlayer login admission, preserved live/Dark Riders/Broken Shore branches, owned inventory128937 plus unique102479 objective, scenario/checkpoint guards, native5710/1514 pose and degree conversion, checkpoint cleanup only after accepted transfer; no item/progression grant. Native pose/collision and client checks separate.')

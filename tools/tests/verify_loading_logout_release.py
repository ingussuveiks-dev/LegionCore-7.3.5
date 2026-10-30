"""Verify logout removal bypasses only the loading gate in the actual x64 PE."""
import argparse
import bisect
from pathlib import Path
import runpy
import sys
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM

parser = argparse.ArgumentParser()
parser.add_argument('release', type=Path)
parser.add_argument('--expect-baseline', action='store_true')
args = parser.parse_args()
sys.argv = [sys.argv[0], str(args.release)]
n = runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
cs = Cs(CS_ARCH_X86, CS_MODE_64)
cs.detail = True


def instructions(prefix):
    address = n['symbol'](prefix)
    index = bisect.bisect_right(n['starts'], address - n['base']) - 1
    begin, end, unwind = n['functions'][index]
    assert begin <= address - n['base'] < end
    origin = n['root'](begin, unwind)
    for tail_begin, tail_end, tail_unwind in n['functions'][index + 1:]:
        if tail_begin != end or n['root'](tail_begin, tail_unwind) != origin:
            break
        end = tail_end
    at = n['offset'](address)
    return list(cs.disasm(n['binary'][at:at + n['base'] + end - address], address))


def byte_write(entry, value):
    return (entry.mnemonic == 'mov' and entry.operands[0].type == CS_OP_MEM
            and entry.operands[0].size == 1 and entry.operands[1].type == CS_OP_IMM
            and entry.operands[1].imm == value)


def immediate_call(entry, target):
    return (entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and n['resolve'](entry.operands[0].imm) == target)


request = instructions('?HandleLogoutRequest@WorldSession@@')
mapping = instructions('?Update@Map@@')
session = instructions('?Update@WorldSession@@')
# Derive the remove byte from the real accepted instant request and both its
# native map consumer/reset and delayed scheduler. No hard-coded Player ABI.
offsets = {i.operands[0].mem.disp for i in request if byte_write(i, 1)}
offsets &= {i.operands[0].mem.disp for i in mapping if byte_write(i, 0)}
offsets &= {i.operands[0].mem.disp for i in session if byte_write(i, 1)}
assert len(offsets) == 1, 'Native accepted/scheduled/remove/reset field changed'
remove_offset = offsets.pop()
remove_tests = [i for i in mapping if i.mnemonic == 'cmp' and i.operands[0].type == CS_OP_MEM
                and i.operands[0].size == 1 and i.operands[0].mem.disp == remove_offset]
assert len(remove_tests) == 1, 'Missing native map removal test'
remove_test = remove_tests[0]
by_address = {i.address: k for k, i in enumerate(mapping)}
empty = n['symbol']('?IsEmpty@ObjectGuid@@')
loading_calls = [k for k, i in enumerate(mapping) if immediate_call(i, empty)
                 and k and mapping[k - 1].mnemonic == 'lea'
                 and mapping[k - 1].operands[1].type == CS_OP_MEM]
assert len(loading_calls) == 1, 'Reinspect native map loading-receipt check'
index = loading_calls[0]
loading_offset = mapping[index - 1].operands[1].mem.disp
branch = mapping[index + 2]
assert mapping[index + 1].mnemonic == 'test' and branch.mnemonic == 'je'
repaired_map = branch.operands[0].imm == remove_test.address
if args.expect_baseline:
    assert not repaired_map, 'Expected old PE counterfactual is already repaired'
    print('COUNTERFACTUAL actual old PE: pending loading receipt skips native map removal; pair with repaired PE/native regressions.')
    raise SystemExit(0)
assert repaired_map, 'Loading branch does not reach native pending-logout removal'
tail = mapping[by_address[remove_test.address]:by_address[remove_test.address] + 10]
assert any(i.mnemonic == 'call' and i.operands[0].type == CS_OP_MEM for i in tail), 'Missing native virtual map-removal dispatch'
assert any(byte_write(i, 0) and i.operands[0].mem.disp == remove_offset for i in tail), 'Removal request not consumed'
ready = [i.operands[0].mem.disp for i in tail if byte_write(i, 1)]
assert len(ready) == 1, 'Missing native map-detached logout handoff'
assert n['symbol']('?LogoutPlayer@WorldSession@@') in n['calls']('?Update@WorldSession@@'), 'Scheduler no longer calls native logout'
# The delayed20s branch must accept a real Player even with a nonempty GUID;
# its no-Player arm must still defer an unfinished login.
sites = [k for k, i in enumerate(session) if immediate_call(i, empty) and k
         and session[k - 1].mnemonic == 'lea' and session[k - 1].operands[1].type == CS_OP_MEM
         and session[k - 1].operands[1].mem.disp == loading_offset]
assert len(sites) == 1, 'Reinspect native delayed logout loading check'
index = sites[0]
branch = session[index + 2]
assert session[index + 1].mnemonic == 'test' and branch.mnemonic == 'jne', 'Pending loading still blocks delayed logout'
assert session[index + 3].mnemonic == 'cmp' and session[index + 3].operands[0].type == CS_OP_MEM
assert session[index + 4].mnemonic == 'je', 'Uncreated pending login is no longer protected'
pending = session[index + 5:index + 9]
assert any(byte_write(i, 1) and i.operands[0].mem.disp == remove_offset for i in pending), 'Existing pending Player does not request map detach'
preceding = session[index - 10:index]
assert any(i.mnemonic == 'add' and any(op.type == CS_OP_IMM and op.imm == 20 for op in i.operands) for i in preceding), 'Native20s logout delay changed'
print('PASS actual Release: accepted/scheduled remove byte reaches native map detach despite loading receipt; request consumed,ready handoff/native LogoutPlayer retained; delayed20s existing-Player branch works and uncreated login remains protected.')

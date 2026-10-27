"""Inspect the linked candidate mask and its unchanged native predicate path."""
from pathlib import Path
import runpy
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM

n = runpy.run_path(str(Path(__file__).with_name('verify_scenario_lfg_release.py')))
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
decoder.detail = True
method = '?GetSearcherTypeMaskForCondition@Condition@@'
start = n['symbol'](method)
end = min(n['resolve'](a) for a in n['symbols'].values() if n['resolve'](a) > start)
at = n['offset'](start)
body = list(decoder.disasm(n['binary'][at:at + end - start], start))
# Condition has twelve 32-bit fields followed by target/negative bytes at 0x30/31.
assert any(x.mnemonic == 'cmp' and x.op_str == 'byte ptr [rcx + 0x30], 0' for x in body[:12]), 'Missing actual ConditionTarget guard'
assert any(x.mnemonic == 'cmp' and x.op_str == 'byte ptr [rcx + 0x31], 0' for x in body[:12]), 'Missing existing negative-condition guard'
branches = [x.operands[0].imm for x in body[:6] if x.mnemonic == 'jne' and x.operands[0].type == CS_OP_IMM]
assert len(branches) == 2 and branches[0] == branches[1], 'Target/negative guards must share all-container return'
tail = next(i for i, x in enumerate(body) if x.address == branches[0])
assert body[tail].mnemonic == 'mov' and body[tail].op_str == 'eax, 0xff', 'Noncandidate/negative conditions do not retain all containers'
for caller, callee in (
    ('?GetSearcherTypeMaskForConditionList@ConditionMgr@@', method),
    ('?GetSearcherTypeMask@Spell@@', '?GetSearcherTypeMaskForConditionList@ConditionMgr@@'),
    ('?SearchNearbyTarget@Spell@@', '?GetSearcherTypeMask@Spell@@'),
    ('?Meets@Condition@@', '?HasItemOrGemWithIdEquipped@Player@@'),
    ('?Meets@Condition@@', '?GetCurrentStep@Scenario@@'),
):
    assert n['symbol'](callee) in n['calls'](caller), (caller, callee)
print('PASS linked condition target-index/negative guards and native mask/search/equipped/scenario predicate consumers; native regression checks exact masks, references, alternative groups and actual22 Execute conditions')

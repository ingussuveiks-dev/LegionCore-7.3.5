"""Verify scoped Titanstrike stage admission in the actual linked Release."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_talonclaw_pickup_admission_release.py')))
native = previous['native']
instructions = previous['instructions']
body = instructions('?Use@ArtifactAcquisition@@')


def compares(entry, value):
    return entry.mnemonic == 'cmp' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_IMM and entry.operands[1].imm == value


first = next(index for index, entry in enumerate(body) if compares(entry, 249812))
last = next(index for index in range(first + 1, len(body)) if compares(body[index], 248834))
guard = body[first:last]
assert guard[1].mnemonic == 'jne' and guard[1].operands[0].imm == body[last].address, 'Titanstrike gate leaked to other pickup entries'
rejected = {}
for current, following in zip(guard, guard[1:]):
    expected = next((branch for value, branch in [(1609, 'jne'), (1099, 'jne'), (4, 'jb'), (5, 'ja')] if compares(current, value)), None)
    if expected:
        assert following.mnemonic == expected and following.operands[0].type == CS_OP_IMM, 'Wrong map/scenario/stage branch'
        rejected[current.operands[1].imm] = following.operands[0].imm
assert rejected.keys() == {1609, 1099, 4, 5} and len(set(rejected.values())) == 1, 'Stage4/5 must share the map/scenario rejection path'
failure = next(index for index, entry in enumerate(body) if entry.address == rejected[4])
assert body[failure].mnemonic == 'mov' and body[failure].op_str == 'al, 1', 'Invalid context must consume opening before native loot/replay'
assert any(entry.mnemonic == 'test' and entry.op_str == 'rax, rax' and guard[index + 1].mnemonic == 'je' and guard[index + 1].operands[0].imm == rejected[4] for index, entry in enumerate(guard[:-1])), 'Missing-scenario lookup is not rejected'
for prefix in ['?GetScenario@ScenarioMgr@@', '?GetScenarioId@Scenario@@', '?GetCurrentStep@Scenario@@']:
    assert native['symbol'](prefix) in native['calls']('?Use@ArtifactAcquisition@@'), f'Missing native scenario lookup: {prefix}'
print('PASS: canonical Release scopes249812 to map1609/scenario1099/exact4-5, rejects missing context before ordinary loot or owned replay and leaves other pickup branches separate')

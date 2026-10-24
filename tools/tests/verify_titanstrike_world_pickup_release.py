"""Verify the actual Titanstrike completed-loot visual-retirement implementation."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_AC_READ

previous = runpy.run_path(str(Path(__file__).with_name('verify_talonclaw_world_pickup_release.py')))
native = previous['native']
instructions = previous['instructions']
retire = '?Retire@TitanstrikeWorldPickup@@'
body = instructions(retire)
values = {operand.imm for instruction in body for operand in instruction.operands if operand.type == CS_OP_IMM}
assert {249812, 1609, 42185, 128861, 1099, 103154, 106745, 4, 5, 1000}.issubset(values), 'Missing scoped Titanstrike retirement constants'
assert previous['accesses'](retire, CS_AC_READ), 'Titanstrike retirement must read actual chest provenance'
targets = native['calls'](retire)
for prefix in ('?GetCreatureListWithEntryInGrid@WorldObject@@', '?DespawnOrUnsummon@Creature@@',
               '?GetScenario@ScenarioMgr@@', '?GetCurrentStep@Scenario@@'):
    assert native['symbol'](prefix) in targets, f'Missing native Titanstrike dependency: {prefix}'
binding = '?OnLootStateChanged@go_artifact_acquisition@@'
assert native['symbol'](retire) in native['calls'](binding), 'Completed-loot script does not dispatch Titanstrike retirement'
for stage, branch in ((4, 'jb'), (5, 'ja')):
    assert any(instruction.mnemonic == 'cmp' and len(instruction.operands) == 2 and
               instruction.operands[1].type == CS_OP_IMM and instruction.operands[1].imm == stage and
               body[index + 1].mnemonic == branch for index, instruction in enumerate(body[:-1])), 'Missing exact native stage bound'
print('PASS: deployed completed-loot script retains Talonclaw and dispatches scoped Titanstrike249812/1609/1099/4-5 retirement; actual source GUID and native delayed despawn are linked')

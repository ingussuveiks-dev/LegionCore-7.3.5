"""Verify native stage admission and SmartAI delegation in the canonical Release."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM

previous = runpy.run_path(str(Path(__file__).with_name('verify_talonclaw_world_pickup_release.py')))
native = previous['native']
instructions = previous['instructions']
gate = '?Update@Gate@TalonclawDakarr@@'
values = {operand.imm for instruction in instructions(gate) for operand in instruction.operands if operand.type == CS_OP_IMM}
assert {1511, 973, 11, 2}.issubset(values), 'Wrong final-lair or protection-flag admission'
assert native['symbol']('?CombatStop@Unit@@') in native['calls'](gate)
assert native['symbol']('?SetReactState@Creature@@') in native['calls'](gate)
for method in ('InitializeAI', 'Reset', 'UpdateAI', 'AttackStart', 'DamageTaken'):
    wrapper = f'?{method}@npc_talonclaw_dakarr@@'
    targets = native['calls'](wrapper)
    assert native['symbol'](gate) in targets, f'{method} misses live stage admission'
    assert native['symbol'](f'?{method}@SmartAI@@') in targets, f'{method} loses native SmartAI behavior'
assert any(instruction.mnemonic == 'mov' and len(instruction.operands) == 2 and
           instruction.operands[0].type == CS_OP_MEM and instruction.operands[0].size == 4 and
           instruction.operands[1].type == CS_OP_IMM and instruction.operands[1].imm == 0
           for instruction in instructions('?DamageTaken@npc_talonclaw_dakarr@@')), 'Blocked damage is not zeroed'
assert b'npc_talonclaw_dakarr' in native['binary']
print('PASS: actual Release guards Dakarr1511/973/11, passive/nonattackable cleanup and every native SmartAI delegate without replacing death credit/spawn actions')

recover = '?RecoverDeathCredit@TalonclawDakarr@@'
eligible = '?Eligible@TalonclawDakarr@@'
values = {operand.imm for instruction in instructions(recover) for operand in instruction.operands if operand.type == CS_OP_IMM}
assert {1511, 973, 11, 12, 47688}.issubset(values), 'Wrong missed death-event recovery scope'
targets = native['calls'](recover)
for prefix in (eligible, '?IsObjectMeetingSmartEventConditions@ConditionMgr@@', '?GetLootRecipient@Creature@@', '?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@', '?UpdateAchievementCriteria@Player@@'):
    assert native['symbol'](prefix) in targets, f'Missing native death-credit path: {prefix}'
assert 40385 in {operand.imm for instruction in instructions(eligible) for operand in instruction.operands if operand.type == CS_OP_IMM}
assert native['symbol']('?IsWithinDistInMap@WorldObject@@') in native['calls'](eligible), 'Recovery loses native spatial admission'
death_calls = [operand.imm for instruction in instructions('?JustDied@npc_talonclaw_dakarr@@')
               if instruction.mnemonic in {'call', 'jmp'} for operand in instruction.operands if operand.type == CS_OP_IMM]
assert death_calls.index(native['symbol']('?JustDied@SmartAI@@')) < death_calls.index(native['symbol'](recover)), 'Recovery must follow the unchanged native death actions'
print('PASS: actual Release invokes native SmartAI death before scoped47688 recovery, retaining native loot/owner/spatial/player-criteria bridges')

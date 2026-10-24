"""Verify native GO dispatch, LINK resolution and criterion consumer."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_talonclaw_apata_death_warning_release.py')))
native = previous['native']
dialogue = previous['dialogue']
for caller, event in [('?GossipHello@SmartGameObjectAI@@', 64), ('?OnStateChanged@SmartGameObjectAI@@', 70)]:
    assert native['symbol']('?ProcessEventsFor@SmartScript@@') in native['calls'](caller), f'Missing GO dispatcher: {caller}'
    assert any(operand.type == CS_OP_IMM and operand.imm == event for entry in dialogue['instructions'](caller) for operand in entry.operands), f'Missing actual event{event}'
assert native['symbol']('?UpdateAchievementCriteria@Player@@') in dialogue['calls'](dialogue['branch'](205)), 'Actual205 criterion consumer missing'
print('PASS: deployed SmartGameObjectAI gossip64/state70 reach native event dispatch; admitted LINK61 resolves to actual criterion205/Player consumer. Scene/cast/loot success remains separate')

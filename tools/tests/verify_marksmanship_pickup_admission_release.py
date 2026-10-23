"""Check actual Thas'dorah opening and preserved native condition bridges."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_marksmanship_presentation_release.py')))
native = previous['native']
instructions = previous['instructions']
pickup = '?Use@ArtifactAcquisition@@'
values = {operand.imm for instruction in instructions(pickup)
          for operand in instruction.operands if operand.type == CS_OP_IMM}
assert {248419, 1489, 972, 7, 8}.issubset(values), 'Missing actual final/completed Thasdorah opening gate'
assert native['symbol']('?IsObjectMeetingSmartEventConditions@ConditionMgr@@') in native['calls']('?ProcessEventsFor@SmartScript@@'), 'Native replay bypasses SmartAI condition admission'
assert native['symbol']('?IsObjectMeetToConditions@ConditionMgr@@QEAA_NPEAVWorldObject@@AEBV') in native['calls']('?AllowedForPlayer@LootItem@@'), 'Real loot omits native per-item conditions'
assert native['symbol']('?LootItemInSlot@Loot@@') in native['calls']('?StoreLootItem@Player@@'), 'Cached receipt bypasses native slot validation'
assert native['symbol']('?GetConditionsForSmartEvent@ConditionMgr@@') in native['calls']('?ProcessEvent@SmartScript@@'), 'Post-claim native event loses condition dispatch'
print('PASS: actual Release guards Thasdorah972/7-8 opening and retains native gossip/loot/slot/post-claim condition dispatch')

"""Verify Talonclaw opening admission in the actual canonical executable."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_talonclaw_dakarr_release.py')))
native = previous['native']
instructions = previous['instructions']
values = {operand.imm for instruction in instructions('?Use@ArtifactAcquisition@@')
          for operand in instruction.operands if operand.type == CS_OP_IMM}
assert {248834, 1511, 973, 12}.issubset(values), 'Linked pickup handler lacks final-stage Talonclaw admission'
assert native['symbol']('?IsObjectMeetToConditions@ConditionMgr@@QEAA_NPEAVWorldObject@@AEBV') in native['calls']('?AllowedForPlayer@LootItem@@'), 'Native per-item conditions are not executed'
assert native['symbol']('?LootItemInSlot@Loot@@') in native['calls']('?StoreLootItem@Player@@'), 'Native inventory receipt bypasses slot validation'
print('PASS: actual Release pickup handler contains Talonclaw1511/973/final-stage admission; native item-condition and store-slot bridges remain linked')

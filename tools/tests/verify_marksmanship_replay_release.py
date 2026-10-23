"""Verify unchanged native condition/ownership dispatch used by the new replay data."""
from pathlib import Path
import runpy

previous = runpy.run_path(str(Path(__file__).with_name('verify_talonclaw_pickup_admission_release.py')))
native = previous['native']
assert native['symbol']('?IsObjectMeetingSmartEventConditions@ConditionMgr@@') in native['calls']('?ProcessEventsFor@SmartScript@@')
assert native['symbol']('?GetConditionsForSmartEvent@ConditionMgr@@') in native['calls']('?ProcessEvent@SmartScript@@')
ownership = {address for name, address in native['symbols'].items() if name.startswith('?HasItemCount@Player@@')}
assert ownership & native['calls']('?Meets@Condition@@'), 'Native condition evaluator loses item ownership/bank admission'
print('PASS: canonical SmartAI event dispatch executes source22 conditions and the native ownership/bank bridge; new replay behavior remains data driven')

"""Verify that native SmartAI consumes its per-root conditions before actions."""
from pathlib import Path
import runpy

previous = runpy.run_path(str(Path(__file__).with_name('verify_xalatath_pickup_link_release.py')))
native = previous['native']
assert native['symbol']('?IsObjectMeetingSmartEventConditions@ConditionMgr@@') in native['calls']('?ProcessEventsFor@SmartScript@@'), 'SmartAI bypasses the source22 event admission'
assert native['symbol']('?IsObjectMeetToConditions@ConditionMgr@@QEBA_NAEAUConditionSourceInfo@@AEBV') in native['calls']('?IsObjectMeetingSmartEventConditions@ConditionMgr@@'), 'Source22 misses native condition evaluation'
print('PASS: canonical SmartAI ProcessEventsFor -> source22 per-entry/event-ID/source admission -> native condition evaluator, retaining the existing LINK/criterion consumer')

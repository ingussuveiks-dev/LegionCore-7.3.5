"""Check the deployed native waypoint action beneath Inquisitor admission."""
from pathlib import Path
import runpy

previous = runpy.run_path(str(Path(__file__).with_name('verify_marksmanship_herald_release.py')))
native = previous['native']
for caller, callee in [
    ('?ProcessAction@SmartScript@@', '?StartPath@SmartAI@@'),
    ('?StartPath@SmartAI@@', '?isInCombat@Unit@@'),
    ('?StartPath@SmartAI@@', '?GetPath@SmartWaypointMgr@@'),
    ('?StartPath@SmartAI@@', '?SetPathId@SmartScript@@'),
    ('?StartPath@SmartAI@@', '?ProcessEventsFor@SmartScript@@'),
]:
    assert native['symbol'](callee) in native['calls'](caller), f'Missing deployed waypoint bridge {caller} -> {callee}'
print('PASS: deployed WP_START reaches native SmartAI combat admission, inlined LoadPath lookup/path-ID write and waypoint-start dispatch; condition-only repair does not certify movement, later conversations or combat/path retry')

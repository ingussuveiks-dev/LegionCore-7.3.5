"""Verify the existing native owner selector and Halak presentation services."""
from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_smart_invoker_player_release.py')))
n=p['native']
calls=n['calls']('?ProcessAction@SmartScript@@')
for prefix in ('?GetTargets@SmartScript@@','?CreateConversation@Conversation@@','?SendChat@CreatureTextMgr@@'):
    assert n['symbol'](prefix) in calls,('Missing native presentation service',prefix)
print('PASS actual canonical target130 loader/owner/map/phase/range and native206 conversation creation/TALK SendChat services; SQL row identity and native host event dispatch checked separately. No new core conversation behavior or client rendering claim.')

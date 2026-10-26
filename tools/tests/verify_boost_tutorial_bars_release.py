"""Check actual boost update invokes post-loading once-per-arrival overlay."""
from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n=p['native']
script='@instance_boost_experience_InstanceScript@instance_boost_experience@@'
refresh='?RefreshTutorialActionBar'+script
assert n['symbol'](refresh) in n['calls']('?Update'+script)
for method in ('?IsMapObjectArrivalProtected@Player@@','?SendActionButtons@Player@@','?GetScenario@ScenarioMgr@@'):
    assert n['symbol'](method) in n['calls'](refresh), method
for forbidden in ('?ArrangeBoostActionBar@CharacterService@@','?RemoveActionButton@Player@@','?AddActionButton@Player@@'):
    assert n['symbol'](forbidden) not in n['calls'](refresh), 'Arrival overlay must not rearrange saved buttons'
assert b'Boost tutorial action overlay refreshed after arrival on map %u at step %u' in n['binary']
print('PASS selected boost update reaches post-loading/arrival guard and native action packet without altering saved layouts; actual fresh-client initialization retest pending.')

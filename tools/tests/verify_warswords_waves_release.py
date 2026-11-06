"""Select the linked instance wave controller and native acknowledgements."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM
n=runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
a,ins=n['native'],n['instructions']
update='?Update@State@WarswordsWaves@@'
for prefix in ('?OnUnitDeath@instance_warswords_InstanceMapScript', '?onScenarionNextStep@instance_warswords_InstanceMapScript', '?Update@instance_warswords_InstanceMapScript'):
    assert a['symbol'](update) in a['calls'](prefix), 'Missing actual instance-owned wave hook: '+prefix
calls=a['calls'](update)
for prefix in ('?Counter@State@WarswordsWaves@@','?Progress@State@WarswordsWaves@@','?IsParticipant@WarswordsArrival@@','?SummonCreature@WorldObject@@','?UpdateAchievementCriteria@Player@@','?GetCreature@Map@@'):
    assert any(name.startswith(prefix) and a['resolve'](address) in calls for name,address in a['symbols'].items()), 'Missing native wave dependency: '+prefix
values={v.imm for x in ins(update) for v in x.operands if v.type==CS_OP_IMM}
assert {47289,46804,29100,29055,97992,97994,98069,97649,1000} <= values
death=ins('?OnUnitDeath@instance_warswords_InstanceMapScript')
assert a['symbol']('?GetGuidValue@Object@@') in a['calls']('?OnUnitDeath@instance_warswords_InstanceMapScript')
for forbidden in ('?AddItem@Player@@','?RewardQuest@Player@@','?CompleteQuest@Player@@','?KilledMonsterCredit@Player@@'):
    assert not any(name.startswith(forbidden) and a['resolve'](address) in calls for name,address in a['symbols'].items())
print('PASS linked Warswords instance stage/death/update select shared full-GUID wave controller; actual native wave/leader criteria, summon/retry and acknowledgement dependencies present; no quest/item grants. Source ASAN/native catalog are separate from client combat.')

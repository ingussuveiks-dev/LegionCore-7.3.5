"""Select native2009 request/ownership plus retained firstfight/unlock/route."""
from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_tuure_route_opening_release.py')))
n,values=p['n'],p['values']
scope='instance_tuure_InstanceMapScript@instance_tuure@@'
story='?CompleteVictoryStory@'+scope
for prefix in ('?CreateConversation@Conversation@@','?BindQuestObject@Player@@','?Eligible@TuureFirstFight@@','?HasVictory@TuureFirstFight@@'):
 addresses={n['resolve'](a) for s,a in n['symbols'].items() if s.startswith(prefix)}
 assert addresses and addresses&n['calls'](story),prefix
assert {2009,41993,1000,30,106137,361432}<=values(story)
for method in ('BeginUnlock','Update'):
 assert n['symbol'](story) in n['calls']('?'+method+'@'+scope)
for prefix in ('?FindNearestCreature@','?UpdateAchievementCriteria@','?CompleteQuest@','?MovePath@','?RemoveFlag@'):
 assert not {n['resolve'](a) for s,a in n['symbols'].items() if s.startswith(prefix)}&n['calls'](story),prefix
assert 16000 in values('?BeginUnlock@'+scope)
print('PASS selected checked2009 native publication/quest binding/captured roles;earned16000ms unlock and original kill/route dispatch retained;no client playback claim')

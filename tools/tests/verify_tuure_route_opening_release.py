"""Selected native2039 publication and the retained2007/kill/arrival chain."""
from pathlib import Path
import runpy
p=runpy.run_path(str(Path(__file__).with_name('verify_tuure_route_middle_release.py')))
n,values=p['n'],p['values']
scope='instance_tuure_InstanceMapScript@instance_tuure@@'
complete='?CompleteRouteTaunt@'+scope
for prefix in ('?CreateConversation@Conversation@@','?BindQuestObject@Player@@','?Eligible@TuureFirstFight@@'):
 addresses={n['resolve'](a) for s,a in n['symbols'].items() if s.startswith(prefix)}
 assert addresses and addresses&n['calls'](complete),prefix
assert {2039,41993,1000,30}<=values(complete)
assert n['symbol'](complete) in n['calls']('?UpdateRoute@'+scope)
for prefix in ('?FindNearestCreature@','?UpdateAchievementCriteria@','?CompleteQuest@','?MovePath@','?MoveFollow@','?SendChat@'):
 assert not {n['resolve'](a) for s,a in n['symbols'].items() if s.startswith(prefix)}&n['calls'](complete),prefix
print('PASS selected checked native2039/quest-bound ownership;no movement,Jace replay,nearest physical Lady or progress grants;retained2007/kill/arrival chain selected')

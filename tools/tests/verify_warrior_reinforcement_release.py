"""Inspect instance-owned real Bezzeredes death and acknowledged intro retries."""
from pathlib import Path
import runpy

p=runpy.run_path(str(Path(__file__).with_name('verify_malgalor_release.py')))
n,ins,needs,refs,table,values=(p[k] for k in ('n','ins','needs','refs','table','values'))
scope='instance_broken_shore_InstanceMapScript@instance_broken_shore@@'
death='?Death@State@WarriorReinforcement@@'
update='?Update@State@WarriorReinforcement@@'
for callback, callee in (('CreatureDies',death),('Update',update)):
    needs('?'+callback+'@'+scope,callee)
    assert any(n['symbol']('?'+callback+'@'+scope) in table(a) for a in refs('?GetInstanceScript@instance_broken_shore@@'))
assert {92608,361167}<=values('?OnCreatureCreate@'+scope)
assert {98257,361151,45773,3}<=values(death)
needs(death,'?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@','?AddQuestDelayedEvent@Player@@',update)
needs(update,'?Current@State@WarriorReinforcement@@','?Leaf@State@WarriorReinforcement@@',
      '?UpdateAchievementCriteria@Player@@','?Speak@WarriorReinforcement@@')
assert {46880,45773,45777,93301,8000,24000,2000,771,1414,197593}<=values(update)
needs('?Current@State@WarriorReinforcement@@','?Original@MalgalorFinale@@')
needs('?Progress@State@WarriorReinforcement@@','?GetScenario@ScenarioMgr@@')
assert {1500,933}<=values('?Progress@State@WarriorReinforcement@@')
needs('?Speak@WarriorReinforcement@@','?CreateConversation@Conversation@@')
assert n['symbol']('?CreateConversation@Unit@@') not in n['calls'](update), 'Void convenience helper cannot acknowledge acceptance'
marker='?GetData@npc_malgalorAI@npc_malgalor@@'
assert 93301 in values(marker)
assert any(n['symbol'](marker) in table(a) for a in refs('?GetAI@npc_malgalor@@'))
print('PASS linked instance factory death/create/update hooks, exact real361151 death, native46880 tree receipt, original quest lifetime, actual Malgalor AI status marker and accepted native conversation/scene retry path; Paladin/native finale retained. Client traversal pending.')

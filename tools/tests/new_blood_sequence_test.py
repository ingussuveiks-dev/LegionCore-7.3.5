"""Migration replay and native 26972 stage/negative-path checks (not a client test)."""
import json,sys
from pathlib import Path
sys.dont_write_bytecode=True
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'research'))
from audit_spell_mechanics import Data
rows=[json.loads(x) for x in Path(sys.argv[1]).read_text().splitlines()]
scripts={(r['entry'],r['source'],r['id']):r for r in rows if r['kind']=='script'}
conditions=[r for r in rows if r['kind']=='condition']
def script(e,i,src=0):return scripts[e,src,i]
def meets(src,group,entry,state,source_id=0):
 rules=[c for c in conditions if (c['source'],c['group'],c['entry'],c['sourceId'])==(src,group,entry,source_id)]
 assert rules,(src,group,entry)
 for branch in {c['branch'] for c in rules}:
  results=[]
  for c in rules:
   if c['branch']!=branch:continue
   k,v=c['type'],c['v1']
   if k==44: value=state['scenario']==v and state['step']==c['v2']
   elif k==22:value=state.get('map')==v
   elif k==9:value=state.get('quest')==v
   elif k==8:value=v in state.get('rewarded',[])
   elif k==28:value=v in state.get('complete',[])
   elif k==2:value=state.get('items',{}).get(v,0)>=c['v2']
   elif k==29:value=v in state.get('aliveNear',[])
   elif k==30:value=v in state.get('objectsNear',[])
   elif k==1:value=v in state.get('auras',[])
   elif k==52:value=v in state.get('criteriaTrees',[])
   elif k==31:value=state.get('type')==v and (not c['v2'] or state.get('entry')==c['v2'])
   else:raise AssertionError(c)
   results.append(not value if c['negative'] else value)
  if all(results):return True
 return False
base={'scenario':962,'step':0,'quest':40729,'map':1522,'objectsNear':[245924]}
grant=script(98771,7,9)
assert (grant['action'],grant['a1'],grant['a2'],grant['target'])==(11,199338,18,18)
recovery=script(98771,8)
assert (recovery['event'],recovery['flags'],recovery['action'],recovery['a1'],recovery['a2'],recovery['target'])==(10,0,11,199338,18,7)
prisoner=base|{'step':1}
assert meets(22,9,98771,prisoner)
for changed in [{'auras':[199338]},{'criteriaTrees':[46006]},{'step':0},{'step':2},{'scenario':998},{'quest':0}]:
 assert not meets(22,9,98771,prisoner|changed),changed
assert not meets(17,0,199336,prisoner|{'criteriaTrees':[46006]})
# Exactly the player and four surviving council actors get captors; pets,
# Zelifrax, other NPCs and players without the quest cannot summon one.
captives=[{'type':4,'quest':40729}]+[{'type':3,'entry':e} for e in [101456,101454,101455,101457]]
for captive in captives:
 assert meets(13,1,200038,captive) and meets(17,0,200032,captive)
 assert meets(22,3,101460,captive)==(captive['type']==4)
for bystander in [{'type':4},{'type':3,'entry':98797},{'type':3,'entry':101433},{'type':3,'entry':416}]:
 assert not meets(13,1,200038,bystander) and not meets(17,0,200032,bystander)
assert (script(101460,1)['action'],script(101460,1)['target'],script(101460,1)['link'])==(66,23,5)
assert (script(101460,5)['action'],script(101460,5)['a1'],script(101460,5)['target'],script(101460,5)['link'])==(11,200033,23,0)
assert script(101460,2)['event']==54 # linked events do not evaluate source-22 guards
assert script(101460,0,9)['ep1']==12000 and script(101460,0,9)['a1']==199362
assert script(101460,1,9)['ep1']==1000 and script(101460,1,9)['a1']==199359
assert next(r for r in rows if r['kind']=='instantCaptureLinks')['count']==0
waiting={'rewarded':[40716]}
assert meets(23,8392,200,waiting)
assert not meets(23,8392,200,waiting|{'quest':40729})
assert not meets(23,8392,200,waiting|{'complete':[40729]})
assert not meets(23,8392,200,{'rewarded':[40716,40729]})
exit_pos=next(r for r in rows if r['kind']=='exit')
assert exit_pos['map']==1502 and abs(exit_pos['z']-695.968)<0.01
assert (101460,9,8) not in scripts # no premature escape credit
assert script(99627,0)['a1']==92 and script(99627,0)['target']==7
for step in range(6):
 state=base|{'step':step}
 assert meets(17,0,199336,state)==(step==1)
 assert meets(22,6,98831,state)==(step==5)
 assert meets(22,6,99178,state)==(step==5)
 assert meets(22,1,99627,state)==(step==5)
for scenario in [998,1011,0]:
 state=base|{'scenario':scenario,'step':5}
 for e,g in [(98831,6),(99178,6),(99627,1)]:assert not meets(22,g,e,state)
assert not meets(22,6,99178,base|{'step':5,'objectsNear':[]})
for actor in [98775,98783,98793]:
 assert meets(18,actor,195039,base|{'step':2})
 assert not meets(18,actor,195039,base|{'step':2,'aliveNear':[98878]})
 assert not meets(18,actor,195039,base|{'step':3})
assert not meets(18,98775,195039,base|{'step':2,'aliveNear':[98876]})
assert meets(4,245924,132512,base|{'step':4})
assert not meets(4,245924,132512,base|{'step':4,'aliveNear':[99516]})
assert not meets(4,245924,132512,base|{'step':3})
assert not meets(22,26,99178,base|{'step':4})
assert meets(22,26,99178,base|{'step':4,'items':{132512:1}})
assert not meets(22,26,99178,base|{'step':4,'items':{132512:1},'aliveNear':[99516]})
assert next(r for r in rows if r['kind']=='book')['loot']==245924
assert (245924,1,0) not in scripts # opening loot cannot start the chase
assert script(99178,5)['event']==60 and script(99178,5)['phase']==1
assert script(9917801,4,9)['action']==22 and script(9917801,4,9)['a1']==1
assert script(245234,0,1)['flags']&1
assert script(245233,0,1)['action']==23 and script(245233,1,1)['phase']==2
# Apply the actual phase increment / linked phase mask to two distinct, once-only activators.
phase=0
for click in range(2):
 phase+=script(245233,0,1)['a1']
 opened=bool(script(245233,1,1)['phase']&(1<<(phase-1)))
 assert opened==(click==1)
assert script(98836,3)['link']==16 and script(98836,16)['t1']==245100
assert script(98834,3)['event']==6 and script(98834,3)['action']==45 and script(98834,3)['t1']==99627
assert script(98834,2)['target']==18 # player/pet killing blow cannot change recipients
assert script(99627,0)['phase']==1 and script(99627,2)['a1']==1
# Follow the actual exit link chain: credit, native scenario completion, cleanup, transfer.
chain=[];r=script(99627,0)
while True:
 chain.append((r['action'],r['a1']))
 if not r['link']:break
 r=script(99627,r['link']);assert len(chain)<15
assert chain[0]==(205,92) and chain[-1]==(85,196340)
assert set(a for kind,a in chain if kind==28)=={195045,195066,195068,199195,199338}
assert script(99627,0)['a2']==47404
assert script(99516,4)['action']==11 and script(99516,4)['a2']==18 and script(99516,4)['target']==18
assert script(9883101,4,9)['action']==45 and script(9883101,4,9)['t1']==98783 # DIE ignores target lists!
assert script(98783,7)['action']==11 and script(98783,7)['a1']==199175 and script(98783,7)['target']==8
assert script(98783,9)['ep1']==199048 and script(98783,9)['action']==37
assert script(9883101,7,9)['action']==45 and script(9883101,7,9)['a1']==3
assert script(99178,6)['action']==28 and script(99178,6)['a1']==33356
assert script(99178,26)['a2']==9917801 and script(99178,24)['ep1']==18
assert script(99178,24)['a1']==199215
# Native ScenarioStep/CriteriaTree/Criteria, no guessed replacement IDs.
d=Data(Path('build-extractors/bin/Release/dbc/enUS'),Path('__native_only__'))
steps=sorted((r for r in d.rows('ScenarioStep').values() if r['ScenarioID']==962),key=lambda r:r['OrderIndex'])
trees=d.rows('CriteriaTree'); criteria=d.rows('Criteria')
expected=[{(92,47152)},{(92,47179),(92,47333)},{(92,47181),(92,47182),(92,47183)},{(92,47234)},{(36,132512)},{(92,47404)}]
for step,wanted in zip(steps,expected):
 root=step['Criteriatreeid']; children=[r for r in trees.values() if r['Parent']==root]
 actual={(criteria[r['CriteriaID']]['Type'],criteria[r['CriteriaID']]['Asset']) for r in children}
 assert actual==wanted,(step,actual)
assert len(steps)==6
fx=d.rows('SpellEffect').values()
assert any(r['SpellID']==200038 and r['Effect']==140 and r['EffectTriggerSpell']==200032 for r in fx)
assert any(r['SpellID']==200032 and r['Effect']==28 and r['EffectMiscValue1']==101460 for r in fx)
assert any(r['SpellID']==200033 and r['EffectAura']==12 and r['ImplicitTarget2']==92 for r in fx)
assert any(r['SpellID']==200033 and r['SpellVisualID']==51979 for r in d.rows('SpellXSpellVisual').values())
assert d.rows('SpellDuration')[36]['Duration']==1000
assert any(r['SpellID']==199215 and r['Effect']==28 and r['EffectMiscValue1']==98834 for r in fx)
assert any(r['SpellID']==196340 and r['Effect']==134 and r['EffectMiscValue1']==101150 for r in fx)
assert any(r['SpellID']==199359 and r['Effect']==134 and r['EffectMiscValue1']==101420 for r in fx)
assert any(r['SpellID']==199338 and r['EffectAura']==293 and r['EffectMiscValue1']==643 and r['ImplicitTarget1']==1 for r in fx)
assert d.rows('OverrideSpellData')[643]['Spells1']==199336
enslave=criteria[trees[46006]['CriteriaID']]
assert (enslave['Type'],enslave['Asset'],trees[46006]['Parent'])==(92,47333,46005)
for spell in [199070,199071,199072,199073,199074,199079,199080]:
 assert any(r['SpellID']==spell and r['Effect']==219 for r in fx)
assert any(r['SpellID']==199150 and r['EffectTriggerSpell']==199154 for r in fx)
assert any(r['SpellID']==199156 and r['EffectTriggerSpell']==199166 for r in fx)
assert any(r['SpellID']==199166 and r['Effect']==213 for r in fx)
assert any(r['SpellID']==199175 and r['Effect']==213 and r['EffectTriggerSpell']==199048 for r in fx)
assert any(r['SpellID']==199162 and r['EffectTriggerSpell']==33356 for r in fx)
assert any(r['SpellID']==33356 and r['EffectAura']==26 for r in fx)
print('PASS: native six-stage criteria, cage/enslave, three rescues, two-crystal barrier, actual book loot, archive continuation, Xelthoth death gate, pet kills, exit cleanup and later-scenario isolation.')

import sys,json,re
from pathlib import Path
sys.dont_write_bytecode=True
root=Path(__file__).resolve().parents[2];sys.path.insert(0,str(root/'tools/research'))
from audit_spell_mechanics import Data
from audit_spell_visuals import Table
d=Data(root/'build-extractors/bin/Release/dbc/enUS',root/'.codex/stormheim-no-overlays')
def names(n):
 m=re.split('struct '+n+'Meta\n',d.meta)[1].split('\n};')[0]
 i=int(re.search(r'DB2Meta instance\((-?\d+)',m)[1]);t=Table(d.dbc/(n+'.db2'),i)
 result={t.ids[i]:t.name(i) for i in range(t.count)}
 for a,b in t.copy_pairs:result[a]=result[b]
 return result
sn=names('Scenario');sc={i:r for i,r in d.rows('Scenario').items() if i in (824,826)}
for i,r in sc.items():r['decodedName']=sn[i]
steps=[r for r in d.rows('ScenarioStep').values() if r['ScenarioID'] in sc]
trees=d.rows('CriteriaTree');ids={r['Criteriatreeid'] for r in steps}
while True:
 new=ids|{i for i,r in trees.items() if r['Parent'] in ids}
 if new==ids:break
 ids=new
ct={i:trees[i] for i in ids};criteria={r['CriteriaID']:d.rows('Criteria').get(r['CriteriaID']) for r in ct.values() if r['CriteriaID']}
lfgn=names('LFGDungeons');lfg={i:dict(r,decodedName=lfgn[i]) for i,r in d.rows('LFGDungeons').items() if i in (969,998)}
out={'scenarios':sc,'steps':steps,'trees':ct,'criteria':criteria,'lfg':lfg,'effects':[r for r in d.rows('SpellEffect').values() if r['SpellID']==228560 or r['Effect']==16 and r['EffectMiscValue1'] in(39800,39801)]}
for n,records,fields in [('ScenarioStep',steps,['Description','Title']),('CriteriaTree',list(ct.values()),['Description'])]:
 t=Table(d.dbc/(n+'.db2'));offset=t.strings
 for r in records:
  for f in fields:
   p=offset+r[f];r['decoded'+f]=t.data[p:t.data.index(0,p)].decode()
assets={r['Asset'] for r in criteria.values()}
out['event_spells']=[r for r in d.rows('SpellEffect').values() if r['Effect']==61 and r['EffectMiscValue1'] in assets]
spelln=names('Spell')
for r in out['event_spells']:r['decodedName']=spelln[r['SpellID']]
assert len(steps)==11 and set(sc)=={824,826}
assert [(r['OrderIndex'],r['RewardQuestID']) for r in steps if r['ScenarioID']==826]==[(0,0),(1,0),(2,0),(3,0),(4,39800)]
assert [(r['OrderIndex'],r['RewardQuestID']) for r in steps if r['ScenarioID']==824]==[(0,0),(1,0),(2,0),(3,0),(4,0),(5,39801)]
assert [(i,r['MapID'],r['DifficultyID'],r['ScenarioID']) for i,r in lfg.items()]==[(969,1475,12,826),(998,1475,12,824)]
assert criteria[28073]['Type']==92 and criteria[28073]['Asset']==45340 and ct[43802]['Amount']==1
assert len(out['effects'])==1 and out['effects'][0]['Effect']==45 and out['effects'][0]['EffectMiscValue1']==483
out['movie']=d.rows('Movie')[483]
out['map']=d.rows('Map')[1475]
out['runtime_hashes']=d.hashes
out['scope']='Native reference only. Quest/scenario stages after the opening Horde report are not implemented or client-verified by this entry repair.'
evidence=root/'docs/audits/stormheim-departure-native-2026-10-13.json'
if '--write-evidence' in sys.argv:evidence.write_text(json.dumps(out,indent=2)+'\n')
else:assert json.loads(json.dumps(out))==json.loads(evidence.read_text())
print('PASS: installed 7.3.5 native map, movie, faction LFG scenarios, eleven ordered stages, exact criteria/events and final RewardQuestID mapping; no QUEST_COMPLETE shortcut spell.')
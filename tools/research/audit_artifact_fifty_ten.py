"""Review unique artifact selection/quest/objective records in ten-item cohorts.

Structural evidence only: no progression simulation, world startup or client
claim. A reviewed record is not a repaired mechanic or a complete quest.
"""
import argparse,collections,hashlib,json,sys
from pathlib import Path
sys.dont_write_bytecode=True
from audit_spell_mechanics import Data

def main():
 p=argparse.ArgumentParser(description=__doc__)
 for name in ('plan','world','items','creature-wdb','output'):p.add_argument('--'+name,type=Path,required=True)
 a=p.parse_args();plan=json.loads(a.plan.read_text());cases=plan['cases']
 if len(cases)!=500 or len({(c['kind'],c['key']) for c in cases})!=500:
  raise RuntimeError('Require500 unique reviewed records;never pad duplicates')
 hashes={'plan':hashlib.sha256(a.plan.read_bytes()).hexdigest(),'items':hashlib.sha256(a.items.read_bytes()).hexdigest()}
 def rows(n):
  path=a.world/(n+'.jsonl');hashes[n]=hashlib.sha256(path.read_bytes()).hexdigest()
  return [json.loads(x) for x in path.read_text(encoding='utf-8-sig').splitlines()]
 quests={q['ID']:q for q in rows('quest_template')}
 for q in rows('quest_template_addon'):
  if q['ID'] in quests:quests[q['ID']].update(q)
 objectives={o['ID']:o for o in rows('quest_objectives')}
 creatures={c['entry'] for c in rows('creature_template')}
 # ObjectMgr loads WDB definitions into the store before optional server
 # template overrides. Credit-only actors legitimately have only a WDB row.
 hashes['creature_template_wdb']=hashlib.sha256(a.creature_wdb.read_bytes()).hexdigest()
 for line in a.creature_wdb.read_text(encoding='utf-8-sig').splitlines():
  if line!='EOF':creatures.add(json.loads(line)['entry'])
 objects={g['entry'] for g in rows('gameobject_template')}
 responses=collections.Counter((r['ChoiceId'],r['ResponseId']) for r in rows('playerchoice_response'))
 rewards=collections.defaultdict(list)
 for r in rows('playerchoice_response_reward'):rewards[(r['ChoiceId'],r['ResponseId'])].append(r)
 native_items={}
 for line in a.items.read_text(encoding='utf-8-sig').splitlines():
  fields=line.split('\t')
  if fields[0]=='ITEM':native_items[int(fields[1])]=dict(zip(('exists','artifact','class','spec'),map(int,fields[2:])))
 native=Data(Path('build-extractors/bin/Release/dbc/enUS'),Path('__native_only__'))
 trees=native.rows('CriteriaTree')
 effects=collections.defaultdict(list)
 for e in native.rows('SpellEffect').values():effects[e['SpellID']].append(e)
 choices={c['item']:c['route'] for c in cases if c['kind']=='choice'}
 results=[]
 for c in cases:
  checks=[];issues=[];notes=[]
  def check(rule,ok,detail=''):
   checks.append(rule)
   if not ok:issues.append({'rule':rule,'detail':detail})
  if c['kind']=='choice':
   r=c['route'];key=(r['choice'],r['response']);reward=rewards[key]
   check('unique_native_response',responses[key]==1,str(key))
   check('unique_reward',len(reward)==1,str(key))
   spell=reward[0]['SpellID'] if len(reward)==1 else 0
   check('native_chosen_quest_effect16',any(e['Effect']==16 and e['EffectMiscValue1']==r['chosen'] for e in effects[spell]),str((spell,r['chosen'])))
   item=native_items.get(c['item'],{})
   check('native_sparse_item_and_specialization_class',bool(item.get('exists') and item.get('artifact') and item.get('spec') and item.get('class')==r['class_id']),str(c['item']))
   check('chosen_and_start_quests_present',r['chosen'] in quests and r['start'] in quests,str((r['chosen'],r['start'])))
  elif c['kind']=='quest':
   q=quests.get(c['key']);check('quest_row_present',q is not None,str(c['key']))
   if q:
    for field in ('PrevQuestID','NextQuestID','RewardNextQuest'):
     value=q.get(field,0);check('existing_'+field,not value or abs(value) in quests,str(value))
    mask=q.get('AllowableClasses',0)
    classes={choices[item]['class_id'] for item in c['items']}
    for cls in classes:
     allowed=not mask or (mask>0 and bool(mask&(1<<(cls-1)))) or (mask<0 and not bool(-mask&(1<<(cls-1))))
     check('class_admission_'+str(cls),allowed,str(mask))
    check('nonempty_native_race_admission',bool(q.get('AllowableRaces',0)),str(q.get('AllowableRaces')))
  elif c['kind']=='objective':
   o=objectives.get(c['key']);check('objective_present',o is not None,str(c['key']))
   if o:
    check('existing_parent_quest',o['QuestID'] in quests,str(o['QuestID']))
    kind=o['Type'];target=o['ObjectID']
    check('native_objective_type',0<=kind<=17,str(kind))
    if kind in (0,1,2,3,9,10):check('native_nonnegative_counter_storage',o['StorageIndex']>=0,str(o['StorageIndex']))
    if kind==0:check('native_creature_target',target in creatures,str(target))
    elif kind==1:check('native_sparse_item_target',bool(native_items.get(target,{}).get('exists')),str(target))
    elif kind==2:check('native_gameobject_target',target in objects,str(target))
    elif kind==3:check('talk_actor_template',target in creatures,str(target))
    elif kind==14:check('native_criteria_tree',target in trees,str(target))
    if kind in (0,1,2,3):
     # Native HasQuestObjectiveComplete compares counter>=Amount and the
     # loader permits zero. This establishes engine legality only, not the
     # author's intended objective count; do not invent a mandatory counter.
     check('nonnegative_objective_amount',o['Amount']>=0,str(o['Amount']))
     if not o['Amount']:notes.append('Zero is accepted by the engine;authored intent and gameplay remain unverified.')
  else:raise RuntimeError('Unknown review unit')
  results.append({'sequence':c['sequence'],'batch':c['batch'],'kind':c['kind'],'key':c['key'],'artifact_items':c.get('items',[c.get('item')]),'checks':checks,'status':'REVIEW' if issues else 'PASS','issues':issues,'notes':notes})
 if collections.Counter(r['batch'] for r in results)!=collections.Counter({i:10 for i in range(1,51)}):raise RuntimeError('Cohort count differs')
 a.output.mkdir(parents=True,exist_ok=True)
 for batch in range(1,51):
  group=[r for r in results if r['batch']==batch]
  (a.output/f'batch{batch:02}.json').write_text(json.dumps({'batch':batch,'unit_count':10,'limits':plan['units'],'records':group},indent=2)+'\n')
 summary={'date':'2026-11-17','count':len(results),'batches':50,'unit_types':dict(collections.Counter(r['kind'] for r in results)),'statuses':dict(collections.Counter(r['status'] for r in results)),'new_fixes':0,'hashes':hashes,'native_hashes':native.hashes,'scope_exclusion':plan['exclusion'],'limits':plan['units'],'review_findings':[r for r in results if r['issues']]}
 (a.output/'summary.json').write_text(json.dumps(summary,indent=2)+'\n')
 print('REVIEW500 records/50 cohorts',summary['unit_types'],summary['statuses'],'new fixes=0;not full gameplay')
 for r in summary['review_findings']:print(json.dumps(r))

if __name__=='__main__':main()

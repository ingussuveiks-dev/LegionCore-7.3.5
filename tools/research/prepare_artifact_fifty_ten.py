"""Choose500 unique scoped records,with explicit active-artifact exclusions.

Choices and every unique scoped quest come first. Typed objectives are then
selected round-robin across artifacts,prioritizing specialized target types.
This plans structural reviews,not500 repairs or scripted gameplay cases.
"""
import argparse,collections,json
from pathlib import Path

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--routes',type=Path,required=True)
 p.add_argument('--output',type=Path,required=True)
 p.add_argument('--exclude-item',type=int,action='append',default=[])
 a=p.parse_args();report=json.loads(a.routes.read_text(encoding='utf-8-sig'))
 routes=[s for s in report['routes'] if s['item'] not in a.exclude_item]
 quests={};provenance=collections.defaultdict(set)
 for s in routes:
  for q in s['quest_details']:quests[q['id']]=q;provenance[q['id']].add(s['item'])
 cases=[{'kind':'choice','key':s['response'],'item':s['item'],'route':s} for s in routes]
 cases += [{'kind':'quest','key':qid,'quest':q,'items':sorted(provenance[qid])} for qid,q in sorted(quests.items())]
 queue={s['item']:sorted({o['ID']:o for q in s['quest_details'] for o in q['objectives']}.values(),key=lambda o:(o['Type']==0,o['ID'])) for s in routes}
 used=set()
 if len(cases)>500:raise RuntimeError('Mandatory scope exceeds500;choose an explicit larger review instead of dropping quests')
 while len(cases)<500:
  progress=False
  for s in routes:
   q=queue[s['item']]
   while q and q[0]['ID'] in used:q.pop(0)
   if not q:continue
   o=q.pop(0);used.add(o['ID']);cases.append({'kind':'objective','key':o['ID'],'objective':o,'items':sorted(provenance[o['QuestID']])});progress=True
   if len(cases)==500:break
  if not progress:raise RuntimeError('Fewer than500 unique scoped records;never pad duplicate checks')
 for i,c in enumerate(cases,1):c.update(sequence=i,batch=(i-1)//10+1)
 items=sorted({s['item'] for s in routes}|{c['objective']['ObjectID'] for c in cases if c['kind']=='objective' and c['objective']['Type']==1})
 a.output.mkdir(parents=True,exist_ok=True)
 (a.output/'native-item-queries.txt').write_text('\n'.join(map(str,items))+'\n')
 (a.output/'case-plan.json').write_text(json.dumps({'count':500,'batches':50,'exclusion':'Artifact items excluded from review: '+','.join(map(str,a.exclude_item)),'units':'One unique native selection,World quest row or typed objective record per item;multiple invariants in a record do not add to count. Structural review,not500 fixes/client paths.','cases':cases},ensure_ascii=False,indent=2)+'\n')
 print('PLANNED',dict(collections.Counter(c['kind'] for c in cases)),'native sparse queries',len(items),'excluded',a.exclude_item)

if __name__=='__main__':main()

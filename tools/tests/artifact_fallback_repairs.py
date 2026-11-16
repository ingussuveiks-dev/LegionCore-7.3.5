"""Artifact-only fallback data; existing Broadcast/action/quest wiring is preserved."""
from pathlib import Path
import argparse,json,re,hashlib
from gameobject_name_encoding import fingerprint
ROOT=Path(__file__).resolve().parents[2]
PLAN=ROOT/'tools/tests/ArtifactTenTen20261116FallbackRepairs.json'
TABLES=('creature_text','gossip_menu_option')
def load():return json.loads(PLAN.read_text(encoding='utf-8'))
def filename(n):return f'2026_11_16_06_artifact_fallback_batch{n:02}.sql'
def key(x,a='g',group=False):return ' AND '.join(f'{a}.`{k}`={v}' for k,v in x['key'].items() if not(group and k=='ID'))
def fp(x,a='g'):return fingerprint([c for c in x['values'] if c!=x['field']],a)
def matches(x,states=('before_hex','after_hex')):return '('+key(x)+f" AND HEX(g.`{x['field']}`) IN("+','.join("'"+x[s]+"'" for s in states)+f") AND {fp(x)}='{x['other_fingerprint']}')"
def validate(p):
 rows=p['repairs'];assert len(rows)==100 and len({(x['table'],tuple(x['key'].values()),x['field']) for x in rows})==100
 assert sum(x['table']=='creature_text' for x in rows)==80 and sum(x['table']=='gossip_menu_option' for x in rows)==20
 assert all(x['table']==('creature_text' if x['batch']<=8 else 'gossip_menu_option') for x in rows)
 prior={(x['key']['CreatureID'],x['key']['GroupID'],x['key']['ID']) for f in p['previous_plans'] for x in json.loads((ROOT/f).read_text())['repairs'] if x['table']=='creature_text'}
 for x in rows:
  old=x['values'][x['field']];new=x['after_text'];assert old!=new and old.encode().hex().upper()==x['before_hex'] and new.encode().hex().upper()==x['after_hex']
  tok=lambda s:re.findall(r'\$[A-Za-z]|%[0-9.$+\-]*[sduf]|\|[0-9A-Za-z]+',s)
  assert tok(old)==tok(new) and old.isascii() and new.isascii()
  assert not any(ord(c)<32 and c not in '\r\n' for c in new)
  if x['table']=='creature_text':
   assert x['field']=='Text' and tuple(x['key'].values()) not in prior and int(x['shape']['group_rows'])==1
   assert x['key']['CreatureID'] not in (96468,96433,94813,96740)
   assert x['provenance']['static_artifact_maps'] or x['provenance']['direct_quest_actor']
  else:assert x['table']=='gossip_menu_option' and x['field']=='OptionText' and int(x['shape']['key_rows'])==1 and x['provenance']
  assert x['broadcast'] and len(x['other_fingerprint'])==64
 for n in range(1,11):assert sum(x['batch']==n for x in rows)==10
def migration(rows,n):
 guards=[]
 for table in TABLES:
  cohort=[x for x in rows if x['table']==table]
  if not cohort:continue
  pred=' OR '.join(matches(x) for x in cohort)
  if table=='creature_text':
   scope=' OR '.join('('+key(x,group=True)+')' for x in cohort)
   guards.append(f"(SELECT COUNT(*) FROM (SELECT g.CreatureID,g.GroupID,COUNT(*) AS group_rows,SUM(CASE WHEN {pred} THEN 1 ELSE 0 END) AS valid_rows FROM creature_text g WHERE {scope} GROUP BY g.CreatureID,g.GroupID) text_shape WHERE group_rows=1 AND valid_rows=1)={len(cohort)}")
  else:guards.append(f'(SELECT COUNT(*) FROM {table} g WHERE {pred})={len(cohort)}')
 lines=[f'-- Artifact batch{n}: ten stored fallback texts aligned to their existing native26972 Broadcast records.', '-- Only fallback text changes; Broadcast, actions, conditions, voices, timers and progression stay intact.', 'START TRANSACTION;','SET @fallback_safe=('+' AND '.join(guards)+');',f"SET @fallback_guard=IF(COALESCE(@fallback_safe,0),'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Artifact fallback batch{n} differs; review before updating''');",'PREPARE fallback_guard FROM @fallback_guard;EXECUTE fallback_guard;DEALLOCATE PREPARE fallback_guard;']
 lines += [f"UPDATE `{x['table']}` g SET `{x['field']}`=CONVERT(0x{x['after_hex']} USING utf8mb4) WHERE @fallback_safe AND {matches(x,('before_hex',))};" for x in rows]
 return '\n'.join(lines+['COMMIT;'])+'\n'
def setup(p):
 allrows=p['repairs']+[x for f in p['previous_plans'] for x in json.loads((ROOT/f).read_text())['repairs'] if x['table']=='creature_text'];parts=['SET SESSION TRANSACTION READ ONLY;SET SESSION group_concat_max_len=10000000;']
 for t in TABLES:
  xs=[x for x in allrows if x['table']==t];scope=' OR '.join('('+key(x,group=t=='creature_text')+')' for x in xs)
  if t=='creature_text':scope+=' OR g.CreatureID IN(101430,101899)'
  else:scope='g.MenuID IN('+','.join(map(str,sorted({x['key']['MenuID'] for x in xs})))+')'
  # MariaDB requires READ WRITE for TEMP DDL, but permits TEMP DML in
  # READ ONLY. Restore READ ONLY before every INSERT/migration so even a
  # forced-client bootstrap failure cannot write a persistent table.
  parts.append(f'SET SESSION TRANSACTION READ WRITE;CREATE TEMPORARY TABLE seed_{t} LIKE {t};SET SESSION TRANSACTION READ ONLY;INSERT INTO seed_{t} SELECT g.* FROM {t} g WHERE {scope};SET SESSION TRANSACTION READ WRITE;CREATE TEMPORARY TABLE {t} LIKE seed_{t};SET SESSION TRANSACTION READ ONLY;INSERT INTO {t} SELECT * FROM seed_{t};')
 return ''.join(parts)
def snapshot(p,rows,tag,normalize=False):
 parts=[]
 for t in TABLES:
  sample=next(x for x in p['repairs'] if x['table']==t);expr=[]
  for c in sample['values']:
   v=f'g.`{c}`';mutable=[x for x in rows if x['table']==t and x['field']==c]
   if normalize and mutable:v='CASE '+' '.join(f"WHEN {key(x)} AND HEX(g.`{c}`)='{x['after_hex']}' THEN CONVERT(0x{x['before_hex']} USING utf8mb4)" for x in mutable)+f' ELSE g.`{c}` END'
   expr.append(f"COALESCE(HEX(CAST({v} AS CHAR CHARACTER SET utf8mb4)),'NULL')")
  h="SHA2(CONCAT_WS('|',"+','.join(expr)+'),256)';parts.append(f"SELECT '{tag}','{t}',COUNT(*),SHA2(GROUP_CONCAT(h ORDER BY h SEPARATOR ','),256) FROM (SELECT {h} h FROM {t} g) signatures;")
 return ''.join(parts)
def counts(rows,state,tag):
 terms=['(SELECT COUNT(*) FROM '+t+' g WHERE '+' OR '.join(matches(x,(state,)) for x in rows if x['table']==t)+')' for t in TABLES if any(x['table']==t for x in rows)]
 return f"SELECT '{tag}',"+'+'.join(terms)+';'
def prepare(p,sql,out):
 cases=[]
 for n in range(1,11):
  rows=[x for x in p['repairs'] if x['batch']==n];mig=(sql/filename(n)).read_text();x=rows[0]
  reset=''.join(f"UPDATE {r['table']} g SET `{r['field']}`=CONVERT(0x{r['before_hex']} USING utf8mb4) WHERE {matches(r,('after_hex',))};" for r in rows);base=setup(p)+mig+reset
  s=base+counts(rows,'before_hex','baseline')+snapshot(p,rows,'before',True)+mig+counts(rows,'after_hex','fixed')+snapshot(p,rows,'after',True)+mig+counts(rows,'after_hex','replay')+snapshot(p,rows,'last',True)+"SELECT 'EOF';"
  path=out/f'batch{n:02}-positive.sql';path.write_text(s,encoding='utf-8');cases.append(dict(path=str(path),refusal=False,batch=n))
  t=x['table'];field=x['field'];meta='Emote' if t=='creature_text' else 'BoxMoney'
  variants={'foreign-text':f"UPDATE {t} g SET `{field}`='Foreign fallback text' WHERE {key(x)};",'metadata':f"UPDATE {t} g SET `{meta}`=`{meta}`+1 WHERE {key(x)};",'missing':f"DELETE g FROM {t} g WHERE {key(x)};"}
  if t=='creature_text':variants['extra-group']=f"INSERT INTO creature_text SELECT CreatureID,GroupID,ID+1,'Foreign extra variant',Type,Language,Probability,Emote,Duration,Sound,BroadcastTextID,MinTimer,MaxTimer,SpellID,comment FROM creature_text g WHERE {key(x)};"
  for name,change in variants.items():
   path=out/f'batch{n:02}-{name}.sql';path.write_text(base+change+snapshot(p,rows,'before')+mig+snapshot(p,rows,'after')+"SELECT 'EOF';",encoding='utf-8');cases.append(dict(path=str(path),refusal=True,batch=n))
 (out/'cases.json').write_text(json.dumps(cases,indent=2))
 live=['SET SESSION TRANSACTION READ ONLY;']
 for x in p['repairs']:
  cols=','.join("'%s',g.`%s`"%(c,c) for c in x['values']);live.append(f"SELECT 'row',{x['seq']},JSON_OBJECT({cols}) FROM {x['table']} g WHERE {key(x)} AND HEX(g.`{x['field']}`)='{x['after_hex']}';")
 for n in range(1,11):live.append(f"SELECT 'updater','{filename(n)}',hash,state FROM updates WHERE name='{filename(n)}';")
 (out/'live.sql').write_text(''.join(live)+"SELECT 'EOF';",encoding='utf-8')
def verify(p,sql,out,live):
 cases=json.loads((out/'cases.json').read_text());refusals=0
 for case in cases:
  lines=Path(case['path']).with_suffix('.tsv').read_text(encoding='utf-8-sig').splitlines();assert lines[-1]=='EOF',case;a=[s.split('\t') for s in lines[:-1]]
  if case['refusal']:assert len(a)==4 and [r[1:] for r in a[:2]]==[r[1:] for r in a[2:]],case;refusals+=1
  else:assert len(a)==9 and [a[i] for i in (0,3,6)]==[['baseline','10'],['fixed','10'],['replay','10']] and [r[1:] for r in a[1:3]]==[r[1:] for r in a[4:6]]==[r[1:] for r in a[7:9]],case
 if live:
  lines=(out/'live.tsv').read_text(encoding='utf-8-sig').splitlines();assert len(lines)==111 and lines[-1]=='EOF';seen=set()
  for line in lines[:-1]:
   kind,id_,payload=line.split('\t',2);assert (kind,id_) not in seen;seen.add((kind,id_))
   if kind=='row':x=p['repairs'][int(id_)-1];v=dict(x['values']);v[x['field']]=x['after_text'];assert json.loads(payload)==v,id_
   else:assert kind=='updater' and payload==hashlib.sha1((sql/id_).read_text().encode()).hexdigest().upper()+'\tRELEASED',id_
 print(f'PASS100 artifact-only fields/10x10:10 installs/replays,{refusals} exact drift refusals,full-row control preservation;live={live}')
def main():
 a=argparse.ArgumentParser();a.add_argument('mode',choices=['generate','check','prepare','verify']);a.add_argument('--sql-directory',required=True);a.add_argument('--output');a.add_argument('--live',action='store_true');a=a.parse_args();p=load();validate(p);sql=Path(a.sql_directory);sql.mkdir(parents=True,exist_ok=True)
 if a.mode in ('generate','check'):
  for n in range(1,11):
   s=migration([x for x in p['repairs'] if x['batch']==n],n);f=sql/filename(n)
   if a.mode=='generate':f.write_text(s,encoding='utf-8',newline='\n')
   else:assert f.read_text()==s,f
  print('PASS100 artifact-only targets:80 NPC/20 menu fallback fields,unchanged formatting tokens,no prior147 overlap')
 else:
  out=Path(a.output);out.mkdir(parents=True,exist_ok=True)
  if a.mode=='prepare':prepare(p,sql,out)
  else:verify(p,sql,out,a.live)
if __name__=='__main__':main()

"""Generate independent full-schema TEMP regressions and native fixture inputs."""
from pathlib import Path
import argparse,json,hashlib
from ten_thirty_text_repairs import load,key,fp,matches,filename,ROOT
from gameobject_name_encoding import fingerprint
TABLES=('gameobject_template','creature_text')
def schema(plan,table):return list(next(x for x in plan['repairs'] if x['table']==table)['values'])
def prior(plan):return [x for p in plan['previous_plans'] for x in json.loads((ROOT/p).read_text(encoding='utf-8'))['repairs']]
def setup(plan):
 go=[x['key']['entry'] for x in plan['repairs'] if x['table']==TABLES[0]]+[x['entry'] for x in prior(plan)]+[239692]
 ct=['('+key(x,group=True)+')' for x in plan['repairs'] if x['table']==TABLES[1]]
 scopes={TABLES[0]:'g.entry IN('+','.join(map(str,go))+')',TABLES[1]:'('+' OR '.join(ct)+') OR g.CreatureID IN(101430,101899)'}
 out=['SET SESSION group_concat_max_len=10000000;']
 for t in TABLES:out.append(f'CREATE TEMPORARY TABLE seed_{t} LIKE {t};INSERT INTO seed_{t} SELECT g.* FROM {t} g WHERE {scopes[t]};CREATE TEMPORARY TABLE {t} LIKE seed_{t};INSERT INTO {t} SELECT * FROM seed_{t};')
 return '\n'.join(out+['SET SESSION TRANSACTION READ ONLY;'])
def reset(rows):return '\n'.join(f"UPDATE {x['table']} g SET `{x['field']}`=CONVERT(0x{x['before_hex']} USING utf8mb4) WHERE @text_safe AND {matches(x,('after_hex',))};" for x in rows)
def count(rows,stage):
 parts=[f"(SELECT COUNT(*) FROM {t} g WHERE "+' OR '.join(matches(x,(stage,)) for x in rows if x['table']==t)+')' for t in TABLES if any(x['table']==t for x in rows)]
 return '('+'+'.join(parts)+')'
def snapshot(plan,rows,tag,normalize):
 result=[]
 for t in TABLES:
  cols=schema(plan,t);changes=[x for x in rows if x['table']==t];expr=[]
  for c in cols:
   value=f'g.`{c}`'
   mutable=[x for x in changes if x['field']==c]
   if normalize and mutable:value='CASE '+' '.join(f"WHEN {key(x)} AND HEX(g.`{c}`)='{x['after_hex']}' THEN CONVERT(0x{x['before_hex']} USING utf8mb4)" for x in mutable)+f' ELSE g.`{c}` END'
   expr.append(f"COALESCE(HEX(CAST({value} AS CHAR CHARACTER SET utf8mb4)),'NULL')")
  h="SHA2(CONCAT_WS('|',"+','.join(expr)+'),256)'
  result.append(f"SELECT '{tag}','{t}',COUNT(*),SHA2(GROUP_CONCAT(h ORDER BY h SEPARATOR ','),256) FROM (SELECT {h} AS h FROM {t} g) text_signatures;")
 return '\n'.join(result)
def prepare_data(plan,sql_dir,out):
 cases=[]
 for batch in range(1,11):
  rows=[x for x in plan['repairs'] if x['batch']==batch];mig=(sql_dir/filename(batch)).read_text(encoding='utf-8');base=setup(plan)+mig+reset(rows);first=rows[0]
  positive=base+f"SELECT 'baseline',{count(rows,'before_hex')};"+snapshot(plan,rows,'before',True)+mig+f"SELECT 'fixed',{count(rows,'after_hex')};"+snapshot(plan,rows,'after',True)+mig+f"SELECT 'replay',{count(rows,'after_hex')};"+snapshot(plan,rows,'last',True)+"SELECT 'EOF';"
  p=out/f'batch{batch:02}-positive.sql';p.write_text(positive,encoding='utf-8');cases.append(dict(path=str(p),batch=batch,variant='positive',refusal=False))
  variants={'foreign-text':f"UPDATE {first['table']} g SET `{first['field']}`='__foreign_text__' WHERE {key(first)};",'metadata':f"UPDATE {first['table']} g SET `{('flags' if first['table']==TABLES[0] else 'Duration')}`=`{('flags' if first['table']==TABLES[0] else 'Duration')}`+1 WHERE {key(first)};",'missing':f"DELETE g FROM {first['table']} g WHERE {key(first)};"}
  ct=next((x for x in rows if x['table']==TABLES[1]),None)
  if ct:
   cols=schema(plan,TABLES[1]);values=','.join("'__extra_foreign_group_row__'" if c=='Text' else 'g.`'+c+'`' for c in cols)
   variants['extra-group']=f"INSERT INTO creature_text SELECT {values} FROM seed_creature_text g WHERE {key(ct)};"
   if first['table']!=TABLES[1]:variants['mixed-npc-metadata']=f"UPDATE creature_text g SET Duration=Duration+1 WHERE {key(ct)};"
  for variant,change in variants.items():
   p=out/f'batch{batch:02}-{variant}.sql';p.write_text(base+change+snapshot(plan,rows,'custom',False)+mig+snapshot(plan,rows,'after',False)+"SELECT 'EOF';",encoding='utf-8');cases.append(dict(path=str(p),batch=batch,variant=variant,refusal=True))
 (out/'cases.json').write_text(json.dumps(cases,indent=2)+'\n',encoding='utf-8')
 live=[]
 for x in plan['repairs']:
  fields=','.join(f"'{c}',g.`{c}`" for c in x['values']);live.append(f"SELECT 'row',{x['seq']},JSON_OBJECT({fields}) FROM {x['table']} g WHERE {key(x)} AND HEX(g.`{x['field']}`)='{x['after_hex']}';")
 for x in prior(plan):
  fields=','.join(f"'{c}',g.`{c}`" for c in schema(plan,TABLES[0]));live.append(f"SELECT 'control',{x['entry']},JSON_OBJECT({fields}) FROM gameobject_template g WHERE g.entry={x['entry']};")
 for batch in range(1,11):
  file=filename(batch);h=hashlib.sha1((sql_dir/file).read_text(encoding='utf-8').encode('utf-8')).hexdigest().upper();live.append(f"SELECT 'updater','{file}',hash,state FROM updates WHERE name='{file}';")
  (out/f'hash-{batch}.txt').write_text(h,encoding='ascii')
 (out/'live.sql').write_text('SET SESSION TRANSACTION READ ONLY;'+''.join(live)+"SELECT 'EOF';",encoding='utf-8')
def verify_data(plan,out,live):
 cases=json.loads((out/'cases.json').read_text());refusals=0
 for case in cases:
  p=Path(case['path']);lines=p.with_suffix('.tsv').read_text(encoding='utf-8-sig').splitlines();assert lines[-1]=='EOF',p
  rows=[s.split('\t') for s in lines[:-1]]
  if case['refusal']:
   assert len(rows)==4 and [r[1:] for r in rows[:2]]==[r[1:] for r in rows[2:]],p;refusals+=1
  else:
   assert len(rows)==9 and [rows[i] for i in (0,3,6)]==[['baseline','30'],['fixed','30'],['replay','30']],p
   a=[r[1:] for r in rows[1:3]];assert a==[r[1:] for r in rows[4:6]]==[r[1:] for r in rows[7:9]],p
   assert a[0][0]=='gameobject_template' and int(a[0][1])==554 and a[1][0]=='creature_text' and int(a[1][1])>147,p
 if live:
  lines=(out/'live.tsv').read_text(encoding='utf-8-sig').splitlines();assert lines[-1]=='EOF' and len(lines)==711
  seen=set();controls={x['entry']:x for x in prior(plan)};actuals={x['seq']:x for x in plan['repairs']}
  for line in lines[:-1]:
   kind,id_,payload=line.split('\t',2)
   if kind=='row':
    x=actuals[int(id_)];expected=dict(x['values']);expected[x['field']]=x['after_text'];assert json.loads(payload)==expected,(id_,'live delta differs')
   elif kind=='control':
    x=controls[int(id_)];expected={c:x[c] for c in schema(plan,TABLES[0])};expected['name']=x['after_name'];assert json.loads(payload)==expected,(id_,'control differs')
   else:
    assert kind=='updater';n=next(n for n in range(1,11) if filename(n)==id_);assert payload==(out/f'hash-{n}.txt').read_text()+'\tRELEASED',id_
   assert (kind,id_) not in seen;seen.add((kind,id_))
 (out/'result.json').write_text(json.dumps(dict(positive=10,refusals=refusals,live=live,new_fields=300,prior_controls=400,persistent_writes=False),indent=2)+'\n')
 print(f'PASS10x30 actual SQL installs/replays, full-row554 GO/selected NPC+legacy-control signatures, {refusals} exact drift refusals; live={live}; prior400 preserved')
def main():
 ap=argparse.ArgumentParser();ap.add_argument('mode',choices=['prepare','verify']);ap.add_argument('--sql-directory',required=True);ap.add_argument('--output',required=True);ap.add_argument('--live',action='store_true');args=ap.parse_args();plan=load();out=Path(args.output);out.mkdir(parents=True,exist_ok=True)
 if args.mode=='prepare':prepare_data(plan,Path(args.sql_directory),out)
 else:verify_data(plan,out,args.live)
if __name__=='__main__':main()

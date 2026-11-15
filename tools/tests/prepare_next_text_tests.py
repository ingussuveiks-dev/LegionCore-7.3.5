from pathlib import Path
import argparse,json,hashlib
from next_text_repairs import load,ROOT,key,match,filename
def snapshot(plan,tag,normalize=True):
 out=[]
 for t in sorted({r['table'] for r in plan['repairs']}):
  rs=[r for r in plan['repairs'] if r['table']==t];parts=[]
  for c in rs[0]['values']:
   v=f'g.`{c}`';sub=[r for r in rs if r['field']==c]
   if normalize and sub:v='CASE '+' '.join(f"WHEN {match(r,('after_hex',))} THEN CONVERT(0x{r['before_hex']} USING utf8mb4)" for r in sub)+f' ELSE g.`{c}` END'
   parts.append(f"COALESCE(HEX(CAST({v} AS CHAR CHARACTER SET utf8mb4)),'NULL')")
  fp="SHA2(CONCAT_WS('|',"+','.join(parts)+'),256)'
  out.append(f"SELECT '{tag}','{t}',COUNT(*),SHA2(GROUP_CONCAT(h ORDER BY h SEPARATOR ','),256) FROM (SELECT {fp} AS h FROM `{t}` g) sig;")
 return '\n'.join(out)
def setup(plan,migrations):
 out=['SET SESSION group_concat_max_len=10000000;']
 for t in sorted({r['table'] for r in plan['repairs']}):
  rs=[r for r in plan['repairs'] if r['table']==t]
  scope=' OR '.join('('+(' AND '.join(f'g.`{k}`={v}' for k,v in r['key'].items() if k!='ID') if t=='creature_text' else key(r))+')' for r in rs)
  out.append(f'CREATE TEMPORARY TABLE seed_{t} LIKE `{t}`;INSERT INTO seed_{t} SELECT g.* FROM `{t}` g WHERE {scope};CREATE TEMPORARY TABLE `{t}` LIKE seed_{t};INSERT INTO `{t}` SELECT * FROM seed_{t};')
 out+=['SET SESSION TRANSACTION READ ONLY;',migrations]
 for r in plan['repairs']:out.append(f"UPDATE `{r['table']}` g SET `{r['field']}`=CONVERT(0x{r['before_hex']} USING utf8mb4) WHERE {match(r,('after_hex',))};")
 return '\n'.join(out)
def prepare(p,sql_dir,out):
 migrations='\n'.join((sql_dir/filename(n)).read_text(encoding='utf-8') for n in range(1,10));base=setup(p,migrations);cases=[]
 def case(label,body,refusal=False):
  (out/f'{label}.sql').write_text(base+body+"SELECT 'EOF';",encoding='utf-8');cases.append(dict(label=label,refusal=refusal))
 counts=lambda state:'\n'.join(f"SELECT '{state}',{r['seq']},COUNT(*) FROM `{r['table']}` g WHERE {match(r,(state+'_hex',))};" for r in p['repairs'])
 case('positive',counts('before')+snapshot(p,'baseline')+migrations+counts('after')+snapshot(p,'fixed')+migrations+counts('after')+snapshot(p,'replay'))
 for t in sorted({r['table'] for r in p['repairs']}):
  r=next(r for r in p['repairs'] if r['table']==t);meta=next(c for c in r['values'] if c not in r['key'] and c!=r['field']);change=f"UPDATE `{t}` g SET `{meta}`="+('NULL' if r['values'][meta] is None else (str(r['values'][meta]+1) if isinstance(r['values'][meta],(int,float)) else "'__foreign_meta__'"))+f' WHERE {key(r)};'
  variants={'metadata':change,'foreign-text':f"UPDATE `{t}` g SET `{r['field']}`='__foreign_text__' WHERE {match(r,('before_hex',))};",'missing':f'DELETE g FROM `{t}` g WHERE {key(r)};'}
  if t=='creature_text':
   vals=','.join("'__foreign_group_row__'" if c=='Text' else f'g.`{c}`' for c in r['values']);variants['extra-group']=f'INSERT INTO creature_text SELECT {vals} FROM seed_creature_text g WHERE {key(r)} LIMIT 1;'
  for label,change in variants.items():case(t+'-'+label,change+snapshot(p,'custom',False)+(sql_dir/filename(r['batch'])).read_text(encoding='utf-8')+snapshot(p,'unchanged',False),True)
 (out/'cases.json').write_text(json.dumps(cases,indent=2),encoding='utf-8')
 live=['SET SESSION TRANSACTION READ ONLY;']
 for r in p['repairs']:live.append(f"SELECT 'live',{r['seq']},COUNT(*) FROM `{r['table']}` g WHERE {match(r,('after_hex',))};")
 for n in range(1,10):
  f=sql_dir/filename(n);live.append(f"SELECT 'update','{f.stem}',COUNT(*) FROM updates WHERE name='{f.name}' AND hash='{hashlib.sha1(f.read_bytes()).hexdigest().upper()}' AND state='RELEASED';")
 (out/'live.sql').write_text('\n'.join(live+["SELECT 'EOF';"]),encoding='utf-8')
def verify(p,out,live):
 for c in json.loads((out/'cases.json').read_text()):
  lines=[x.split('\t') for x in (out/(c['label']+'.tsv')).read_text(encoding='utf-8-sig').splitlines()];assert lines[-1]==['EOF']
  if c['refusal']:
   a=[x[1:] for x in lines if x[0]=='custom'];b=[x[1:] for x in lines if x[0]=='unchanged'];assert a==b
  else:
   assert len([x for x in lines if x[0] in ('before','after')])==261 and all(x[2]=='1' for x in lines if x[0] in ('before','after'))
   a=[x[1:] for x in lines if x[0]=='baseline'];assert a==[x[1:] for x in lines if x[0]=='fixed']==[x[1:] for x in lines if x[0]=='replay']
 if live:
  lines=(out/'live.tsv').read_text(encoding='utf-8-sig').splitlines();assert lines[-1]=='EOF' and len(lines)==97 and all(x.split('\t')[-1]=='1' for x in lines[:-1])
 print('PASS87 TEMP baseline/repair/replay, full row/group preservation,16 exact drift refusals'+('; live87/update9 verified' if live else ''))
def main():
 ap=argparse.ArgumentParser();ap.add_argument('mode',choices=('prepare','verify'));ap.add_argument('--output',type=Path,required=True);ap.add_argument('--sql-directory',type=Path,required=True);ap.add_argument('--live',action='store_true');a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=True)
 if a.mode=='prepare':prepare(load(),a.sql_directory,a.output)
 else:verify(load(),a.output,a.live)
if __name__=='__main__':main()

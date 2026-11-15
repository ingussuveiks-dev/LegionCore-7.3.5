"""Connection-local TEMP regressions; full rows and source rows stay invariant."""
from pathlib import Path
import argparse,json,hashlib
from layered_ten_ten import load,filename,pred,field
from gameobject_name_encoding import fingerprint

def batches(p):return range(1,max(r['batch'] for r in p['repairs'])+1)
def setup(p,kind,migrations):
 ids=','.join(str(r['entry']) for r in p['repairs']);s=['SET SESSION group_concat_max_len=10000000;']
 tables=[('gameobject_template',f'g.entry IN({ids},239692)')]
 if kind=='loot':tables.append(('gameobject_loot_template',f'g.Entry IN({ids})'))
 for t,scope in tables:s.append(f'CREATE TEMPORARY TABLE seed_{t} LIKE {t};INSERT INTO seed_{t} SELECT g.* FROM {t} g WHERE {scope};CREATE TEMPORARY TABLE {t} LIKE seed_{t};INSERT INTO {t} SELECT * FROM seed_{t};')
 s+=['SET SESSION TRANSACTION READ ONLY;',migrations]
 for r in p['repairs']:
  value=f"CONVERT(0x{r['before_hex']} USING utf8mb4)" if kind=='names' else '0'
  s.append(f'UPDATE gameobject_template g SET `{field(kind)}`={value} WHERE {pred(r,kind)};')
 return '\n'.join(s)

def snapshot(p,kind,tag,normalize=True):
 out=[];rs=p['repairs'];ids=','.join(str(r['entry']) for r in rs)
 tables=[('gameobject_template',list(rs[0]['values']))]
 if kind=='loot':tables.append(('gameobject_loot_template',p['loot_columns']))
 for t,cols in tables:
  parts=[]
  for c in cols:
   v=f'g.`{c}`'
   if normalize and t=='gameobject_template' and c==field(kind):
    if kind=='loot':v=f'CASE WHEN g.entry IN({ids}) AND g.Data1=g.entry THEN 0 ELSE g.Data1 END'
    else:v='CASE '+''.join(f"WHEN g.entry={r['entry']} AND HEX(g.name)='{r['after_hex']}' THEN CONVERT(0x{r['before_hex']} USING utf8mb4) " for r in rs)+'ELSE g.name END'
   parts.append(f"COALESCE(HEX(CAST({v} AS CHAR CHARACTER SET utf8mb4)),'NULL')")
  h="SHA2(CONCAT_WS('|',"+','.join(parts)+'),256)'
  out.append(f"SELECT '{tag}','{t}',COUNT(*),SHA2(GROUP_CONCAT(h ORDER BY h SEPARATOR ','),256) FROM (SELECT {h} AS h FROM {t} g) sig;")
 return '\n'.join(out)

def prepare(p,kind,out,sql_dir):
 migrations='\n'.join((sql_dir/filename(kind,n)).read_text(encoding='utf-8') for n in batches(p));base=setup(p,kind,migrations);cases=[]
 def case(label,body,refusal=False,count=0):
  (out/f'{label}.sql').write_text(base+body+"SELECT 'EOF';",encoding='utf-8');cases.append(dict(label=label,refusal=refusal,count=count))
 for n in batches(p):
  rows=[r for r in p['repairs'] if r['batch']==n];ids=','.join(str(r['entry']) for r in rows);mig=(sql_dir/filename(kind,n)).read_text(encoding='utf-8');e=rows[0]['entry']
  def count(tag,fixed):
   clauses=[f"(entry={r['entry']} AND "+(f"HEX(name)='{r['after_hex'] if fixed else r['before_hex']}'" if kind=='names' else f"Data1={r['entry'] if fixed else 0}")+')' for r in rows]
   return f"SELECT '{tag}',COUNT(*) FROM gameobject_template WHERE "+' OR '.join(clauses)+';'
  case(f'batch{n:02}-positive',count('before',False)+snapshot(p,kind,'baseline')+mig+count('fixed',True)+snapshot(p,kind,'fixed')+mig+count('replay',True)+snapshot(p,kind,'replay'),count=len(rows))
  changes={'custom-field':f"UPDATE gameobject_template SET `{field(kind)}`="+("'Custom safe name'" if kind=='names' else '299')+f' WHERE entry={e};','metadata':f'UPDATE gameobject_template SET flags=flags+1 WHERE entry={e};','missing':f'DELETE FROM gameobject_template WHERE entry={e};'}
  if kind=='loot':changes.update({'source-chance':f'UPDATE gameobject_loot_template SET Chance=Chance+1 WHERE Entry={e};','source-missing':f'DELETE FROM gameobject_loot_template WHERE Entry={e} LIMIT 1;'})
  for label,change in changes.items():case(f'batch{n:02}-{label}',change+snapshot(p,kind,'custom',False)+mig+snapshot(p,kind,'unchanged',False),True)
 (out/'cases.json').write_text(json.dumps(cases),encoding='utf-8');live=['SET SESSION TRANSACTION READ ONLY;SET SESSION group_concat_max_len=10000000;']
 for r in p['repairs']:
  fixed=f"HEX(g.name)='{r['after_hex']}'" if kind=='names' else 'g.Data1=g.entry'
  live.append(f"SELECT 'live',{r['entry']},COUNT(*) FROM gameobject_template g WHERE {pred(r,kind)} AND {fixed};")
  if kind=='loot':live.append(f"SELECT 'source',{r['entry']},COUNT(*)={r['loot_rows']} AND SHA2(GROUP_CONCAT({fingerprint(p['loot_columns'])} ORDER BY g.Item SEPARATOR ','),256)='{r['loot_signature']}' FROM gameobject_loot_template g WHERE Entry={r['entry']};")
 for n in batches(p):
  f=sql_dir/filename(kind,n);sha=hashlib.sha1(f.read_bytes().replace(bytes([13,10]),bytes([10]))).hexdigest().upper();live.append(f"SELECT 'update','{f.name}',COUNT(*) FROM updates WHERE name='{f.name}' AND hash='{sha}' AND state='RELEASED';")
 (out/'live.sql').write_text('\n'.join(live+["SELECT 'EOF';"]),encoding='utf-8')

def verify(p,kind,out,live):
 cases=json.loads((out/'cases.json').read_text())
 for c in cases:
  rs=[l.split('\t') for l in (out/(c['label']+'.tsv')).read_text(encoding='utf-8-sig').splitlines()];assert rs[-1]==['EOF']
  if c['refusal']:assert [r[1:] for r in rs if r[0]=='custom']==[r[1:] for r in rs if r[0]=='unchanged']
  else:
   assert all(int(r[1])==c['count'] for r in rs if len(r)==2 and r[0] in ('before','fixed','replay'));assert [r[1:] for r in rs if r[0]=='baseline']==[r[1:] for r in rs if r[0]=='fixed' and len(r)>2]==[r[1:] for r in rs if r[0]=='replay' and len(r)>2]
 if live:
  rs=(out/'live.tsv').read_text(encoding='utf-8-sig').splitlines();assert len(rs)==len(p['repairs'])*(2 if kind=='loot' else 1)+len(batches(p))+1 and rs[-1]=='EOF' and all(r.split('\t')[-1]=='1' for r in rs[:-1])
 print(f"PASS{len(p['repairs'])} {kind} TEMP baseline/repair/replay/full metadata and control preservation;{sum(c['refusal'] for c in cases)} exact refusals"+('; actual live/updater verified' if live else ''))
def main():
 ap=argparse.ArgumentParser();ap.add_argument('mode',choices=('prepare','verify'));ap.add_argument('--kind',choices=('names','loot'),required=True);ap.add_argument('--output',type=Path,required=True);ap.add_argument('--sql-directory',type=Path,required=True);ap.add_argument('--live',action='store_true');a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=True)
 if a.mode=='prepare':prepare(load(a.kind),a.kind,a.output,a.sql_directory)
 else:verify(load(a.kind),a.kind,a.output,a.live)
if __name__=='__main__':main()

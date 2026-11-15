from pathlib import Path
import json,argparse,hashlib
from following_go_loot import load,predicate,filename,ROOT
from gameobject_name_encoding import fingerprint
def setup(p,mig):
 ids=','.join(str(r['entry']) for r in p['repairs']);s=['SET SESSION group_concat_max_len=10000000;']
 for t,scope in [('gameobject_template',f'g.entry IN({ids},239692)'),('gameobject_loot_template',f'g.Entry IN({ids})')]:s.append(f'CREATE TEMPORARY TABLE seed_{t} LIKE {t};INSERT INTO seed_{t} SELECT g.* FROM {t} g WHERE {scope};CREATE TEMPORARY TABLE {t} LIKE seed_{t};INSERT INTO {t} SELECT * FROM seed_{t};')
 s+=['SET SESSION TRANSACTION READ ONLY;',mig]
 for r in p['repairs']:s.append(f"UPDATE gameobject_template g SET Data1=0 WHERE {predicate(r)};")
 return '\n'.join(s)
def snapshot(p,tag,normalize=True):
 out=[];ids=','.join(str(r['entry']) for r in p['repairs'])
 for t,cols in [('gameobject_template',list(p['repairs'][0]['values'])),('gameobject_loot_template',p['loot_columns'])]:
  parts=[]
  for c in cols:
   v=f'g.`{c}`'
   if normalize and t=='gameobject_template' and c=='Data1':v=f'CASE WHEN g.entry IN({ids}) AND g.Data1=g.entry THEN 0 ELSE g.Data1 END'
   parts.append(f"COALESCE(HEX(CAST({v} AS CHAR CHARACTER SET utf8mb4)),'NULL')")
  h="SHA2(CONCAT_WS('|',"+','.join(parts)+'),256)';out.append(f"SELECT '{tag}','{t}',COUNT(*),SHA2(GROUP_CONCAT(h ORDER BY h SEPARATOR ','),256) FROM (SELECT {h} AS h FROM {t} g) sig;")
 return '\n'.join(out)
def prepare(p,out,sql_dir):
 migrations='\n'.join((sql_dir/filename(n)).read_text(encoding='utf-8') for n in range(1,11));base=setup(p,migrations);cases=[]
 def case(label,body,refusal=False):
  (out/f'{label}.sql').write_text(base+body+"SELECT 'EOF';",encoding='utf-8');cases.append(dict(label=label,refusal=refusal))
 for n in range(1,11):
  rows=[r for r in p['repairs'] if r['batch']==n];ids=','.join(str(r['entry']) for r in rows);mig=(sql_dir/filename(n)).read_text(encoding='utf-8');e=rows[0]['entry'];count=lambda tag,state:f"SELECT '{tag}',COUNT(*) FROM gameobject_template WHERE entry IN({ids}) AND Data1={state};"
  case(f'batch{n:02}-positive',count('before','0')+snapshot(p,'baseline')+mig+count('fixed','entry')+snapshot(p,'fixed')+mig+count('replay','entry')+snapshot(p,'replay'))
  for label,change in {'custom-link':f'UPDATE gameobject_template SET Data1=299 WHERE entry={e};','metadata':f'UPDATE gameobject_template SET flags=flags+1 WHERE entry={e};','missing':f'DELETE FROM gameobject_template WHERE entry={e};','source-chance':f'UPDATE gameobject_loot_template SET Chance=Chance+1 WHERE Entry={e};','source-missing':f'DELETE FROM gameobject_loot_template WHERE Entry={e} LIMIT 1;'}.items():case(f'batch{n:02}-{label}',change+snapshot(p,'custom',False)+mig+snapshot(p,'unchanged',False),True)
 (out/'cases.json').write_text(json.dumps(cases),encoding='utf-8');live=['SET SESSION TRANSACTION READ ONLY;SET SESSION group_concat_max_len=10000000;']
 for r in p['repairs']:
  live.append(f"SELECT 'live',{r['entry']},COUNT(*) FROM gameobject_template g WHERE {predicate(r)} AND g.Data1=g.entry;");live.append(f"SELECT 'source',{r['entry']},COUNT(*)={r['loot_rows']} AND SHA2(GROUP_CONCAT({fingerprint(p['loot_columns'])} ORDER BY g.Item SEPARATOR ','),256)='{r['loot_signature']}' FROM gameobject_loot_template g WHERE Entry={r['entry']};")
 for n in range(1,11):
  f=sql_dir/filename(n);live.append(f"SELECT 'update','{f.name}',COUNT(*) FROM updates WHERE name='{f.name}' AND hash='{hashlib.sha1(f.read_bytes().replace(bytes([13,10]),bytes([10]))).hexdigest().upper()}' AND state='RELEASED';")
 (out/'live.sql').write_text('\n'.join(live+["SELECT 'EOF';"]),encoding='utf-8')
def verify(out,live):
 for c in json.loads((out/'cases.json').read_text()):
  rs=[l.split('\t') for l in (out/(c['label']+'.tsv')).read_text(encoding='utf-8-sig').splitlines()];assert rs[-1]==['EOF']
  if c['refusal']:assert [r[1:] for r in rs if r[0]=='custom']==[r[1:] for r in rs if r[0]=='unchanged']
  else:
   assert all(r[1]=='10' for r in rs if len(r)==2 and r[0] in ('before','fixed','replay'));assert [r[1:] for r in rs if r[0]=='baseline']==[r[1:] for r in rs if r[0]=='fixed' and len(r)>2]==[r[1:] for r in rs if r[0]=='replay' and len(r)>2]
 if live:
  rs=(out/'live.tsv').read_text(encoding='utf-8-sig').splitlines();assert len(rs)==211 and rs[-1]=='EOF' and all(r.split('\t')[-1]=='1' for r in rs[:-1])
 print('PASS100 TEMP ten cohort baseline/repair/replay,101 templates/authored source preservation,50 exact drift refusals'+('; live100/source100/updater10 verified' if live else ''))
def main():
 ap=argparse.ArgumentParser();ap.add_argument('mode',choices=('prepare','verify'));ap.add_argument('--output',type=Path,required=True);ap.add_argument('--sql-directory',type=Path,required=True);ap.add_argument('--live',action='store_true');a=ap.parse_args();a.output.mkdir(parents=True,exist_ok=True)
 if a.mode=='prepare':prepare(load(),a.output,a.sql_directory)
 else:verify(a.output,a.live)
if __name__=='__main__':main()

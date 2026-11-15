from pathlib import Path
import argparse,json,hashlib
from loot_link_repairs import load,ROOT,FILE,predicate
from gameobject_name_encoding import fingerprint
def setup(p,mig):
 ids=','.join(str(r['entry']) for r in p['repairs']);out=['SET SESSION group_concat_max_len=10000000;']
 for t,scope in [('creature_template',f'g.entry IN({ids},299)'),('creature_loot_template',f'g.Entry IN({ids})')]:out.append(f'CREATE TEMPORARY TABLE seed_{t} LIKE {t};INSERT INTO seed_{t} SELECT g.* FROM {t} g WHERE {scope};CREATE TEMPORARY TABLE {t} LIKE seed_{t};INSERT INTO {t} SELECT * FROM seed_{t};')
 out+=['SET SESSION TRANSACTION READ ONLY;',mig]
 for r in p['repairs']:out.append(f"UPDATE creature_template g SET lootid=0 WHERE {predicate(r)};")
 return '\n'.join(out)
def snapshot(p,tag,normalize=True):
 out=[];ids=','.join(str(r['entry']) for r in p['repairs'])
 for t,cols in [('creature_template',list(p['repairs'][0]['values'])),('creature_loot_template',p['loot_columns'])]:
  parts=[]
  for c in cols:
   v=f'g.`{c}`'
   if normalize and t=='creature_template' and c=='lootid':v=f'CASE WHEN g.entry IN({ids}) AND g.lootid=g.entry THEN 0 ELSE g.lootid END'
   parts.append(f"COALESCE(HEX(CAST({v} AS CHAR CHARACTER SET utf8mb4)),'NULL')")
  h="SHA2(CONCAT_WS('|',"+','.join(parts)+'),256)';out.append(f"SELECT '{tag}','{t}',COUNT(*),SHA2(GROUP_CONCAT(h ORDER BY h SEPARATOR ','),256) FROM (SELECT {h} AS h FROM {t} g) sig;")
 return '\n'.join(out)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('mode',choices=('prepare','verify'));ap.add_argument('--output',type=Path,required=True);ap.add_argument('--sql-file',type=Path,required=True);ap.add_argument('--live',action='store_true');a=ap.parse_args();p=load();out=a.output;out.mkdir(parents=True,exist_ok=True);mig=a.sql_file.read_text(encoding='utf-8')
 if a.mode=='prepare':
  base=setup(p,mig);cases=[]
  def case(label,body,refusal=False):
   (out/f'{label}.sql').write_text(base+body+"SELECT 'EOF';",encoding='utf-8');cases.append(dict(label=label,refusal=refusal))
  def count(state):return f"SELECT '{state}',COUNT(*) FROM creature_template g WHERE g.entry IN("+','.join(str(r['entry']) for r in p['repairs'])+') AND g.lootid='+('0' if state=='before' else 'g.entry')+';'
  case('positive',count('before')+snapshot(p,'baseline')+mig+count('fixed')+snapshot(p,'fixed')+mig+count('replay')+snapshot(p,'replay'))
  for label,change in {'custom-link':'UPDATE creature_template SET lootid=299 WHERE entry=108699;','metadata':'UPDATE creature_template SET minlevel=minlevel+1 WHERE entry=108699;','missing':'DELETE FROM creature_template WHERE entry=108699;','source-chance':'UPDATE creature_loot_template SET Chance=Chance+1 WHERE Entry=108699;','source-missing':'DELETE FROM creature_loot_template WHERE Entry=108699 LIMIT 1;'}.items():case(label,change+snapshot(p,'custom',False)+mig+snapshot(p,'unchanged',False),True)
  (out/'cases.json').write_text(json.dumps(cases),encoding='utf-8');live=['SET SESSION TRANSACTION READ ONLY;SET SESSION group_concat_max_len=10000000;']
  for r in p['repairs']:live.append(f"SELECT 'live',{r['entry']},COUNT(*) FROM creature_template g WHERE {predicate(r)} AND g.lootid=g.entry;");live.append(f"SELECT 'source',{r['entry']},COUNT(*)={r['loot_rows']} AND SHA2(GROUP_CONCAT({fingerprint(p['loot_columns'])} ORDER BY g.Item SEPARATOR ','),256)='{r['loot_signature']}' FROM creature_loot_template g WHERE Entry={r['entry']};")
  live.append(f"SELECT 'update',COUNT(*) FROM updates WHERE name='{FILE}' AND hash='{hashlib.sha1(a.sql_file.read_bytes()).hexdigest().upper()}' AND state='RELEASED';");(out/'live.sql').write_text('\n'.join(live+["SELECT 'EOF';"]),encoding='utf-8')
 else:
  for c in json.loads((out/'cases.json').read_text()):
   rs=[l.split('\t') for l in (out/(c['label']+'.tsv')).read_text(encoding='utf-8-sig').splitlines()];assert rs[-1]==['EOF']
   if c['refusal']:assert [r[1:] for r in rs if r[0]=='custom']==[r[1:] for r in rs if r[0]=='unchanged']
   else:
    assert all(r[1]=='13' for r in rs if r[0] in ('before','fixed','replay') and len(r)==2);assert [r[1:] for r in rs if r[0]=='baseline']==[r[1:] for r in rs if r[0]=='fixed' and len(r)>2]==[r[1:] for r in rs if r[0]=='replay' and len(r)>2]
  if a.live:
   rs=(out/'live.tsv').read_text(encoding='utf-8-sig').splitlines();assert len(rs)==28 and rs[-1]=='EOF' and all(r.split('\t')[-1]=='1' for r in rs[:-1])
  print('PASS13 actual TEMP baseline/repair/replay, metadata/loot-source preservation and5 drift refusals'+('; live13/source13/updater verified' if a.live else ''))
if __name__=='__main__':main()

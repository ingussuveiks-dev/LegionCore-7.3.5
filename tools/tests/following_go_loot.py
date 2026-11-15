"""Explicit links to existing authored chest loot, without spawning objects."""
from pathlib import Path
import json,argparse
from gameobject_name_encoding import fingerprint
ROOT=Path(__file__).resolve().parents[2]
def load():return json.loads((ROOT/'tools/tests/FollowingGoLoot20261115Repairs.json').read_text(encoding='utf-8'))
def filename(n):return f'2026_11_15_86_go_loot_batch{n:02}.sql'
def predicate(r):return f"(g.entry={r['entry']} AND g.Data1 IN(0,{r['entry']}) AND {fingerprint([c for c in r['values'] if c!='Data1'])}='{r['other_fingerprint']}')"
def sql(p,n):
 rows=[r for r in p['repairs'] if r['batch']==n];s=[f'-- Cohort{n}: ten existing authored chest loot links; no spawns or grants.','START TRANSACTION;SET SESSION group_concat_max_len=10000000;',"SET @following_go_safe=((SELECT COUNT(*) FROM gameobject_template g WHERE "+' OR '.join(predicate(r) for r in rows)+')=10);']
 for r in rows:
  e=r['entry'];s.append(f"SET @following_go_safe=@following_go_safe AND ((SELECT COUNT(*)={r['loot_rows']} AND SHA2(GROUP_CONCAT({fingerprint(p['loot_columns'])} ORDER BY g.Item SEPARATOR ','),256)='{r['loot_signature']}' FROM gameobject_loot_template g WHERE g.Entry={e})) AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup={e}) AND NOT EXISTS(SELECT 1 FROM personal_loot_template WHERE type=0 AND entry={e});")
 s.append("SET @following_go_guard=IF(COALESCE(@following_go_safe,0),'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Following chest link/source differs; review before updating''');PREPARE following_go_guard FROM @following_go_guard;EXECUTE following_go_guard;DEALLOCATE PREPARE following_go_guard;")
 for r in rows:s.append(f"UPDATE gameobject_template g SET Data1={r['entry']} WHERE @following_go_safe AND g.Data1=0 AND {predicate(r)};")
 return '\n'.join(s+['COMMIT;'])+'\n'
def validate(p):
 assert len(p['repairs'])==len({r['entry'] for r in p['repairs']})==100
 for r in p['repairs']:
  v=r['values'];assert v['type']==3 and v['Data1']==v['Data6']==v['Data8']==v['Data25']==v['Data26']==v['Data15']==0 and not v['ScriptName'] and not v['AIName'] and r['loot_rows']>0
 assert all(sum(r['batch']==n for r in p['repairs'])==10 for n in range(1,11))
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--sql-directory',type=Path,required=True);ap.add_argument('--generate',action='store_true');a=ap.parse_args();p=load();validate(p);a.sql_directory.mkdir(parents=True,exist_ok=True)
 for n in range(1,11):
  f=a.sql_directory/filename(n)
  if a.generate:f.write_text(sql(p,n),encoding='utf-8',newline='\n')
  else:assert f.read_text(encoding='utf-8')==sql(p,n)
 print('PASS100 unique existing chest loot links/ten cohorts; exact template/source guards;1 static template and99 whose actual dynamic placement is unestablished; no quest traversal claim')
if __name__=='__main__':main()

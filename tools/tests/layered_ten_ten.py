from pathlib import Path
import json,argparse
from gameobject_name_encoding import fingerprint
from DoubleUtf8NameRecovery import recover
ROOT=Path(__file__).resolve().parents[2]
def load(kind):return json.loads((ROOT/'tools/tests'/('LayeredNames20261115.json' if kind=='names' else 'QuestChests20261115.json')).read_text(encoding='utf-8'))
def filename(kind,n):return f'2026_11_15_{88 if kind=="names" else 89}_{kind}_batch{n:02}.sql'
def field(kind):return 'name' if kind=='names' else 'Data1'
def pred(r,kind):
 f=field(kind);value=f"HEX(g.name) IN('{r['before_hex']}','{r['after_hex']}')" if kind=='names' else f"g.Data1 IN(0,{r['entry']})"
 return f"(g.entry={r['entry']} AND {value} AND {fingerprint([c for c in r['values'] if c!=f])}='{r['other_fingerprint']}')"
def sql(p,kind,n):
 rs=[r for r in p['repairs'] if r['batch']==n];s=[f'-- {kind} cohort{n}: exact existing-field repair; no spawns or grants.','START TRANSACTION;SET SESSION group_concat_max_len=10000000;',"SET @layered_safe=((SELECT COUNT(*) FROM gameobject_template g WHERE "+' OR '.join(pred(r,kind) for r in rs)+f')={len(rs)});']
 if kind=='loot':
  for r in rs:
   e=r['entry'];s.append(f"SET @layered_safe=@layered_safe AND ((SELECT COUNT(*)={r['loot_rows']} AND SHA2(GROUP_CONCAT({fingerprint(p['loot_columns'])} ORDER BY g.Item SEPARATOR ','),256)='{r['loot_signature']}' FROM gameobject_loot_template g WHERE g.Entry={e})) AND NOT EXISTS(SELECT 1 FROM conditions WHERE SourceTypeOrReferenceId=4 AND SourceGroup={e}) AND NOT EXISTS(SELECT 1 FROM personal_loot_template WHERE (type=0 AND entry={e}) OR goEntry={e});")
 s.append("SET @layered_guard=IF(COALESCE(@layered_safe,0),'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Layered repair row/source differs; review before updating''');PREPARE layered_guard FROM @layered_guard;EXECUTE layered_guard;DEALLOCATE PREPARE layered_guard;")
 for r in rs:
  value=f"CONVERT(0x{r['after_hex']} USING utf8mb4)" if kind=='names' else str(r['entry']);before=f"HEX(g.name)='{r['before_hex']}'" if kind=='names' else 'g.Data1=0';s.append(f"UPDATE gameobject_template g SET `{field(kind)}`={value} WHERE @layered_safe AND {before} AND {pred(r,kind)};")
 return '\n'.join(s+['COMMIT;'])+'\n'
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--kind',choices=('names','loot'),required=True);ap.add_argument('--sql-directory',type=Path,required=True);ap.add_argument('--generate',action='store_true');a=ap.parse_args();p=load(a.kind);rs=p['repairs'];assert len(rs)==len({r['entry'] for r in rs})==(37 if a.kind=='names' else 63)
 for r in rs:
  if a.kind=='names':assert recover(r['values']['name'])==r['after_name'] and r['after_hex']==r['after_name'].encode().hex().upper()
  else:assert r['values']['type']==3 and r['values']['Data1']==r['values']['Data6']==r['values']['Data8']==r['values']['Data25']==r['values']['Data26']==0 and not r['values']['AIName'] and not r['values']['ScriptName'] and r['loot_rows']>0
 a.sql_directory.mkdir(parents=True,exist_ok=True)
 for n in range(1,max(r['batch'] for r in rs)+1):
  f=a.sql_directory/filename(a.kind,n)
  if a.generate:f.write_text(sql(p,a.kind,n),encoding='utf-8',newline='\n')
  else:assert f.read_text(encoding='utf-8')==sql(p,a.kind,n)
 if a.kind=='names':
  for text in ('Correct','Дерево','Ð¢','Ã','ÃÂ¢\x00','�'):
   try:recover(text)
   except ValueError:pass
   else:raise AssertionError('Unsafe/correct/incomplete text admitted')
 print(f'PASS{len(rs)} {a.kind}: unique exact field repairs, full metadata/source guards; no translation, placement or client claim')
if __name__=='__main__':main()

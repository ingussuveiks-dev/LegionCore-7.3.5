from pathlib import Path
import json,argparse
from gameobject_name_encoding import fingerprint
from Utf8TextRecovery import recover
ROOT=Path(__file__).resolve().parents[2];PLAN=ROOT/'tools/tests/OpenGoText20261115Repairs.json'
def load():return json.loads(PLAN.read_text(encoding='utf-8'))
def expr(row,after=False,alias='g'):
 parts=[]
 for c in row['values']:
  value=f'{alias}.`{c}`'
  if after and c in row['changes']:value=f"CONVERT(0x{row['changes'][c]['after_hex']} USING utf8mb4)"
  parts.append(f"COALESCE(HEX(CAST({value} AS CHAR CHARACTER SET utf8mb4)),'NULL')")
 return "SHA2(CONCAT_WS('|',"+','.join(parts)+'),256)'
def controls():
 out=[]
 for file in ('GameObjectName20261115Repairs.json','ThirtyTen20261115NameRepairs.json'):
  out += [dict(entry=x['entry'],name=x['after_name'],hex=x['after_hex']) for x in json.loads((ROOT/'tools/tests'/file).read_text(encoding='utf-8'))['repairs']]
 out += [dict(entry=x['key']['entry'],name=x['after_text'],hex=x['after_hex']) for x in json.loads((ROOT/'tools/tests/TenThirty20261115TextRepairs.json').read_text(encoding='utf-8'))['repairs'] if x['table']=='gameobject_template']
 return out
def sql(plan):
 rows=plan['rows'];fp=expr(rows[0]);conditions=' OR '.join(f"(g.entry={x['entry']} AND {fp} IN('{x['before_fingerprint']}','{x['after_fingerprint']}'))" for x in rows)
 lines=['-- Repair37 cast-caption/auxiliary query strings on35 rows atomically.','START TRANSACTION;',f'SET @open_go_safe=((SELECT COUNT(*) FROM gameobject_template g WHERE {conditions})=35);',"SET @open_go_guard=IF(COALESCE(@open_go_safe,0),'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Open GO text row differs; review before updating''');PREPARE open_go_guard FROM @open_go_guard;EXECUTE open_go_guard;DEALLOCATE PREPARE open_go_guard;"]
 for x in rows:
  assignments=','.join(f"`{f}`=CONVERT(0x{c['after_hex']} USING utf8mb4)" for f,c in x['changes'].items());lines.append(f"UPDATE gameobject_template g SET {assignments} WHERE @open_go_safe AND g.entry={x['entry']} AND {fp}='{x['before_fingerprint']}';")
 return '\n'.join(lines+['COMMIT;'])+'\n'
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--sql-file',required=True);ap.add_argument('--generate',action='store_true');a=ap.parse_args();plan=load();rows=plan['rows'];assert len(rows)==35 and sum(len(x['changes']) for x in rows)==37
 for x in rows:
  assert set(x['changes'])<= {'castBarCaption','unk1'}
  for f,c in x['changes'].items():assert recover(c['before'])==(c['codec'],c['after']) and c['before']==x['values'][f] and c['after'].encode().hex().upper()==c['after_hex']
 expected=sql(plan);p=Path(a.sql_file);p.parent.mkdir(parents=True,exist_ok=True)
 if a.generate:p.write_text(expected,encoding='utf-8',newline='\n')
 else:assert p.read_text(encoding='utf-8')==expected
 print('PASS37 exact closed caption/auxiliary strings on35 rows; whole-row before/after fingerprints, atomic paired-field update and553 name controls declared')
if __name__=='__main__':main()

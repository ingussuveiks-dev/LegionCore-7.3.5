"""Closed UTF-8 restoration; whole-row and whole-dialogue drift guards."""
from pathlib import Path
import json,argparse,hashlib
from Utf8TextRecovery import recover
from gameobject_name_encoding import fingerprint
ROOT=Path(__file__).resolve().parents[2]
def load():return json.loads((ROOT/'tools/tests/NextText20261115Repairs.json').read_text(encoding='utf-8'))
def filename(n):return f'2026_11_15_83_text_batch{n:02}.sql'
def key(r,alias='g'):return ' AND '.join(f'{alias}.`{k}`={v}' for k,v in r['key'].items())
def match(r,states=('before_hex','after_hex')):
 return '('+key(r)+f" AND HEX(g.`{r['field']}`) IN("+','.join("'"+r[s]+"'" for s in states)+f") AND {fingerprint([c for c in r['values'] if c!=r['field']])}='{r['other_fingerprint']}')"
def group_hash(plan):
 rows=[r for r in plan['repairs'] if r['table']=='creature_text'];cols=list(rows[0]['values']);parts=[]
 for c in cols:
  v=f'g.`{c}`'
  if c=='Text':v='CASE '+' '.join(f"WHEN {match(r,('after_hex',))} THEN CONVERT(0x{r['before_hex']} USING utf8mb4)" for r in rows)+' ELSE g.Text END'
  parts.append(f"COALESCE(HEX(CAST({v} AS CHAR CHARACTER SET utf8mb4)),'NULL')")
 return "SHA2(CONCAT_WS('|',"+','.join(parts)+'),256)'
def sql(plan,n):
 rows=[r for r in plan['repairs'] if r['batch']==n];out=[f'-- Batch{n}: closed UTF-8 restoration only; all gameplay fields retained.','START TRANSACTION;SET SESSION group_concat_max_len=10000000;','SET @next_text_safe=1;']
 for t in sorted({r['table'] for r in rows}):
  sub=[r for r in rows if r['table']==t];out.append(f"SET @next_text_safe=@next_text_safe AND ((SELECT COUNT(*) FROM `{t}` g WHERE "+' OR '.join(match(r) for r in sub)+f')={len(sub)});')
 groups={(r['values']['CreatureID'],r['values']['GroupID']) for r in rows if r['table']=='creature_text'}
 for g in plan['groups']:
  if (g['entry'],g['group']) in groups:out.append(f"SET @next_text_safe=@next_text_safe AND ((SELECT COUNT(*)={len(g['rows'])} AND SHA2(GROUP_CONCAT(h ORDER BY h SEPARATOR ','),256)='{g['signature']}' FROM (SELECT {group_hash(plan)} AS h FROM creature_text g WHERE g.CreatureID={g['entry']} AND g.GroupID={g['group']}) known_group));")
 out+= ["SET @next_text_guard=IF(COALESCE(@next_text_safe,0),'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Next text row/group differs; review before updating''');PREPARE next_text_guard FROM @next_text_guard;EXECUTE next_text_guard;DEALLOCATE PREPARE next_text_guard;"]
 for r in rows:out.append(f"UPDATE `{r['table']}` g SET `{r['field']}`=CONVERT(0x{r['after_hex']} USING utf8mb4) WHERE @next_text_safe AND {match(r,('before_hex',))};")
 return '\n'.join(out+['COMMIT;'])+'\n'
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--sql-directory',required=True);ap.add_argument('--generate',action='store_true');a=ap.parse_args();p=load();rs=p['repairs'];assert len(rs)==87 and len({(r['table'],tuple(r['key'].items()),r['before_hex']) for r in rs})==87
 for r in rs:assert recover(r['values'][r['field']])==(r['codec'],r['after_text']) and r['after_hex']==r['after_text'].encode().hex().upper()
 for g in p['groups']:assert g['signature']==hashlib.sha256(','.join(sorted(r['fingerprint'] for r in g['rows'])).encode()).hexdigest()
 dest=Path(a.sql_directory);dest.mkdir(parents=True,exist_ok=True)
 for n in range(1,10):
  f=dest/filename(n);s=sql(p,n)
  if a.generate:f.write_text(s,encoding='utf-8',newline='\n')
  else:assert f.read_text(encoding='utf-8')==s
 print('PASS87 closed restorations:49 NPC,26 gossip,10 request-items,1 objective,1 localized system string; full dialogue drift guards')
if __name__=='__main__':main()

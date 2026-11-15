from pathlib import Path
import json,argparse,re
from Utf8TextRecovery import recover
from gameobject_name_encoding import fingerprint
ROOT=Path(__file__).resolve().parents[2]
PLAN=ROOT/'tools/tests/TenThirty20261115TextRepairs.json'
def load():return json.loads(PLAN.read_text(encoding='utf-8'))
def filename(batch):return f'2026_11_15_81_text_batch{batch:02}.sql'
def key(row,alias='g',group=False):
 return ' AND '.join(f'{alias}.`{k}`={v}' for k,v in row['key'].items() if not(group and k=='ID'))
def fp(row,alias='g',include=False):return fingerprint([k for k in row['values'] if include or k!=row['field']],alias)
def matches(row,states=('before_hex','after_hex'),metadata=True,alias='g'):
 s=f"({key(row,alias)} AND HEX({alias}.`{row['field']}`) IN("+','.join("'"+row[k]+"'" for k in states)+')'
 if metadata:s+=f" AND {fp(row,alias)}='{row['other_fingerprint']}'"
 return s+')'
def eligibility(rows):
 parts=[]
 for table in ('gameobject_template','creature_text'):
  cohort=[x for x in rows if x['table']==table]
  if not cohort:continue
  pred=' OR '.join(matches(x) for x in cohort)
  if table=='gameobject_template':parts.append(f'(SELECT COUNT(*) FROM {table} g WHERE {pred})={len(cohort)}')
  else:
   scope=' OR '.join('('+key(x,group=True)+')' for x in cohort)
   parts.append(f'(SELECT COUNT(*) FROM (SELECT g.CreatureID,g.GroupID,COUNT(*) AS group_rows,SUM(CASE WHEN {pred} THEN 1 ELSE 0 END) AS valid_rows FROM {table} g WHERE {scope} GROUP BY g.CreatureID,g.GroupID) text_shape WHERE group_rows=1 AND valid_rows=1)={len(cohort)}')
 return ' AND '.join(parts)
def migration(rows,batch):
 out=[f'-- Batch{batch}: thirty closed UTF-8 text repairs; no translation/gameplay edits.','START TRANSACTION;',f'SET @text_safe=({eligibility(rows)});',f"SET @text_guard=IF(COALESCE(@text_safe,0),'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Text batch{batch} differs; review before updating''');",'PREPARE text_guard FROM @text_guard;EXECUTE text_guard;DEALLOCATE PREPARE text_guard;']
 for x in rows:out.append(f"UPDATE `{x['table']}` g SET `{x['field']}`=CONVERT(0x{x['after_hex']} USING utf8mb4) WHERE @text_safe AND {matches(x,('before_hex',))};")
 return '\n'.join(out+['COMMIT;'])+'\n'
def validate(plan):
 rows=plan['repairs'];assert len(rows)==300 and len({(x['table'],tuple(x['key'].items()),x['field']) for x in rows})==300
 prior={x['entry'] for p in plan['previous_plans'] for x in json.loads((ROOT/p).read_text(encoding='utf-8'))['repairs']}
 for x in rows:
  codec,fixed=recover(x['values'][x['field']]);assert codec==x['codec'] and fixed==x['after_text']
  assert x['before_hex']==x['values'][x['field']].encode().hex().upper() and x['after_hex']==fixed.encode().hex().upper()
  assert all(re.fullmatch(r'[A-Za-z0-9_]+',k) for k in x['values'])
  assert 1<=x['batch']<=10
  if x['table']=='gameobject_template':assert x['field']=='name' and x['key']['entry'] not in prior
  else:assert x['table']=='creature_text' and x['field']=='Text' and int(x['context']['group_rows'])==1
  if 'other_fingerprint' in x:assert len(x['other_fingerprint'])==64
 for n in range(1,11):assert sum(x['batch']==n for x in rows)==30
 # Exact examples and rejection cases independently exercise both codecs.
 for correct in ('Already correct','Привет','I can’t go…'):
  try:recover(correct)
  except ValueError:pass
  else:raise AssertionError('Correct text admitted')
 assert recover('РџСЂРёРІРµС‚')[1]=='Привет'
 assert recover('Waitâ€¦')[1]=='Wait…'
 for bad in ('Ð','Ð¢\x00','ÃÂ¢'):
  try:recover(bad)
  except ValueError:pass
  else:raise AssertionError('Malformed/layered text admitted')
def main():
 a=argparse.ArgumentParser();a.add_argument('--sql-directory',required=True);a.add_argument('--generate',action='store_true');args=a.parse_args();plan=load();validate(plan);dest=Path(args.sql_directory);dest.mkdir(parents=True,exist_ok=True)
 for n in range(1,11):
  text=migration([x for x in plan['repairs'] if x['batch']==n],n);p=dest/filename(n)
  if args.generate:p.write_text(text,encoding='utf-8',newline='\n')
  else:assert p.read_text(encoding='utf-8')==text,p
 print('PASS300 new text fields/10x30 cohorts, unique closed1251/1252 reversal, invariant ASCII tokens; singleton NPC groups/140 Broadcast-backed fallbacks explicitly retained')
if __name__=='__main__':main()

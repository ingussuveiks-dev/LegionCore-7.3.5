"""Thirty cohorts of additional exact GameObject name byte repairs.

Reuse the validated single-pass codec and SQL builder; do not translate or
change gameplay. Static/dynamic references and query-only definitions remain
separate evidence categories in the plan.
"""
from pathlib import Path
import argparse,json
from gameobject_name_encoding import recover,sql_for,META
EXTRA={'references','usage','usage_tags'}
def filename(batch):return f'2026_11_15_80_name_batch{batch:02}.sql'
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--sql-directory',required=True);ap.add_argument('--generate',action='store_true');a=ap.parse_args();r=Path(__file__).resolve().parents[2]
 plan=json.loads((r/'tools/tests/ThirtyTen20261115NameRepairs.json').read_text(encoding='utf-8'));rows=plan['repairs'];prior=json.loads((r/plan['previous_plan']).read_text(encoding='utf-8'))['repairs'];assert len(rows)==len({x['entry'] for x in rows})==300
 assert not {x['entry'] for x in rows}&{x['entry'] for x in prior}
 fields=[f for f in rows[0] if f not in META|EXTRA]
 assert fields==[f for f in prior[0] if f not in META]
 for x in rows:
  assert recover(x['name'])==x['after_name'];assert x['before_hex']==x['name'].encode('utf-8').hex().upper();assert x['after_hex']==x['after_name'].encode('utf-8').hex().upper()
  assert 1<=x['batch']<=30 and len(x['other_fingerprint'])==64
  assert bool(int(x['spawn_count']))==('static' in x['usage_tags'])
  assert bool(x['references'])==('db2' in x['usage_tags'])
  for ref in x['references']:
   assert int(ref[1])==x['entry'] and ref[0] in ('GAMEOBJECT','SUMMON')
   if ref[0]=='SUMMON':assert int(ref[4]) in (50,76,104,171)
 assert sum(bool(x['usage_tags']) for x in rows)==180
 assert sum(bool(int(x['spawn_count'])) for x in rows)==132
 dest=Path(a.sql_directory);dest.mkdir(parents=True,exist_ok=True)
 for batch in range(1,31):
  cohort=[x for x in rows if x['batch']==batch];assert len(cohort)==10;p=dest/filename(batch);expected=sql_for(cohort,fields,batch)
  if a.generate:p.write_text(expected,encoding='utf-8',newline='\n')
  else:assert p.read_text(encoding='utf-8')==expected,p
 print('PASS300 distinct exact name-byte recoveries/30x10 cohorts; previous100 excluded;132 static/48 other authored references/120 query-only definitions explicitly separated; SQL generation exact')
if __name__=='__main__':main()

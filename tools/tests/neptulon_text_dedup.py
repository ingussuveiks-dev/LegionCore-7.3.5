"""Remove one exact encoding duplicate; keep the existing correct dialogue row."""
from pathlib import Path
import argparse,json,re,hashlib
from next_text_repairs import load,ROOT
from Utf8TextRecovery import recover
from gameobject_name_encoding import fingerprint
FILE='2026_11_15_85_neptulon_text_duplicate.sql'
def info():
 d=load()['deferred'][0];r=d['row'];assert recover(r['values']['Text'])==(r['codec'],r['after_text']);rows=d['group']['rows'];correct=next(x for x in rows if x['values']['Text']==r['after_text']);bad=next(x for x in rows if x['values']['Text']==r['values']['Text']);assert {k:v for k,v in correct['values'].items() if k!='Text'}=={k:v for k,v in bad['values'].items() if k!='Text'};return r,correct,bad
def sql():
 r,good,bad=info();fp=fingerprint(list(r['values']));return f"""-- Keep the already-correct Neptulon40792/group12 row; remove its exact
-- misdecoded duplicate. All other columns, voice and timing are identical.
START TRANSACTION;
SET @neptulon_safe=((SELECT COUNT(*) IN(1,2) AND SUM({fp}='{good['fingerprint']}')=1 AND SUM({fp} NOT IN('{good['fingerprint']}','{bad['fingerprint']}'))=0 FROM creature_text g WHERE g.CreatureID=40792 AND g.GroupID=12));
SET @neptulon_guard=IF(COALESCE(@neptulon_safe,0),'DO 0','SIGNAL SQLSTATE ''45000'' SET MESSAGE_TEXT=''Neptulon duplicate group differs; review before updating''');PREPARE neptulon_guard FROM @neptulon_guard;EXECUTE neptulon_guard;DEALLOCATE PREPARE neptulon_guard;
DELETE g FROM creature_text g WHERE @neptulon_safe AND g.CreatureID=40792 AND g.GroupID=12 AND {fp}='{bad['fingerprint']}';
COMMIT;
"""
def setup():
 r,good,bad=info();cols=list(r['values']);values=','.join(f'CONVERT(0x{str(bad["values"][c]).encode().hex().upper()} USING utf8mb4)' if isinstance(bad['values'][c],str) else str(bad['values'][c]) for c in cols)
 return f"CREATE TEMPORARY TABLE seed_creature_text LIKE creature_text;INSERT INTO seed_creature_text SELECT * FROM creature_text WHERE CreatureID=40792 AND GroupID=12;CREATE TEMPORARY TABLE creature_text LIKE seed_creature_text;INSERT INTO creature_text SELECT * FROM seed_creature_text;SET SESSION TRANSACTION READ ONLY;"+sql()+f"INSERT INTO creature_text ({','.join('`'+c+'`' for c in cols)}) VALUES({values});"
def snapshot(tag):
 r,good,bad=info();return f"SELECT '{tag}',COUNT(*),SHA2(GROUP_CONCAT({fingerprint(list(r['values']))} ORDER BY Text SEPARATOR ','),256) FROM creature_text g;"
def main():
 ap=argparse.ArgumentParser();ap.add_argument('mode',choices=('generate','prepare','verify','native'));ap.add_argument('--output',type=Path);ap.add_argument('--sql-file',type=Path,required=True);ap.add_argument('--live',action='store_true');a=ap.parse_args();r,good,bad=info()
 if a.mode=='generate':a.sql_file.parent.mkdir(parents=True,exist_ok=True);a.sql_file.write_text(sql(),encoding='utf-8',newline='\n');print('PASS exact duplicate metadata and closed text reversal; keep correct source row')
 elif a.mode=='prepare':
  out=a.output;out.mkdir(parents=True,exist_ok=True);cases=[]
  def case(label,body,refusal=False):
   (out/f'{label}.sql').write_text('SET SESSION group_concat_max_len=10000000;'+setup()+body+"SELECT 'EOF';",encoding='utf-8');cases.append(dict(label=label,refusal=refusal))
  case('positive',snapshot('baseline')+sql()+snapshot('fixed')+sql()+snapshot('replay'))
  for label,change in {'voice':'UPDATE creature_text SET Sound=Sound+1;','no-correct':f"DELETE FROM creature_text WHERE HEX(Text)='{r['after_hex']}';",'extra':"INSERT INTO creature_text SELECT CreatureID,GroupID,ID,'__foreign__',Type,Language,Probability,Emote,Duration,Sound,BroadcastTextID,MinTimer,MaxTimer,SpellID,comment FROM seed_creature_text LIMIT 1;"}.items():case(label,change+snapshot('custom')+sql()+snapshot('unchanged'),True)
  (out/'cases.json').write_text(json.dumps(cases),encoding='utf-8');(out/'live.sql').write_text('SET SESSION TRANSACTION READ ONLY;'+f"SELECT 'live',COUNT(*)=1 AND SUM({fingerprint(list(r['values']))}='{good['fingerprint']}')=1 FROM creature_text g WHERE CreatureID=40792 AND GroupID=12;SELECT 'update',COUNT(*) FROM updates WHERE name='{FILE}' AND state='RELEASED' AND hash='{hashlib.sha1(a.sql_file.read_bytes()).hexdigest().upper()}';SELECT 'EOF';",encoding='utf-8')
 elif a.mode=='verify':
  for c in json.loads((a.output/'cases.json').read_text()):
   rs=[l.split('\t') for l in (a.output/(c['label']+'.tsv')).read_text(encoding='utf-8-sig').splitlines()];assert rs[-1]==['EOF']
   if c['refusal']:assert rs[0][1:]==rs[1][1:]
   else:assert rs[0][1]=='2' and rs[1][1]=='1' and rs[1][1:]==rs[2][1:]
  if a.live:assert (a.output/'live.tsv').read_text(encoding='utf-8-sig').splitlines()==['live\t1','update\t1','EOF']
  print('PASS exact duplicate2-to1, replay and3 drift refusals'+('; live correct row/updater verified' if a.live else ''))
 else:
  out=a.output;out.mkdir(parents=True,exist_ok=True);lines=['struct NameRow{uint32 entry;char const* before;char const* after;};static NameRow const NameRows[]={{0,"",""}};static char const* NameIds="0";static char const* NameMigrations[]={"DO 0"};'];(out/'GameObjectNameRows.inc').write_text('\n'.join(lines),encoding='utf-8')
  def arr(name,s):
   s='\n'.join(l for l in s.splitlines() if not l.startswith('--'));return 'static char const* '+name+'[]={'+','.join('R"NEPT('+x+')NEPT"' for x in re.split(r";(?=(?:[^']*'[^']*')*[^']*$)",s) if x.strip())+'};'
  (out/'NeptulonDedup.inc').write_text(arr('Migration',sql())+arr('Setup',setup().split('SET SESSION TRANSACTION READ ONLY;')[0])+arr('Seed',setup().split('SET SESSION TRANSACTION READ ONLY;')[1])+f'\nstatic char const* CorrectHex="{r["after_hex"]}";static char const* BrokenHex="{r["before_hex"]}";',encoding='utf-8')
  source=(ROOT/'src/server/game/Texts/CreatureTextMgr.cpp').read_text(encoding='utf-8');start=source.index('class CreatureTextBuilder\n');end=source.index('class PlayerTextBuilder',start);(out/'NativeCreatureTextBuilder.inc').write_text(source[start:end],encoding='utf-8')
if __name__=='__main__':main()

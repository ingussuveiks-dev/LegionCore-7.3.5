from pathlib import Path
import json,argparse,hashlib,re
from lili_phase import load,cols,sql,SCOPES,ROOT,FILE
from gameobject_name_encoding import fingerprint
def setup():
 s=[]
 for t,scope in SCOPES.items():s.append(f'CREATE TEMPORARY TABLE seed_{t} LIKE `{t}`;INSERT INTO seed_{t} SELECT g.* FROM `{t}` g WHERE {scope};CREATE TEMPORARY TABLE `{t}` LIKE seed_{t};INSERT INTO `{t}` SELECT * FROM seed_{t};')
 return '\n'.join(s)
def snapshot(p,tag,normalize=False):
 s=[]
 for t in SCOPES:
  parts=[]
  for c in cols(p,t):
   value=f'g.`{c}`'
   if normalize and t=='creature' and c=='PhaseId':value="CASE WHEN g.guid=340237 AND g.PhaseId='5910 6735' THEN '6735' ELSE g.PhaseId END"
   parts.append(f"COALESCE(HEX(CAST({value} AS CHAR CHARACTER SET utf8mb4)),'NULL')")
  fp="SHA2(CONCAT_WS('|',"+','.join(parts)+'),256)';s.append(f"SELECT '{tag}','{t}',COUNT(*),SHA2(GROUP_CONCAT(h ORDER BY h SEPARATOR ','),256) FROM (SELECT {fp} AS h FROM `{t}` g) shapes;")
 return '\n'.join(s)
def main():
 ap=argparse.ArgumentParser();ap.add_argument('mode',choices=('prepare','verify','native'));ap.add_argument('--output',type=Path,required=True);ap.add_argument('--sql-file',type=Path,required=True);ap.add_argument('--live',action='store_true');a=ap.parse_args();out=a.output;out.mkdir(parents=True,exist_ok=True);p=load();mig=a.sql_file.read_text(encoding='utf-8');assert mig==sql(p)
 if a.mode=='prepare':
  base='SET SESSION group_concat_max_len=10000000;'+setup()+'SET SESSION TRANSACTION READ ONLY;'+mig+"UPDATE creature SET PhaseId='6735' WHERE guid=340237 AND @lili_phase_safe;";cases=[]
  def case(label,body,refusal=False):
   (out/f'{label}.sql').write_text(base+body+"SELECT 'EOF';",encoding='utf-8');cases.append(dict(label=label,refusal=refusal))
  case('positive',snapshot(p,'before',True)+mig+snapshot(p,'fixed',True)+mig+snapshot(p,'replay',True)+"SELECT 'phase',guid,PhaseId FROM creature ORDER BY guid;")
  for label,change in {'custom-phase':"UPDATE creature SET PhaseId='6918' WHERE guid=340237;",'spawn-metadata':'UPDATE creature SET AiID=0 WHERE guid=340237;','missing-spawn':'DELETE FROM creature WHERE guid=372111;','remote-father':"UPDATE creature SET PhaseId='6735' WHERE guid=382407;",'dialogue':"UPDATE smart_scripts SET action_param2=action_param2+1 WHERE entryorguid=100475 AND id=1;",'base-phase':"UPDATE phase_definitions SET flags=1 WHERE zoneId=7902 AND entry=1;",'phase-condition':"DELETE FROM conditions WHERE SourceTypeOrReferenceId=23 AND SourceGroup=7902 LIMIT 1;"}.items():case(label,change+snapshot(p,'custom')+mig+snapshot(p,'unchanged'),True)
  (out/'cases.json').write_text(json.dumps(cases),encoding='utf-8');f=a.sql_file;live='SET SESSION TRANSACTION READ ONLY;SET SESSION group_concat_max_len=10000000;'+mig+"SELECT 'live',@lili_phase_safe,(SELECT PhaseId FROM creature WHERE guid=340237);"+f"SELECT 'update',COUNT(*) FROM updates WHERE name='{FILE}' AND hash='{hashlib.sha1(f.read_bytes().replace(bytes([13,10]),bytes([10]))).hexdigest().upper()}' AND state='RELEASED';SELECT 'EOF';"
  # Live verification is read-only: evaluate the exact guards, never execute DML.
  live=live.replace("START TRANSACTION;",'').replace("UPDATE creature SET PhaseId='5910 6735' WHERE @lili_phase_safe AND guid=340237 AND PhaseId='6735';",'').replace('COMMIT;','');(out/'live.sql').write_text(live,encoding='utf-8')
 elif a.mode=='verify':
  for c in json.loads((out/'cases.json').read_text()):
   rs=[l.split('\t') for l in (out/(c['label']+'.tsv')).read_text(encoding='utf-8-sig').splitlines()];assert rs[-1]==['EOF']
   if c['refusal']:assert [r[1:] for r in rs if r[0]=='custom']==[r[1:] for r in rs if r[0]=='unchanged']
   else:
    assert [r[1:] for r in rs if r[0]=='before']==[r[1:] for r in rs if r[0]=='fixed']==[r[1:] for r in rs if r[0]=='replay'];assert [r for r in rs if r[0]=='phase']==[['phase','340237','5910 6735'],['phase','372111','5910'],['phase','382407','6918']]
  if a.live:assert (out/'live.tsv').read_text(encoding='utf-8-sig').splitlines()==['live\t1\t5910 6735','update\t1','EOF']
  print('PASS scoped phase union/replay/full row preservation and7 drift refusals'+('; actual live context/updater verified' if a.live else ''))
 else:
  def array(name,text):
   text='\n'.join(l for l in text.splitlines() if not l.startswith('--'));ss=[s for s in re.split(r";(?=(?:[^']*'[^']*')*[^']*$)",text) if s.strip()];return 'static std::string const '+name+'[]={'+','.join('std::string('+')+std::string('.join('R"LILI('+s[i:i+16000]+')LILI"' for i in range(0,len(s),16000))+')' for s in ss)+'};'
  base=(ROOT/'tools/tests/RockawayChannelNative.cpp').read_text(encoding='utf-8');inc=base[:base.index('// The scheduler lookup')]
  player=(ROOT/'tools/tests/prepare_following_go_loot_native.py').read_text(encoding='utf-8')
  # Reuse only the retained fixture declaration, not its chest/quest main.
  def block(source,start):
   a=source.index(start);j=source.index('{',a)+1;depth=1
   while depth:depth+=(source[j]=='{')-(source[j]=='}');j+=1
   return source[a:j]
  ps=block((ROOT/'tools/tests/RockawayOffensiveNative.cpp').read_text(encoding='utf-8'),'struct NativePlayer : Player');i=ps.index('        // Controlled accepted-task state;');j=ps.index('\n    }',i);ps=ps[:i]+ps[j:]
  inc+='\n#include "WorldSession.h"\n#include "CellImpl.h"\n#include "Opcodes.h"\n'+ps+';\n'+array('Setup',setup())+array('Migration',mig)+"\nstatic uint32 const SpawnIds[]={340237,372111,382407};";(out/'NativeLiliPhase.inc').write_text(inc,encoding='utf-8')
if __name__=='__main__':main()

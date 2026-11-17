"""TEMP-only native SQL guard/replay checks; never mutates live quest rows."""
import argparse,json
from pathlib import Path
IDS=(282680,282681,282682,283341,283342,283343,284274,284275)
SCOPE='QuestID IN(40225,40938,41626,42006,42452,42771)'
FIELDS=('ID','QuestID','Type','StorageIndex','ObjectID','Amount','Flags','Flags2','TaskStep','Description','VerifiedBuild','Bugged')
def snapshot(tag):
 args=','.join("'"+k+"',"+k for k in FIELDS)
 return f"SELECT '{tag}',JSON_OBJECT({args}) FROM quest_objectives ORDER BY ID;\n"
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('mode',choices=('prepare','verify'));p.add_argument('--output',type=Path,required=True);p.add_argument('--sql-file',type=Path,required=True);p.add_argument('--live',action='store_true');a=p.parse_args()
 if a.mode=='prepare':
  a.output.mkdir(parents=True,exist_ok=True);migration=a.sql_file.read_text(encoding='utf-8-sig')
  setup=f'CREATE TEMPORARY TABLE seed LIKE quest_objectives;INSERT INTO seed SELECT * FROM quest_objectives WHERE {SCOPE};CREATE TEMPORARY TABLE quest_objectives LIKE seed;INSERT INTO quest_objectives SELECT * FROM seed;UPDATE quest_objectives SET Amount=0 WHERE ID IN({",".join(map(str,IDS))});\n'
  cases=[]
  def case(name,change='',refusal=False):
   text=setup+change+snapshot('before')+migration+snapshot('after')
   if not refusal:text+=migration+snapshot('replay')
   text+="SELECT 'EOF';\n";(a.output/(name+'.sql')).write_text(text);cases.append({'name':name,'refusal':refusal})
  case('baseline-repair-replay')
  for label,change in {'foreign-amount':'UPDATE quest_objectives SET Amount=2 WHERE ID=282680;','foreign-flag':'UPDATE quest_objectives SET Flags=0 WHERE ID=282681;','foreign-target':'UPDATE quest_objectives SET ObjectID=1 WHERE ID=282682;','foreign-quest':'UPDATE quest_objectives SET QuestID=1 WHERE ID=283341;','foreign-storage':'UPDATE quest_objectives SET StorageIndex=7 WHERE ID=283342;','foreign-description':"UPDATE quest_objectives SET Description='custom' WHERE ID=283343;",'foreign-bugged':'UPDATE quest_objectives SET Bugged=1 WHERE ID=284274;','missing':'DELETE FROM quest_objectives WHERE ID=284275;'}.items():case(label,change,True)
  (a.output/'cases.json').write_text(json.dumps(cases))
  (a.output/'live.sql').write_text('SET SESSION TRANSACTION READ ONLY;'+snapshot('live')+"SELECT 'EOF';\n")
 else:
  cases=json.loads((a.output/'cases.json').read_text())
  if a.live:cases.append({'name':'live','refusal':False})
  for c in cases:
   lines=(a.output/(c['name']+'.tsv')).read_text(encoding='utf-8-sig').splitlines();assert lines[-1]=='EOF'
   rows={}
   for line in lines[:-1]:
    tag,payload=line.split('\t',1);r=json.loads(payload);rows.setdefault(tag,{})[r['ID']]=r
   if c['name']=='live':assert all(rows['live'][i]['Amount']==1 for i in IDS);continue
   if c['refusal']:assert rows['before']==rows['after'];continue
   assert rows['after']==rows['replay']
   expected=json.loads(json.dumps(rows['before']))
   for i in IDS:expected[str(i)]['Amount']=1
   assert {str(k):v for k,v in rows['after'].items()}==expected
   assert all(rows['after'][i]['Flags']&(4|16) for i in IDS)
  print('PASS TEMP full-row preservation,original zero baseline,exact eight-count repair,replay,eight foreign/missing refusals'+(' and live readback' if a.live else '')+';required objective counts/flags unchanged')
if __name__=='__main__':main()

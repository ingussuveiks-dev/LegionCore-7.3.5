"""Six additional authored-source links, separate from the earlier100 ledger."""
from pathlib import Path
import argparse,json
from layered_ten_ten import ROOT,pred,sql
def load(kind='loot'):
 assert kind=='loot'
 return json.loads((ROOT/'tools/tests/TailLoot20261115.json').read_text(encoding='utf-8'))
def filename(kind,n):
 assert kind=='loot' and n==1
 return '2026_11_15_90_tail_loot.sql'
def main():
 ap=argparse.ArgumentParser();ap.add_argument('--sql-directory',type=Path,required=True);ap.add_argument('--generate',action='store_true');a=ap.parse_args();p=load();rs=p['repairs'];assert len(rs)==len({r['entry'] for r in rs})==6
 assert {r['entry'] for r in rs}=={211423,269138,272009,272010,272387,273274}
 for r in rs:
  v=r['values'];assert v['Data1']==0 and v['type']==(25 if r['entry']==211423 else 3) and not v['AIName'] and not v['ScriptName'] and r['loot_rows']==1
 text=sql(p,'loot',1);a.sql_directory.mkdir(parents=True,exist_ok=True);f=a.sql_directory/filename('loot',1)
 if a.generate:f.write_text(text,encoding='utf-8',newline='\n')
 else:assert f.read_text(encoding='utf-8')==text
 print('PASS6 exact additional links: five chests and one fishing pool; no new placement, quest credit or source changes')
if __name__=='__main__':main()

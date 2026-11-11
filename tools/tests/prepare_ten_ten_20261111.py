from pathlib import Path
import json,sys
from ten_ten_20261111_scope import ROOT,PLAN,require_fixed
out=Path(sys.argv[1]);mode=sys.argv[2]if len(sys.argv)>2 else 'fixed';batch=int(sys.argv[3])if len(sys.argv)>3 else 1
rows=[e for e in PLAN['changes']if e['batch']==batch];assert len(rows)==10
case=sys.argv[4]if len(sys.argv)>4 else None
require_fixed([case]if case else[e['key']for e in rows]);out.mkdir(parents=True,exist_ok=True);text=''
for i,e in enumerate(rows):
 if batch==2:
  sources=[]
  for edit in e['edits']:
   actual=(ROOT/edit['path']).read_text(encoding='utf-8-sig')
   sources.append(edit['old'if mode=='baseline'or edit['new']not in actual else'new'])
  text+=('struct CharDisplay{'+''.join(sources)+'};\n')if e['case']=='char_display'else sources[0]+';\n'
  continue
 edit=e['edits'][0];actual=(ROOT/edit['path']).read_text(encoding='utf-8-sig');source=edit['old'if mode=='baseline'or edit['new']not in actual else'new']
 if e['kind']=='aura_state':text+=source+';\n'
 elif e['kind']=='console':text+='struct Case'+str(i)+'{'+source+'};\n'
 else:raise AssertionError(e['kind'])
text+='#define CONSOLE_DISPATCH '+''.join('case '+str(i)+':RunConsole(Case'+str(i)+'::'+e['method']+','+str(i)+');break;'for i,e in enumerate(rows)if e['kind']=='console')+'\n'
if batch==2:
 fields={'Mainhand':15,'Head':0,'Shoulders':2,'Chest':4,'Waist':5,'Legs':6,'Feet':7,'Wrists':8,'Hands':9,'Back':14,'Offhand':16,'Ranged':17,'Tabard':18,'Shirt':3}
 text+='#define DISPLAY_CASES '+','.join('{CharDisplay::'+method+','+str(fields[method.removeprefix('HandleCharDisplay').removesuffix('Command')])+',&Visuals::m_vis'+method.removeprefix('HandleCharDisplay').removesuffix('Command')+'}'for method in rows[0]['methods'])+'\n'
(out/'Selected.inc').write_text(text,encoding='utf8');(out/'cases.json').write_text(json.dumps([e['key']for e in rows]),encoding='utf8')
print('Extracted ten complete selected methods/classes;external endpoints controlled;native registration/context separate')

from pathlib import Path
import json,sys
from ten_ten_20261111_scope import ROOT,PLAN,require_fixed
out=Path(sys.argv[1]);mode=sys.argv[2]if len(sys.argv)>2 else 'fixed';batch=int(sys.argv[3])if len(sys.argv)>3 else 1
rows=[e for e in PLAN['changes']if e['batch']==batch];assert len(rows)==10
case=sys.argv[4]if len(sys.argv)>4 else None
require_fixed([case]if case else[e['key']for e in rows]);out.mkdir(parents=True,exist_ok=True);text=''
for i,e in enumerate(rows):
 edit=e['edits'][0];actual=(ROOT/edit['path']).read_text(encoding='utf-8-sig');source=edit['old'if mode=='baseline'or edit['new']not in actual else'new']
 if e['kind']=='aura_state':text+=source+';\n'
 elif e['kind']=='console':text+='struct Case'+str(i)+'{'+source+'};\n'
 else:raise AssertionError(e['kind'])
text+='#define CONSOLE_DISPATCH '+''.join('case '+str(i)+':RunConsole(Case'+str(i)+'::'+e['method']+','+str(i)+');break;'for i,e in enumerate(rows)if e['kind']=='console')+'\n'
(out/'Selected.inc').write_text(text,encoding='utf8');(out/'cases.json').write_text(json.dumps([e['key']for e in rows]),encoding='utf8')
print('Extracted ten complete selected methods/classes;external endpoints controlled;native registration/context separate')

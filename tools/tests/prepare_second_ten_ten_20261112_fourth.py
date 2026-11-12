from pathlib import Path
import json,re,sys
from second_ten_ten_20261112_scope import ROOT,PLAN,require_fixed
out=Path(sys.argv[1]);mode=sys.argv[2];require_fixed();out.mkdir(parents=True,exist_ok=True);rows=[r for r in PLAN['changes']if r['batch']==4];assert len(rows)==10
text='#define FIXED_LIFETIME_CONTROL\n' if mode=='fixed' else ''
for i,row in enumerate(rows):
 text+='namespace Fourth'+str(i)+'{\n'+';\n'.join(ed['old'if mode=='baseline'else'new']for ed in row['edits'])+';\n}\n'
labels=set(re.findall(r'\b[A-Z][A-Z_0-9]+\b',text))|{'TYPEID_PLAYER','TYPEID_UNIT','IN_PROGRESS'}
fixed={'TYPEID_PLAYER':1,'TYPEID_UNIT':2,'POINT_MOTION_TYPE':1,'IN_MILLISECONDS':1000,'GO_STATE_READY':0,'IN_PROGRESS':1,'UNIT_STATE_STUNNED':1,'UNIT_STATE_ONVEHICLE':2}
for n in sorted(labels-set(fixed)-{'NULL','RAND','AI','NPC','GO'}):fixed[n]=len(fixed)+100
(out/'Labels.inc').write_text('enum{'+','.join(k+'='+str(v)for k,v in fixed.items())+'};\n');(out/'Selected.inc').write_bytes(text.encode('utf8',errors='surrogateescape'));(out/'cases.json').write_text(json.dumps([r['key']for r in rows]));print('Prepared eleven complete production AI classes for ten distinct owner/target lifetime families;world/motion/cast endpoints controlled;actual native actor/catalogue checks separate.')

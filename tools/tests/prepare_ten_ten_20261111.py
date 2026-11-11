from pathlib import Path
import json,sys,re
from ten_ten_20261111_scope import ROOT,PLAN,require_fixed
out=Path(sys.argv[1]);mode=sys.argv[2]if len(sys.argv)>2 else 'fixed';batch=int(sys.argv[3])if len(sys.argv)>3 else 1
rows=[e for e in PLAN['changes']if e['batch']==batch];assert len(rows)==10
case=sys.argv[4]if len(sys.argv)>4 else None
require_fixed([case]if case else[e['key']for e in rows]);out.mkdir(parents=True,exist_ok=True);text=''
def block(source,pattern):
 masked=re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"',lambda m:re.sub(r'[^\n]',' ',m[0]),source,flags=re.S);match=re.search(pattern,masked);assert match,pattern
 start=masked.index('{',match.start());end=start+1;depth=1
 while depth:depth+=(masked[end]=='{')-(masked[end]=='}');end+=1
 return source[match.start():end]
for i,e in enumerate(rows):
 if batch==2:
  sources=[]
  for edit in e['edits']:
   actual=(ROOT/edit['path']).read_text(encoding='utf-8-sig')
   sources.append(edit['old'if mode=='baseline'or edit['new']not in actual else'new'])
  text+=('struct CharDisplay{'+''.join(sources)+'};\n')if e['case']=='char_display'else sources[0]+';\n'
  continue
 if batch==3:
  sources=[]
  for edit in e['edits']:
   actual=(ROOT/edit['path']).read_text(encoding='utf-8-sig')
   sources.append(edit['old'if mode=='baseline'or edit['new']not in actual else'new'])
  text+='struct Third'+str(i)+':Base{using Base::Base;'+''.join(sources)+'};\n'
  continue
 if batch==4:
  edit=e['edits'][e.get('class_edit_index',0)]
  actual=(ROOT/edit['path']).read_text(encoding='utf-8-sig')
  chosen=edit['old'if mode=='baseline'or edit['new']not in actual else'new']
  if e['case']=='avatar':
   text+='struct AvatarFrame:Base{using Base::Base;std::shared_ptr<uint8> _delayedAILifetimeGuard=std::make_shared<uint8>(0);void Queue(){'+chosen+'}};\n'
  elif e['case']=='scarab':
   for j,path in enumerate(e['paths']):
    source=(ROOT/path).read_text(encoding='utf-8-sig')
    for edit in reversed(e['edits']):
     if edit['path']==path and mode=='baseline':assert source.count(edit['new'])==1;source=source.replace(edit['new'],edit['old'],1)
    outdoor='CallOfTheScarab'in path
    body=block(source,r'bool\s+Update\s*\(')if outdoor else block(source,r'void\s+Update\s*\(')
    initialization=block(source,r'void\s+HandleGameEventStart\s*\(')if outdoor else block(source,r'void\s+Initialize\s*\(')
    text+='struct Scarab'+str(j)+':ScarabBase<'+('bool'if outdoor else'void')+'>{'+initialization+body+'};\n'
   text+='#define SCARAB_CASES '+''.join('RunScarab<Scarab'+str(j)+' >('+('true'if 'CallOfTheScarab'in path else'false')+');'for j,path in enumerate(e['paths']))+'\n'
  else:text+=chosen+';\n'
  continue
 edit=e['edits'][0];actual=(ROOT/edit['path']).read_text(encoding='utf-8-sig');source=edit['old'if mode=='baseline'or edit['new']not in actual else'new']
 if e['kind']=='aura_state':text+=source+';\n'
 elif e['kind']=='console':text+='struct Case'+str(i)+'{'+source+'};\n'
 else:raise AssertionError(e['kind'])
text+='#define CONSOLE_DISPATCH '+''.join('case '+str(i)+':RunConsole(Case'+str(i)+'::'+e['method']+','+str(i)+');break;'for i,e in enumerate(rows)if e['kind']=='console')+'\n'
if batch==2:
 fields={'Mainhand':15,'Head':0,'Shoulders':2,'Chest':4,'Waist':5,'Legs':6,'Feet':7,'Wrists':8,'Hands':9,'Back':14,'Offhand':16,'Ranged':17,'Tabard':18,'Shirt':3}
 text+='#define DISPLAY_CASES '+','.join('{CharDisplay::'+method+','+str(fields[method.removeprefix('HandleCharDisplay').removesuffix('Command')])+',&Visuals::m_vis'+method.removeprefix('HandleCharDisplay').removesuffix('Command')+'}'for method in rows[0]['methods'])+'\n'
if batch==3:
 labels=set(re.findall(r'\b[A-Z][A-Z_0-9]+\b',text))-{'NULL','EFFECT_0','IN_MILLISECONDS','AI','M_PI'}
 text='enum ThirdLabels{'+','.join(label+'='+str(i+100)for i,label in enumerate(sorted(labels)))+'};\n'+text
if batch==4:
 excluded={'NULL','AI','MS','EFFECT_0','EFFECT_1','EFFECT_2','EFFECT_3','EFFECT_4','TYPEID_PLAYER','TYPEID_UNIT','POWER_RAGE','IN_MILLISECONDS','SPELL_CAST_OK','UNIT_FIELD_VIRTUAL_ITEMS','EQUIPMENT_SLOT_MAINHAND','EQUIPMENT_SLOT_OFFHAND','EQUIPMENT_SLOT_RANGED','EVENT_CALL_OF_THE_SCARAB','AURA_EFFECT_HANDLE_REAL','AURA_EFFECT_HANDLE_REAL_OR_REAPPLY_MASK'}
 labels=(set(re.findall(r'\b[A-Z][A-Z_0-9]+\b',text))|{'SPELL_FREEZiNG_CIRCLE'})-excluded
 text='enum FourthLabels{'+','.join(label+'='+str(i+100)for i,label in enumerate(sorted(labels)))+'};\n'+text
(out/'Selected.inc').write_text(text,encoding='utf8');(out/'cases.json').write_text(json.dumps([e['key']for e in rows]),encoding='utf8')
print('Extracted ten complete selected methods/classes;external endpoints controlled;native registration/context separate')

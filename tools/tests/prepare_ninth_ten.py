from pathlib import Path
import sys,json,re,subprocess
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);key=sys.argv[2];baseline=sys.argv[3]=='baseline';e=next(e for e in json.loads((r/'tools/tests/NinthTenRepairs.json').read_text())if e['key']==key)
def source(p):return subprocess.check_output(['git','-C',str(r),'show','846511e03:'+Path(p).as_posix()]).decode('utf8')if baseline else(r/p).read_text(encoding='utf-8-sig')
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
s=source(e['paths'][0]);selected='';constants=''
if key=='GarroshHeal':
 c=re.search(r'me->ModifyHealth\(int(?:32|64)\(me->CountPctFromMaxHealth\(60\)\)\);',s).group();selected='struct Frame:Base{using Base::Base;void Heal(){'+c+'}};'
elif key=='Garrosh':
 a=re.search(r'uint(?:32|64) hp = me->GetHealth\(\);\s*uint32 power = me->GetPower\(POWER_ENERGY\);\s*garroshrealm->SetHealth\(hp\);\s*garroshrealm->SetPower\(POWER_ENERGY, power\);',s).group()
 b=re.search(r'int32 power = garroshrealm->GetPower\(POWER_ENERGY\);\s*uint(?:32|64) hp = garroshrealm->GetHealth\(\);\s*garroshrealm->DespawnOrUnsummon\(\);\s*instance->SetData\(DATA_UPDATE_GARROSH_REALM, 0\);\s*me->SetPower\(POWER_ENERGY, power\);\s*me->SetHealth\(hp\);',s).group()
 c=re.search(r'me->ModifyHealth\(int(?:32|64)\(me->CountPctFromMaxHealth\(60\)\)\);',s).group()
 d=re.search(r'uint(?:32|64) hp = me->GetHealth\(\);\s*if \(Creature\* stormwindgarrosh = me->SummonCreature\([^\n]+\)\)\s*\{\s*stormwindgarrosh->SetHealth\(hp\);',s).group()
 selected='struct Frame:Base{using Base::Base;void Enter(Unit*garroshrealm){'+a+'}void Return(Unit*garroshrealm){'+b+'}void Final(){'+d+'}}};'
elif key=='HpThreshold':selected=block(s,'void AuraEffect::HandleAuraProcOnHpBelow(')
else:
 scope=block(s,re.search(r'(?:class|struct)\s+'+e['cls']+r'\s*:',s).group())
 # Some AI structs use no space before the colon.
 body=block(scope,'void '+e['method']+'(')
 member=''
 if key=='Immerseus':member=re.search(r'uint(?:32|64) lasthp[^;]*;',scope).group()+'\n'
 selected='struct Frame:Base{using Base::Base;'+member+body+'};'
 if key=='Miasma':constants+=re.search(r'uint32 ancientbarrierbar\[3\][^;]*;',s).group()+'\n'
# Resolve used numeric enum constants from this exact script/header, without invented IDs.
texts=[s]
for h in (r/e['paths'][0]).parent.glob('*.h'):texts.append(source(h.relative_to(r)))
texts += [(r/x).read_text(encoding='utf-8-sig',errors='replace')for x in ['src/server/game/Miscellaneous/SharedDefines.h','src/server/game/Spells/Auras/SpellAuraDefines.h','src/server/game/Entities/Object/Updates/UpdateFields.h','src/server/game/Entities/Unit/Unit.h','src/server/game/Entities/Unit/UnitDefines.h']]
values={}
for text in texts:
 text=re.sub(r'/\*.*?\*/|//[^\n]*','',text,flags=re.S)
 for en in re.findall(r'enum[^;{}]*\{([^{}]*)\}',text):
  n=-1
  for item in en.split(','):
   item=item.strip()
   if not item:continue
   m=re.fullmatch(r'(\w+)\s*(?:=\s*(.*))?',item)
   if not m:continue
   name,expr=m.groups()
   try:
    if expr:
     expr=re.sub(r'(?<=[0-9a-fA-F])[uUlL]+\b','',expr)
     n=eval(expr,{'__builtins__':{}},values)
    else:n+=1
    if isinstance(n,int):values[name]=n
   except Exception:continue
 for name,number in re.findall(r'^\s*#define\s+(\w+)\s+(0x[0-9A-Fa-f]+|\d+)\b',text,re.M):values[name]=int(number,0)
used=set(re.findall(r'\b[A-Z][A-Z_0-9]+\b',re.sub(r'/\*.*?\*/|//[^\n]*|"[^"]*"','',selected+constants,flags=re.S)))
manual={'NULL','UI64FMTD','TC_LOG_DEBUG','EFFECT_0','EFFECT_1','AURA_EFFECT_HANDLE_REAL','SPELL_CAST_OK','IN_PROGRESS'}
for name in sorted(used-manual):
 if name in values:constants='constexpr uint32 '+name+'='+str(values[name])+';\n'+constants
 else:raise ValueError('Unresolved original constant '+name)
(out/'Selected.inc').write_text(selected,encoding='utf8');(out/'Constants.inc').write_text(constants,encoding='utf8')

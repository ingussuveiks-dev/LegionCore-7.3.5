"""Extract selected production handlers, pinned controls and native aura classes."""
from pathlib import Path
import sys,json,re,subprocess
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);plan=json.loads((r/'tools/tests/TwelfthTenRepairs.json').read_text());out.mkdir(parents=True,exist_ok=True)
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
def source(e,old):return subprocess.check_output(['git','-C',str(r),'show','0131dff5f:'+e['paths'][0]]).decode('utf8')if old else(r/e['paths'][0]).read_text(encoding='utf-8-sig')
def selected(e,old):
 s=source(e,old);scope=block(s,re.search(r'(?:class|struct)\s+'+e['cls']+r'\s*:',s).group());key=e['key']
 if key=='ValithriaHealth':
  stmt=re.search(r'me->SetHealth\((?:uint32\(me->GetMaxHealth\(\) \* 0.50\)|me->CountPctFromMaxHealth\(50\))\);',scope).group();assert scope.count(stmt)==2
  return 'void Setup(){'+stmt+'}void IntroHealth(){'+stmt+'}'
 if key=='Delrissa':return 'void Heal(uint32 diff){'+block(scope,'if (HealTimer <= diff)')+'}'
 if key=='Reflection':return block(scope,'void OnProc_0(')+'\n'+block(scope,'void OnProc_1(')
 fn='DamageTaken'if old and key in ['Firesworn','Nefarian']else e['method']
 body=block(scope,'void '+fn+'(')
 if key in ['Firesworn','Nefarian']:body+='void Hit(Unit*a,uint32&d,DamageEffectType t){'+fn+'(a,d,t);}'
 return body
def constants(e,code):
 s=source(e,False);texts=[s]+[(r/e['paths'][0]).parent.joinpath(h.name).read_text(encoding='utf-8-sig')for h in(r/e['paths'][0]).parent.glob('*.h')]
 texts += [(r/x).read_text(encoding='utf-8-sig',errors='replace')for x in ['src/server/game/Miscellaneous/SharedDefines.h','src/server/game/Spells/Auras/SpellAuraDefines.h','src/server/game/Entities/Object/Updates/UpdateFields.h','src/server/game/Entities/Unit/Unit.h','src/server/game/Entities/Unit/UnitDefines.h','src/server/game/Instances/InstanceScript.h','src/server/game/Entities/Object/Object.h']]
 values={}
 for text in texts:
  text=re.sub(r'/\*.*?\*/|//[^\n]*','',text,flags=re.S)
  for en in re.findall(r'enum[^;{}]*\{([^{}]*)\}',text):
   n=-1
   for item in en.split(','):
    m=re.fullmatch(r'(\w+)\s*(?:=\s*(.*))?',item.strip())
    if not m:continue
    name,expr=m.groups()
    try:
     n=eval(re.sub(r'(?<=[0-9a-fA-F])[uUlL]+\b','',expr),{'__builtins__':{}},values)if expr else n+1
     if isinstance(n,int):values[name]=n
    except Exception:continue
  for name,number in re.findall(r'^\s*#define\s+(\w+)\s+(0x[0-9A-Fa-f]+|\d+)\b',text,re.M):values[name]=int(number,0)
 used=set(re.findall(r'\b[A-Z][A-Z_0-9]+\b',re.sub(r'/\*.*?\*/|//[^\n]*','',code,flags=re.S)))-{'DUNGEON_MODE','CAST_AI','NULL','AI'}
 return ''.join('constexpr uint32 '+n+'='+str(values[n])+';\n'for n in sorted(used))
if len(sys.argv)>2:
 e=next(e for e in plan if e['key']==sys.argv[2]);body=selected(e,sys.argv[3]=='baseline').replace(' override','');(out/'Selected.inc').write_text('struct Frame:Base{using Base::Base;'+body+'};',encoding='utf8');(out/'Constants.inc').write_text(constants(e,body),encoding='utf8')
else:
 parts=[]
 for e in plan:
  key=e['key']
  if key in ['Finkle','Reflection']:
   s=source(e,False);c=block(s,'class '+e['cls']+' :')+';';c=re.sub(r'(PrepareAuraScript\([^;]+\);)',r'\1\npublic:',c);parts.append('namespace N'+key+'{'+constants(e,c)+c+'}')
  elif key in ['Hadronox','Sentinel','ValithriaHealth','Firesworn','Delrissa','Council','Nefarian']:
   # Selected full handlers use controlled side-effect endpoints; native Unit health/lookup is real.
   c=selected(e,False).replace(' override','');const=constants(e,c).replace('constexpr uint32 POWER_ENERGY=3;\n','')
   if key=='Sentinel':c=c.replace('CAST_AI(aqsentinelAI, sent->AI())->GainSentinelAbility(ability);','RecordAbility(sent, ability);')
   parts.append('namespace N'+key+'{'+const+'struct Frame:NativeBase{using NativeBase::NativeBase;'+c+'};}')
 (out/'TwelfthNativeScripts.inc').write_text('\n'.join(parts),encoding='utf8')

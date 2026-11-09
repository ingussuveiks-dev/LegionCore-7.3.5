"""Selected production callback bodies with pinned pre-fix controls."""
from pathlib import Path
import json,re,subprocess,sys
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);plan=json.loads((r/'tools/tests/ThirteenthTenRepairs.json').read_text())
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
def source(e,old):return subprocess.check_output(['git','-C',str(r),'show','1dca42afd:'+e['paths'][0]]).decode('utf8')if old else(r/e['paths'][0]).read_text(encoding='utf-8-sig')
def selected(e,old):
 s=source(e,old);key=e['key'];edit=e['edits'][0];marker=edit['old']if old else edit['new']
 if key in ['Quills','Hellfire','Bounds']:
  code=block(s,marker.split('\n')[0])+');';return 'void Start(Unit*target,Creature*summon){'+code+'}'
 if key=='StyleGreeting':
  assert s.count(marker)==edit['count'];return 'void Start(Unit*target){'+marker+'}'
 if key=='StyleTimer':assert marker in s;return 'void Start(){'+marker+'}'
 scope=block(s,re.search(r'(?:class|struct)\s+'+e['cls']+r'\s*:',s).group())
 if key=='StormDragon':
  if not old:assert marker in scope
  return block(scope,'void PassengerBoarded(')if old else marker
 if key=='Hearthstation':return re.search(r'uint32 despawn_timer[^;]*;',scope).group()+block(scope,'void IsSummonedBy(')+block(scope,'void SpellFinishCast(')+block(scope,'void UpdateAI(')
 if key=='Spider':return re.search(r'uint16 check[^;]*;',scope).group()+block(scope,'void IsSummonedBy(')+block(scope,'void UpdateAI(')
 return block(scope,'void '+e['method']+'(')
def constants(e,code):
 s=source(e,False);texts=[s]+[h.read_text(encoding='utf-8-sig')for h in(r/e['paths'][0]).parent.glob('*.h')]
 texts += [(r/x).read_text(encoding='utf-8-sig',errors='replace')for x in ['src/server/game/Miscellaneous/SharedDefines.h','src/server/game/Spells/Auras/SpellAuraDefines.h','src/server/game/Entities/Object/Updates/UpdateFields.h','src/server/game/Entities/Unit/Unit.h','src/server/game/Entities/Unit/UnitDefines.h','src/server/game/Instances/InstanceScript.h','src/server/game/Entities/Object/Object.h','src/server/game/DataStores/DBCEnums.h','src/server/game/Achievements/AchievementMgr.h','src/server/game/AI/CoreAI/UnitAI.h','src/server/game/AI/ScriptedAI/ScriptedCreature.h']]
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
 used=set(re.findall(r'\b[A-Z][A-Z_0-9]+\b',re.sub(r'/\*.*?\*/|//[^\n]*','',code,flags=re.S)))-{'NULL'}
 return ''.join('constexpr uint32 '+n+'='+str(values[n])+';\n'for n in sorted(used))
if len(sys.argv)>2:
 e=next(e for e in plan if e['key']==sys.argv[2]);body=selected(e,sys.argv[3]=='baseline').replace(' override','');alias='using instance_seat_of_the_triumvirate_InstanceMapScript=Frame;'if e['key']=='SeatIntro'else''
 (out/'Selected.inc').write_text('struct Frame:Base{using Base::Base;'+alias+body+'};',encoding='utf8');(out/'Constants.inc').write_text(constants(e,body),encoding='utf8')
else:
 parts=[]
 e=plan[0];s=source(e,False);c=block(s,'class spell_failure_detection_pylon :')+';';c=re.sub(r'(PrepareSpellScript\([^;]+\);)',r'\1\npublic:',c);parts.append('namespace NPylon{'+c+'}')
 # Native admission snippets retain the actual scheduler, capture, lookup and guard;
 # payloads are captured endpoints, not actual raid casts/conversation/achievement grants.
 for e in plan[1:]:
  key=e['key'];c=selected(e,False).replace(' override','')
  if key in ['Quills','Bounds','Hellfire']:
   start=c.index('me->AddDelayedEvent(');end=c.index('return;',c.index('ObjectAccessor::Get',start))+len('return;');body=c[start:end]+'Record("'+key+'");});';parts.append('struct N'+key+':NativeBase{using NativeBase::NativeBase;void Start(Unit*target,Creature*summon){'+body+'}};')
  elif key in ['StyleGreeting','Spider']:
   parts.append('struct N'+key+':NativeBase{using NativeBase::NativeBase;'+c.replace('Talk(SAY_HELLO_RIGHT, targetGuid);','Record("StyleGreeting");').replace('DoCast(owner, 46598, true);','Record("Spider");')+'};')
  elif key=='Hearthstation':
   body=block(c,'void IsSummonedBy(').replace('DoCast(owner, 46598, true);','Record("Hearthstation");');parts.append('struct NHearthstation:NativeBase{using NativeBase::NativeBase;'+body+'};')
  elif key in ['StyleTimer','SeatIntro']:
   a=c.index('player->AddDelayedEvent(');b=c.index('return;',a)+len('return;');body=c[a:b]+'Record("'+key+'");});';parts.append('struct N'+key+':NativeBase{using NativeBase::NativeBase;void Start(Player*player){uint32 const mapId=player->GetMapId(),instanceId=player->GetInstanceId();'+body+'}};')
 (out/'ThirteenthNativeScripts.inc').write_text('\n'.join(parts),encoding='utf8')

"""Selected production bodies, pinned pre-fix counterfactuals and native admission extracts."""
from pathlib import Path
import subprocess,json,re,sys
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);plan=json.loads((r/'tools/tests/FifteenthTenRepairs.json').read_text(encoding='utf8'))
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
def source(e,old):return subprocess.check_output(['git','-C',str(r),'show','2a416922b:'+e['paths'][0]]).decode('utf8')if old else(r/e['paths'][0]).read_text(encoding='utf-8-sig')
def selected(e,old):
 s=source(e,old);k=e['key'];marker=e['edits'][0]['old'if old else'new'];assert s.count(marker)==1
 s=block(s,('struct 'if 'struct '+e['cls']in s else'class ')+e['cls'])
 if k in ['ValEntry','MawEntry']:return block(s,'void OnPlayerEnter(')if old else marker
 if k=='StyleEffects':
  if old:return 'uint64 stageEffectGeneration=0;void ResetEffects(){}void QueueStageEffect(Player*target){'+block(s,'target->AddDelayedEvent(500, [target]() -> void')+');}'
  assert s.count('QueueStageEffect(target);')==20
  assert '++stageEffectGeneration;'in block(s,'void Reset(')
  return 'uint64 stageEffectGeneration=0;void ResetEffects(){++stageEffectGeneration;}'+block(s,'void QueueStageEffect(')
 if k=='Inquisition':
  c=block(s,'other->AddDelayedEvent(100, [other,who]()'if old else'other->AddDelayedEvent(100, [other, attackerGuid')+');'
  return 'void Start(Creature*other,Unit*who){'+('if(who)'if not old else'')+c+'}'
 if k=='Nightbane':return 'bool cinematicStarted=false;'+block(s,'void sGossipSelect(')
 if k=='AvatarIntro':return block(s,'void MoveInLineOfSight(')
 if k=='Khadgar':
  c=block(s,'khadgar->AddDelayedEvent(2000, [this, khadgar]'if old else'me->AddDelayedEvent(2000, [this, khadgarGuid')+');'
  return 'void Start(Creature*khadgar){'+c+'}'
 if k=='OsoIntro':return ('uint64 introGeneration=0;'if not old else'')+block(s,'void Reset(')
 if k=='RaydVisual':
  c=block(s,'visual->AddDelayedEvent(6000, [visual, this]'if old else'visual->AddDelayedEvent(6000, [visual, spellId')+');'
  return 'void Start(Creature*visual){'+c+'}'
 assert k=='RaydRotation'
 scope=block(s,'struct boss_brawguild_raydAI');return block(scope,'void UpdateAI(')
def constants(e,code):
 s=source(e,False);texts=[s]+[p.read_text(encoding='utf-8-sig')for p in(r/e['paths'][0]).parent.glob('*.h')]
 texts +=[(r/p).read_text(encoding='utf-8-sig',errors='replace')for p in ['src/server/game/Miscellaneous/SharedDefines.h','src/server/game/Entities/Object/Updates/UpdateFields.h','src/server/game/DataStores/DBCEnums.h','src/server/game/Entities/Unit/UnitDefines.h','src/server/game/Entities/Unit/Unit.h','src/server/game/AI/ScriptedAI/ScriptedCreature.h','src/server/game/AI/CoreAI/UnitAI.h','src/server/game/Instances/InstanceScript.h']]
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
  for name,num in re.findall(r'^\s*#define\s+(\w+)\s+(0x[\da-fA-F]+|\d+)\b',text,re.M):values[name]=int(num,0)
 for extra in ['src/server/game/Quests/QuestDef.h','src/server/game/Entities/Creature/Creature.h','src/server/game/Entities/Object/Object.h','src/server/game/BrawlersGuild/BrawlersGuild.h','src/server/game/AI/EventMap.h']:
  text=(r/extra).read_text(encoding='utf-8-sig');text=re.sub(r'/\*.*?\*/|//[^\n]*','',text,flags=re.S)
  for en in re.findall(r'enum[^;{}]*\{([^{}]*)\}',text):
   n=-1
   for item in en.split(','):
    m=re.fullmatch(r'(\w+)\s*(?:=\s*(.*))?',item.strip())
    if not m:continue
    name,expr=m.groups()
    try:
     n=eval(expr,{'__builtins__':{}},values)if expr else n+1
     if isinstance(n,int):values[name]=n
    except Exception:continue
 used=set(re.findall(r'\b[A-Z][A-Z_0-9]+\b',re.sub(r'//[^\n]*','',code)))-{'NULL','AI'}
 return ''.join('constexpr uint32 '+n+'='+str(values[n])+';\n'for n in sorted(used))
if len(sys.argv)>2:
 e=next(e for e in plan if e['key']==sys.argv[2]);c=selected(e,sys.argv[3]=='baseline').replace(' override','')
 # The real two paired catalogs, not synthetic test spell IDs.
 arrays=''
 if e['key']=='RaydRotation':
  s=source(e,False)
  for name in ['Spells_Rayd','Spells_Morph']:arrays+=block(s,'uint32 '+name+'[10]')+';'
 (out/'Constants.inc').write_bytes(constants(e,c+arrays).encode());(out/'Selected.inc').write_bytes((arrays+'struct Frame:Base{using Base::Base;'+c+'};').encode())
else:
 parts=[]
 for e in plan:
  k=e['key'];c=selected(e,False).replace(' override','')
  if k in ['ValEntry','MawEntry']:
   a=c.index('auto attempt =');b=c.index('if (player->GetQuestStatus',a)if k=='ValEntry'else c.index('if (GetBossState',a)
   setup=c[a:b];guard='if (weakAttempt.expired() || !player->IsInWorld() || player->GetMapId() != mapId || player->GetInstanceId() != instanceId)'
   assert guard in c
   c='std::map<ObjectGuid,std::shared_ptr<uint8>>entryAttempts;'+block(c,'void OnPlayerLeave(')+'void OnPlayerEnter(Player*player){if(!player)return;'+setup+'player->AddDelayedEvent(500,[player,mapId,instanceId,weakAttempt]{'+guard+'return;Record("'+k+'");});}'
  elif k=='StyleEffects':c=c.replace('if (target->HasAura(261939))\n                    target->CastSpell(target, SPELL_STAGE_FIRE, true);\n                else if (target->HasAura(261938))\n                    target->CastSpell(target, SPELL_STAGE_SPOTLIGHT, true);','Record("StyleEffects");')
  elif k=='Inquisition':c=c.replace('other->AI()->EnterCombat(attacker);','Record("Inquisition");')
  elif k=='Khadgar':
   a=c.index('me->AddDelayedEvent(');b=c.index('return;',a)+len('return;');c='void Start(Creature*khadgar){'+c[a:b]+'Record("Khadgar");});}'
  elif k=='Nightbane':
   # Complete delayed75+1s source slice; full gossip/admission/failure is source-tested.
   a=c.index('me->AddDelayedEvent(75000,');c='void Start(Creature*nightbane){'+block(c[a:],'me->AddDelayedEvent(75000,')+');}'
   c=c.replace('me->CastSpell(nightbane, 30971);','Record("Nightbane75");').replace('nightbane->CastSpell(nightbane, 30969);','')
   c=c.replace('nightbane->CastSpell(me, 30971);','Record("Nightbane76");').replace('me->CastSpell(me, 30973);','').replace('nightbane->CastSpell(me, 30930);','').replace('me->CastSpell(me, 15041);','')
  elif k=='AvatarIntro':
   a=c.index('kiljedan->AddDelayedEvent(27000,');c='void Start(Creature*kiljedan){'+block(c[a:],'kiljedan->AddDelayedEvent(27000,')+');}'
   c=c.replace('kiljedan->CastSpell(avatar, 240897);','Record("Avatar27");').replace('avatar->SetAnimKitId(12758);','Record("Avatar30");').replace('avatar->AI()->DoCast(SPELL_VISUAL_SWORD);','Record("Avatar40");').replace('kiljedan->CastSpell(kiljedan, 240923);','Record("KiljaedenCleanup40");')
  elif k=='OsoIntro':
   a=c.index('me->AddDelayedEvent(6000,');guardend=c.index('if (Creature* morozec',a)
   c='uint64 introGeneration=0;void Start(Creature*morozec){'+c[a:guardend]+'if(ObjectAccessor::GetCreature(*me,morozecGuid))Record("OsoHelper");Record("OsoReady");});}'
  elif k=='RaydVisual':c=c.replace('visual->RemoveAura(spellId);','Record(std::to_string(spellId).c_str());')
  else:continue
  parts.append('struct N'+k+':NativeBase{using NativeBase::NativeBase;'+c+'};')
 (out/'FifteenthNativeScripts.inc').write_bytes('\n'.join(parts).encode())

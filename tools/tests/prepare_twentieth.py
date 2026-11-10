"""Selected production state/cleanup/boarding/surrender bodies with pinned controls."""
from pathlib import Path
import sys,json,subprocess,re,ast
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);plan=json.loads((r/'tools/tests/TwentiethTenRepairs.json').read_text());base='f38d91737'
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
def source(e,old):return subprocess.check_output(['git','-C',str(r),'show',base+':'+e['paths'][0]]).decode('utf8')if old else(r/e['paths'][0]).read_text(encoding='utf-8-sig')
def selected(e,old):
 full=source(e,old);k=e['key'];s=block(full,('struct 'if 'struct '+e['cls']in full else'class ')+e['cls'])
 if k=='NefarianTimers':
  a=s.index('if (phase == PHASE_GROUND && m_uiDistanceCheckTimer'if old else'if (phase == PHASE_GROUND)\n            {\n                if (m_uiDistanceCheckTimer');b=s.index('events.Update(diff);',a);return 'void Run(uint32 diff){'+s[a:b]+'}'
 if k=='AnnHyldeTimers':
  reset=block(s,'void Reset(');return 'void Reset(){'+('uiResurectTimer = 0;uiResurectPhase = 0;'if'uiResurectTimer = 0;'in reset else'')+'}'
 if k=='TrollReturn':return block(s,'void UpdateAI(')
 if k=='BalloonBoard':
  result=[]
  for n,cls in enumerate(['npc_chens','npc_scgnottz','npc_lin_cloudwalker','npc_rungle','npc_zang_cloudwalker','npc_hemet_nesingwary','npc_emi_lan']):
   provider=block(full,'struct '+cls+' :');a=provider.index('me->SummonCreature('if old else'Creature* balloon = me->SummonCreature(');b=provider.index(';',a)+1;summon=provider[a:b]
   if not old:summon+=block(provider,'if (!balloon)')
   callback=block(provider,'g_player->AddDelayedEvent(i * 400 + 1,')+');';summon=summon.replace('me->SummonCreature(','SpawnBalloon(')
   result.append('void Start'+str(n)+'(Player*g_player,uint8 i=1){'+summon+callback+'}')
  return ''.join(result)
 if k in ['ShannoxPrison','MaloriakFreeze','SindragosaTomb','HagaraTomb']:
  return ('bool released=false;'if not old else'')+block(s,'void SetGUID(')+block(s,'void JustDied(')+block(s,'void UpdateAI(')
 if k=='MedivhSequence':
  reset=block(s,'void Reset(');state='EventStarted = false;Step = YellTimer = FireMedivhTimer = FireArcanagosTimer = 0;'if'Step = YellTimer ='in reset else'';abort=block(s,'void AbortEvent(')if not old else'void AbortEvent(){}'
  pre=block(s,'uint32 NextStep(');pre=pre[:pre.index('switch (Step)')]+'return 0;}'
  return 'void ResetState(){'+state+'}'+abort+block(s,'void StartEvent(')+pre
 if k=='ChampionSurrender':return ('bool surrendered=false;'if not old else'')+block(s,'void DamageTaken('if old else'void DamageTakenAfterScaling(')
 raise ValueError(k)
tree=ast.parse((r/'tools/tests/prepare_fifteenth.py').read_text());defs=[x for x in tree.body if isinstance(x,ast.FunctionDef)and x.name=='constants'];ns=dict(r=r,source=source,re=re);exec(compile(ast.Module(body=defs,type_ignores=[]),'enum-reader','exec'),ns);constants=ns['constants']
def arrays(e):
 if e['key']!='MedivhSequence':return ''
 s=source(e,False);return ''.join(re.search(r'(?:float|static float) '+name+r'\[[^;]+;',s).group()for name in ['MedivPos','ArcanagosPos'])
def extra(e,c,native=False):
 supplemental=''
 if 'TYPEID_PLAYER'in c:
  header=(r/'src/server/game/Entities/Object/ObjectGuid.h').read_text(encoding='utf-8-sig');value=re.search(r'TYPEID_PLAYER\s*=\s*(\d+)',header).group(1);c=c.replace('TYPEID_PLAYER',value);supplemental='constexpr uint32 TYPEID_PLAYER='+value+';\n'
 con=constants(e,c)+supplemental
 if native:con=re.sub(r'constexpr uint32 (?:REACT_\w+|GO_STATE_\w+)=\d+;\n','',con)
 return con+arrays(e)
if len(sys.argv)>2:
 e=next(e for e in plan if e['key']==sys.argv[2]);c=selected(e,sys.argv[3]=='baseline').replace(' override','');(out/'Constants.inc').write_bytes(extra(e,c).encode());(out/'Selected.inc').write_bytes(('struct Frame:Base{using Base::Base;'+c+'};').encode())
else:
 parts=[]
 for e in plan:
  c=selected(e,False).replace(' override','')
  if e['key']=='BalloonBoard':
   c=c.replace('g_player->HasAura(SPELL_WAINTING_FOR_BALLOON)','Waiting(g_player)');c=re.sub(r'g_player->CastSpell\(balloon, (\d+), true\);',r'RecordBoard(g_player,balloon,\1);',c)
  if e['key']=='MedivhSequence':c=c.replace('me->SummonCreature(','SpawnBalloon(')
  if e['key']=='TrollReturn':c=c.replace('me->GetMotionMaster()->MoveTargetedHome();','RecordHome();')
  if e['key']=='ChampionSurrender':c=c.replace('DoScriptText(NPC_ARGENT_CHAMPION_TEXT_SAY_WIN, me);','ObserveSpeech();').replace('player->RewardPlayerAndGroupAtEvent(NPC_ARGENT_CHAMPION_CREDIT, me);','RecordChampion(player, NPC_ARGENT_CHAMPION_CREDIT);')
  parts.append('namespace '+e['key']+'{'+extra(e,c,True)+'struct Frame:NativeBase{using NativeBase::NativeBase;'+c+'};}')
 (out/'TwentiethNativeScripts.inc').write_bytes(('namespace TwentiethNative{'+''.join(parts)+'}').encode())

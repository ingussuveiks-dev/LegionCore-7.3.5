"""Selected production bodies and pinned controls for ten independent encounter fixes."""
from pathlib import Path
import sys,json,subprocess,re,ast
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);plan=json.loads((r/'tools/tests/TwentySecondTenRepairs.json').read_text(encoding='utf8'));base='4ce09b5362699afd09f6e800134150668a55772e'
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
def source(e,old):return subprocess.check_output(['git','-C',str(r),'show',base+':'+e['paths'][0]]).decode('utf8')if old else(r/e['paths'][0]).read_text(encoding='utf-8-sig')
def selected(e,old,cls):
 full=source(e,old);marker=re.search(r'(?:struct|class)\s+'+re.escape(cls)+r'\s*:',full).group();s=block(full,marker);k=e['key'];c=block(s,'void DamageTaken('if old else'void DamageTakenAfterScaling(')
 if k=='KoraghAnomaly':c='bool m_Exploded=false;'+c
 if k=='EnergizedPhoenix':
  c=('bool sleeping=false;'if not old else'')+c+block(s,'void Reset(')+block(s,'void IsSummonedBy(')+block(s,'void UpdateAI(')
 if k=='ArtruisRelease':
  assert 'TryReleaseAdds();'in block(s,'void UpdateAI(')if not old else True
  c=('bool releasePending=false;'+block(s,'void TryReleaseAdds(')if not old else'void TryReleaseAdds(){}')+c+block(s,'void Reset(')+block(s,'void DoAction(')
  # Keep the actual production UpdateAI preamble,including the shielded retry.
  u=block(s,'void UpdateAI(');c+=u[:u.index('events.Update(diff);')]+'}'
 if k in ['BrokenDuel']:
  u=block(s,'void UpdateAI(');c+='void Tick(){'+(u[u.index('{')+1:u.index('if (!UpdateVictim())')]if not old else'')+'}'
 if k=='WhitemaneResurrection'and cls.startswith('boss_'):c+=block(s,'void MovementInform(')
 return c
tree=ast.parse((r/'tools/tests/prepare_fifteenth.py').read_text(encoding='utf8'));defs=[x for x in tree.body if isinstance(x,ast.FunctionDef)and x.name=='constants'];ns=dict(r=r,source=source,re=re);exec(compile(ast.Module(body=defs,type_ignores=[]),'enum-reader','exec'),ns);constants=ns['constants']
def extra(e,c,native=False):
 follow=re.search(r'#define PET_FOLLOW_DIST\s+([\d.]+f)',(r/'src/server/game/Entities/Pet/PetDefines.h').read_text(encoding='utf8')).group(1)
 motion=re.search(r'\bPOINT_MOTION_TYPE\s*=\s*(\d+)',(r/'src/server/game/Movement/MotionMaster.h').read_text(encoding='utf8')).group(1)
 con=constants(e,(c+(' PHASE_FLIGHT'if e['key']=='FelmystFlight'else'')).replace('PET_FOLLOW_DIST',follow).replace('POINT_MOTION_TYPE',motion))
 if 'POINT_MOTION_TYPE'in c and not native:con+='constexpr uint32 POINT_MOTION_TYPE='+motion+';\n'
 if 'PET_FOLLOW_DIST'in c and not native:con+='constexpr float PET_FOLLOW_DIST='+follow+';\n'
 if e['key']=='KoraghAnomaly':
  con+=''.join('constexpr uint32 '+name+'='+re.search(r'\b'+name+r'\s*=\s*(\d+)',source(e,False)).group(1)+';\n'for name in ['AlphaFadeOut','Destabilize','SuppressionFieldSilence'])
 if native:con=re.sub(r'constexpr uint32 (?:REACT_\w+|UNIT_\w+|SPELL_CAST_OK|DONE|POINT_MOTION_TYPE)=\d+;\n','',con)
 return con
def native_body(c):
 c=c.replace('player->KilledMonsterCredit(108722);','RecordCredit(player,108722);').replace('me->CastSpell(','ObserveCast(me,').replace('me->DespawnOrUnsummon(','ObserveDespawn(').replace('me->FindNearestCreature(','FindActor(').replace('GetCreatureListWithEntryInGrid(','ObservedCreatures(').replace('GetPlayerListInGrid(','ObservedPlayers(').replace('me->GetMotionMaster()->MoveFollow(','ObserveFollow(').replace('me->GetMotionMaster()->MovePoint(','ObserveMove(').replace('me->GetMotionMaster()->MoveTargetedHome();','ObserveHome();').replace('me->GetMotionMaster()->MovementExpired(true);','ObserveStopMove();')
 return c
if len(sys.argv)>2:
 e=next(e for e in plan if e['key']==sys.argv[2]);old=sys.argv[3]=='baseline';parts=[];con=''
 for i,cls in enumerate(e['classes']):
  c=selected(e,old,cls).replace(' override','');con+=extra(e,c);parts.append('struct Frame'+str(i)+':Base{using Base::Base;'+c+'};')
 con='\n'.join(dict.fromkeys(con.splitlines()))+'\n';(out/'Constants.inc').write_bytes(con.encode('utf8'));(out/'Selected.inc').write_bytes((''.join(parts)+'using Frame=Frame0;').encode('utf8'))
else:
 parts=[]
 for e in plan:
  frames=[];con=''
  for i,cls in enumerate(e['classes']):
   c=native_body(selected(e,False,cls).replace(' override',''));con+=extra(e,c,True);frames.append('struct Frame'+str(i)+':NativeBase{using NativeBase::NativeBase;'+c+'};')
  parts.append('namespace '+e['key']+'{'+ '\n'.join(dict.fromkeys(con.splitlines()))+'\n'+''.join(frames)+'}')
 (out/'TwentySecondNativeScripts.inc').write_bytes(('namespace TwentySecondNative{'+''.join(parts)+'}').encode('utf8'))

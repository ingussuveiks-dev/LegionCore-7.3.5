"""Extract selected production handlers; pin expected-broken controls to38ab2461f."""
from pathlib import Path
import sys,json,subprocess,re,ast
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);plan=json.loads((r/'tools/tests/TwentyFirstTenRepairs.json').read_text(encoding='utf8'));base='38ab2461f'
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
def source(e,old):return subprocess.check_output(['git','-C',str(r),'show',base+':'+e['paths'][0]]).decode('utf8')if old else(r/e['paths'][0]).read_text(encoding='utf-8-sig')
def selected(e,old):
 full=source(e,old);k=e['key'];s=block(full,('class 'if k in ['AntorusIntro','CityValiant']else'struct ')+e['cls'])
 if k=='AntorusIntro':return ('bool eventDone=false;'if old else'')+block(s,'bool OnTrigger(')
 if k=='TsulongEvade':
  reset=block(s,'void Reset(');assert '_Reset();'in reset
  return 'void Reset(){'+('_evadeCheck = false;'if '_evadeCheck = false;'in reset else'')+'}'
 hook=block(s,'void DamageTaken('if old else'void DamageTakenAfterScaling(')
 if k=='CityValiant':
  ai=block(s,'struct npc_valiantAI');return ('bool defeated=false;ObjectGuid challenger;'if not old else'')+hook+block(ai,'void Reset(')+block(ai,'void KilledUnit(')+block(ai,'void DoAction('if old else'bool StartChallenge(')
 if k=='ArgentValiant':return ('bool defeated=false;'if not old else'')+hook
 if k=='ChengChallenge':return 'bool challengeDone=false;'+hook
 if k in ['ChiJi','Xuen','Yulon','Niuzao']:return hook+block(s,'void QuestCredit(')
 return hook
tree=ast.parse((r/'tools/tests/prepare_fifteenth.py').read_text(encoding='utf8'));defs=[x for x in tree.body if isinstance(x,ast.FunctionDef)and x.name=='constants'];ns=dict(r=r,source=source,re=re);exec(compile(ast.Module(body=defs,type_ignores=[]),'enum-reader','exec'),ns);constants=ns['constants']
def extra(e,c,native=False):
 native_type=re.search(r'TYPEID_PLAYER\s*=\s*(\d+)',(r/'src/server/game/Entities/Object/ObjectGuid.h').read_text(encoding='utf8')).group(1)
 c=c.replace('TYPEID_PLAYER',native_type)
 con=constants(e,c)
 if native:con=re.sub(r'constexpr uint32 (?:REACT_\w+|GO_STATE_\w+|UNIT_\w+|SPELL_CAST_OK|DONE)=\d+;\n','',con)
 return con+('constexpr uint32 TYPEID_PLAYER='+native_type+';\n'if 'TYPEID_PLAYER'in selected(e,False)+selected(e,True)and not native else'')
def header():
 s=(r/'src/server/scripts/Legion/AntorusTheBurningThrone/AntorusIntro.h').read_text(encoding='utf8');return block(s,'class AntorusIntroInstance')+';'
if len(sys.argv)>2:
 e=next(e for e in plan if e['key']==sys.argv[2]);old=sys.argv[3]=='baseline';c=selected(e,old).replace(' override','');(out/'Constants.inc').write_bytes(extra(e,c).encode());(out/'Header.inc').write_bytes((header()if e['key']=='AntorusIntro'and not old else'').encode());(out/'Selected.inc').write_bytes(('struct Frame:Base{using Base::Base;'+c+'};').encode())
else:
 parts=[]
 for e in plan[1:]:
  c=selected(e,False).replace(' override','')
  c=re.sub(r'DoScriptText\([^;]+;', 'ObserveSpeech();',c)
  c=c.replace('player->RewardPlayerAndGroupAtEvent(NPC_ARGENT_VALIANT_CREDIT, me);','RecordCredit(player);').replace('player->KilledMonsterCredit(me->GetEntry());','RecordCredit(player);').replace('player->CastSpell(player, 62724, true);','RecordCredit(player);').replace('me->GeneratePersonalLoot(me, looter);','RecordLoot(looter);').replace('Shao->GetAI()->DoAction(', 'RecordParent(')
  # Selected state transitions; native health/owner/GUID/threat/lookups stay real.
  parts.append('namespace '+e['key']+'{'+extra(e,c,True)+'struct Frame:NativeBase{using NativeBase::NativeBase;'+c+'};}')
 (out/'TwentyFirstNativeScripts.inc').write_bytes(('namespace TwentyFirstNative{'+''.join(parts)+'}').encode())

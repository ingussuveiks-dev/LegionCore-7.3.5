"""Actual selected timer bodies and delayed callback trees, with pinned controls."""
from pathlib import Path
import sys,json,subprocess,re,ast
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);plan=json.loads((r/'tools/tests/NineteenthTenRepairs.json').read_text());base='f4a931757'
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
def source(e,old):return subprocess.check_output(['git','-C',str(r),'show',base+':'+e['paths'][0]]).decode('utf8')if old else(r/e['paths'][0]).read_text(encoding='utf-8-sig')
def selected(e,old):
 s=source(e,old);k=e['key'];cls=e['cls']
 if k=='YmironPrisoner':cls='npc_ymiron_seacursed_slaver'
 s=block(s,('struct 'if 'struct '+cls in s else'class ')+cls)
 if k=='InfernoPause':return block(s,'void UpdateAI(')
 if k=='HodirTimer':return ('uint32 pauseTimer=0xCDCDCDCD;'if old else'uint32 pauseTimer = 60000;')+block(s,'void UpdateAI(')
 if k=='YmironWalk':
  reset=block(s,'void Reset(');return 'uint32 m_bIsWalking=0xCDCDCDCD;void Reset(){'+('m_bIsWalking = false;'if 'm_bIsWalking = false;'in reset else'')+'}'
 if k=='VetirRide':
  a='player->AddDelayedEvent(1000, [player]'if old else'player->AddDelayedEvent(1000, [player, mapId';return block(s,'void PassengerBoarded(')+block(s,'void UpdateAI(')+'void Exit(Player*player){'+block(s,a)+');}'
 if k=='BrokenIntro':return 'void Start(Player*player){'+block(s,'player->AddDelayedEvent(5000,')+');}'
 if k=='AssaultCallbacks':
  chunks=[]
  for marker in ['player->CreateConversation(4831);','player->CreateConversation(4832);','player->CreateConversation(4892);','player->CastSpell(player, 243824);']:
   at=s.index(marker);a=s.rfind('player->AddDelayedEvent(',0,at);chunks.append(block(s,s[a:s.index('\n',a)])+');')
  return 'void Start(Player*player){'+''.join(chunks)+'}'
 if k=='OdynCombat':return 'void Start(Player*player){'+block(s,'player->AddDelayedEvent(100,')+');}'
 if k=='YmironPrisoner':return 'void Start(Creature*prisoner){'+block(s,'AddDelayedEvent(2000,')+');}'
 if k=='OperaAssistant':return 'void Start(Creature*decor,Position pos){'+block(s,'AddDelayedEvent(2000, [this, decor')+');}'
 if k=='OperaBrute':return 'void Start(Creature*bella,Creature*brute){'+block(s,'bella->AddDelayedEvent(19000, [bella, brute')+');}'
 raise ValueError(k)
tree=ast.parse((r/'tools/tests/prepare_fifteenth.py').read_text());defs=[x for x in tree.body if isinstance(x,ast.FunctionDef)and x.name=='constants'];ns=dict(r=r,source=source,re=re);exec(compile(ast.Module(body=defs,type_ignores=[]),'enum-reader','exec'),ns);constants=ns['constants']
def const(e,c):
 extra='';c=c.replace('TELE_TO_SEAMLESS','0')
 if 'TELE_TO_SEAMLESS'in selected(e,False):extra='constexpr uint32 TELE_TO_SEAMLESS=0x20;\n'
 return constants(e,c)+extra
if len(sys.argv)>2:
 e=next(e for e in plan if e['key']==sys.argv[2]);c=selected(e,sys.argv[3]=='baseline').replace(' override','');(out/'Constants.inc').write_bytes(const(e,c).encode());(out/'Selected.inc').write_bytes(('struct Frame:Base{using Base::Base;'+c+'};').encode())
else:
 parts=[]
 for e in plan:
  c=selected(e,False).replace(' override','');parts.append('namespace '+e['key']+'{'+re.sub(r'constexpr uint32 (?:REACT_AGGRESSIVE|GO_STATE_ACTIVE)=\d+;\n','',const(e,c))+'struct Frame:NativeBase{using NativeBase::NativeBase;'+c+'};}')
 (out/'NineteenthNativeScripts.inc').write_bytes(('namespace NineteenthNative{'+''.join(parts).replace('npc_escortAI::UpdateAI','ObservedEscort19::UpdateAI')+'}').encode())

"""Selected health/controller source with a pinned, instrumented pre-fix control."""
from pathlib import Path
import subprocess,json,sys,ast
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);plan=json.loads((r/'tools/tests/SixteenthTenRepairs.json').read_text())
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
def source(e,old):return subprocess.check_output(['git','-C',str(r),'show','f55f0ea70:'+e['paths'][0]]).decode('utf8')if old else(r/e['paths'][0]).read_text(encoding='utf-8-sig')
# Reuse the reviewed native enum reader only; do not execute another producer.
tree=ast.parse((r/'tools/tests/prepare_fifteenth.py').read_text());defs=[x for x in tree.body if isinstance(x,ast.FunctionDef)and x.name=='constants'];exec(compile(ast.Module(body=defs,type_ignores=[]),'enum-reader','exec'),globals()|{'re':__import__('re')})
import re
ns=dict(r=r,source=source,re=re);exec(compile(ast.Module(body=defs,type_ignores=[]),'enum-reader','exec'),ns);constants=ns['constants']
def selected(e,old):
 s=source(e,old);k=e['key'];edit=e['edits'][0];marker=edit['old'if old else'new'];assert s.count(marker)==edit['count']
 if k in ['MimironHealth','CryptHealth','SquadHealth']:
  if old and k in ['CryptHealth','SquadHealth']:marker=marker.replace('uint32(','CounterfactualNarrow32(')
  return 'void Run(Creature*creature){'+marker+'}'
 scope=block(s,('struct 'if 'struct '+e['cls']in s else'class ')+e['cls'])
 if k=='HatchlingTimer':return 'uint32 checking=0,SpellVisualCast=0xCDCDCDCD;'+block(scope,'void Reset(')
 if k=='AnubBurrow':
  a=scope.index('if (!_burrowGUID.empty())')if not old else scope.index('GuidList::iterator i = _burrowGUID.begin();')
  b=scope.index('events.ScheduleEvent(EVENT_SUMMON_SCARAB',a);end=scope.index(';',b)+1
  return 'GuidList _burrowGUID;void Run(){'+scope[a:end]+'}'
 if k in ['ArmyExit','EonarDive']:
  c=block(scope,marker.split('\n')[0])+');';param='Player*pplayer'if k=='ArmyExit'else'Unit*caster'
  return 'void Start('+param+'){'+c+'}'
 return block(scope,'void '+e['method']+'(')
if len(sys.argv)>2:
 e=next(x for x in plan if x['key']==sys.argv[2]);c=selected(e,sys.argv[3]=='baseline').replace(' override','')
 # Inline macros remain in the selected pre-fix body; avoid duplicate declarations.
 const=constants(e,c.replace('#define HEAL_BROTHER_AMOUNT 30000.0f','').replace('HEAL_BROTHER_AMOUNT','30000.0f').replace('IN_MILLISECONDS','1000').replace('TELE_TO_SEAMLESS','0'))
 if 'IN_MILLISECONDS'in c:const+='constexpr uint32 IN_MILLISECONDS=1000;\n'
 if 'TELE_TO_SEAMLESS'in c:
  header=(r/'src/server/game/Entities/Player/Player.h').read_text(encoding='utf-8-sig');value=re.search(r'TELE_TO_SEAMLESS\s*=\s*(0x[\da-fA-F]+|\d+)',header).group(1);const+='constexpr uint32 TELE_TO_SEAMLESS='+value+';\n'
 (out/'Constants.inc').write_bytes(const.encode());(out/'Selected.inc').write_bytes(('struct Frame:Base{using Base::Base;'+c+'};').encode())
else:
 parts=[]
 for e in plan:
  k=e['key'];c=selected(e,False).replace(' override','')
  if k=='HatchlingTimer':c=c.replace('uint32 checking=0,SpellVisualCast=0xCDCDCDCD;','uint32 checking=0,SpellVisualCast=0;')
  elif k in ['ArmyExit','EonarDive']:
   if k=='ArmyExit':c=c.replace('pplayer->TeleportTo(1626, 2085.09f, 5399.86f, 47.33f, 6.04f, TELE_TO_SEAMLESS);','Record("ArmyExit");')
   else:c=c.replace('caster->CastSpellDuration(caster, SPELL_SWIFT_FEET, true, 6000, 3);','Record("EonarDive");')
  elif k=='AnubBurrow':c=c.replace('pBurrow->CastSpell(pBurrow, 66340, false);','Record("AnubBurrow");')
  parts.append('struct N'+k+':NativeBase{using NativeBase::NativeBase;'+c+'};')
 (out/'SixteenthNativeScripts.inc').write_bytes(('namespace SixteenthNative{constexpr uint32 TYPE_PHASE=0,SPELL_HEAL_BROTHER=7393,EVENT_SUMMON_SCARAB=8,PHASE_SUBMERGED=2;\n'+'\n'.join(parts)+'\n}\n').encode())
 e=plan[0];s=source(e,False);(out/'SixteenthHatchling.inc').write_bytes(('namespace SixteenthHatchling{'+block(s,'struct npc_moonkin_hatchling_pet :')+';}').encode())

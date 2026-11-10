"""Extract ten production lethal transitions and pulse/queue helpers with pinned controls."""
from pathlib import Path
import ast,json,sys,subprocess,re
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);plan=json.loads((r/'tools/tests/TwentyThirdTenRepairs.json').read_text(encoding='utf8'));base='f28383efd341c8481a566a23777d75fe0cfa0a8f'
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
def source(e,old):return subprocess.check_output(['git','-C',str(r),'show',base+':'+e['paths'][0]]).decode('utf8')if old else(r/e['paths'][0]).read_text(encoding='utf-8-sig')
def selected(e,old):
 full=source(e,old);marker=re.search(r'(?:struct|class)\s+'+re.escape(e['cls'])+r'\s*:',full).group();s=block(full,marker);c=block(s,'void DamageTaken('if old else'void DamageTakenAfterScaling(')
 if e['key']=='ArgusPulse':c+=block(s,'void ReoriginationPulse(')+block(s,'void SpellFinishCast(')+block(s,'void OnApplyOrRemoveAura(')
 if e['key']=='HyrjaAscendance':c+=block(s,'void Reset(')+block(s,'void SpiritForm(')
 return c
tree=ast.parse((r/'tools/tests/prepare_fifteenth.py').read_text(encoding='utf8'));ns=dict(r=r,source=source,re=re);exec(compile(ast.Module(body=[x for x in tree.body if isinstance(x,ast.FunctionDef)and x.name=='constants'],type_ignores=[]),'enum-reader','exec'),ns);constants=ns['constants']
def extra(e,c,native=False):
 # AuraRemoveMode is defined outside the original enum reader's header set.
 value=re.search(r'\bAURA_REMOVE_BY_EXPIRE\s*=\s*(\d+)',(r/'src/server/game/Entities/Unit/Unit.h').read_text(encoding='utf8')).group(1)if'AURA_REMOVE_BY_EXPIRE'in c else None
 con=constants(e,c.replace('AURA_REMOVE_BY_EXPIRE',value or'0'))
 if value and not native:con+='constexpr uint32 AURA_REMOVE_BY_EXPIRE='+value+';\n'
 if native:con=re.sub(r'constexpr uint32 (?:REACT_\w+|UNIT_\w+|TEMPSUMMON_\w+|DONE|AURA_REMOVE_\w+)=\d+;\n','',con)
 if e['key']=='ArgusPulse':con+=re.search(r'Position const centrPos\s*=\s*\{[^;]+;',source(e,False)).group()+'\n'
 return con
def native_body(e,c):
 c=c.replace('me->CastSpell(','ObserveCast(me,').replace('me->DespawnOrUnsummon(','ObserveDespawn(').replace('residue->DespawnOrUnsummon();','ObserveResidueDespawn(residue);').replace('Algalon->SummonCreature(','ObserveSpawn(Algalon,').replace('owner->SummonCreature(','ObserveSpawn(owner,').replace('me->GetMotionMaster()->Initialize();','ObserveMotionReset();').replace('module->GetAreaObject(','ObservedArea(module,').replace('me->GetAreaObject(','ObservedArea(me,')
 if e['key']=='FesterfaceGoo':c=c.replace('me->HasAura(SPELL_RECONGEALING)','HasObservedAura(SPELL_RECONGEALING)')
 return c
if len(sys.argv)>2:
 e=next(e for e in plan if e['key']==sys.argv[2]);c=selected(e,sys.argv[3]=='baseline').replace(' override','');(out/'Constants.inc').write_bytes(extra(e,c).encode('utf8'));(out/'Selected.inc').write_bytes(('struct Frame:Base{using Base::Base;'+c+'};').encode('utf8'))
else:
 parts=[]
 for e in plan:
  c=native_body(e,selected(e,False).replace(' override',''));parts.append('namespace '+e['key']+'{'+extra(e,c,True)+'struct Frame:NativeBase{using NativeBase::NativeBase;'+c+'};}')
 (out/'TwentyThirdNativeScripts.inc').write_bytes(('namespace TwentyThirdNative{'+''.join(parts)+'}').encode('utf8'))

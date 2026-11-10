"""Extract the ten bound encounter transitions; keep pinned pre-fix controls."""
from pathlib import Path
import ast,json,re,subprocess,sys
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
plan=json.loads((r/'tools/tests/TwentyFourthTenRepairs.json').read_text(encoding='utf8'));base='49890c95fd21b88c46501617b0278277bede2f7f'
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
def source(e,old):return subprocess.check_output(['git','-C',str(r),'show',base+':'+e['paths'][0]]).decode('utf8')if old else(r/e['paths'][0]).read_text(encoding='utf-8-sig')
def selected(e,old,cls):
 s=source(e,old);marker=re.search(r'(?:struct|class)\s+'+re.escape(cls)+r'\s*:',s).group();s=block(s,marker)
 return block(s,'void DamageTaken('if old else'void DamageTakenAfterScaling(').replace(' override','')
tree=ast.parse((r/'tools/tests/prepare_fifteenth.py').read_text(encoding='utf8'));ns=dict(r=r,source=source,re=re);exec(compile(ast.Module(body=[x for x in tree.body if isinstance(x,ast.FunctionDef)and x.name=='constants'],type_ignores=[]),'enum-reader','exec'),ns);constants=ns['constants']
def emitted(e,old,native):
 codes=[selected(e,old,cls)for cls in e['classes']];code=''.join(codes)+' UNIT_FIELD_FLAGS'+(' PHASE_UNDEAD'if e['key']=='BlackKnightResurrection'else'');con=constants(e,code.replace('IN_MILLISECONDS','1000').replace('UNIT_FIELD_BYTES_1','UNIT_FIELD_ANIM_TIER'))
 if 'UNIT_FIELD_BYTES_1'in code and not native:con+='constexpr uint32 UNIT_FIELD_BYTES_1=UNIT_FIELD_ANIM_TIER;\n'
 if 'IN_MILLISECONDS'in code:
  value=re.search(r'IN_MILLISECONDS\s*=\s*(\d+)',(r/'src/common/Common.h').read_text(encoding='utf8')).group(1)
  if not native:con+='constexpr uint32 IN_MILLISECONDS='+value+';\n'
 if native:
  con=re.sub(r'constexpr uint32 (?:REACT_\w+|UNIT_\w+|MOVEMENTFLAG_\w+|CRITERIA_TYPE_\w+|ENCOUNTER_FRAME_\w+|MECHANIC_\w+|IMMUNITY_\w+)=\d+;\n','',con)
 def translate(c):
  if not native:return c
  return c.replace('me->CastSpell(','ObserveCast(me,').replace('owner->SummonCreature(','ObserveSpawn(owner,').replace('me->DespawnOrUnsummon(','ObserveDespawn(').replace('me->GetMotionMaster()->MovePoint(1, me->GetHomePosition());','ObserveHome();').replace('me->SetUnitMovementFlags(MOVEMENTFLAG_WALKING);','ObserveWalk();').replace('me->FindNearestCreature(','ObservedNearest(').replace('me->GetInstanceScript()','ObserveInstance()').replace('BindPlayersToInstance(me);','ObserveBind();').replace('InstanceScript* instance','auto instance')
 return con,''.join('struct '+('Frame'if i==0 else'OtherFrame')+':Base{using Base::Base;'+translate(c)+'};'for i,c in enumerate(codes))
if len(sys.argv)>2:
 e=next(e for e in plan if e['key']==sys.argv[2]);con,c=emitted(e,sys.argv[3]=='baseline',False);(out/'Constants.inc').write_bytes(con.encode('utf8'));(out/'Selected.inc').write_bytes(c.encode('utf8'))
else:
 parts=[]
 for e in plan:
  con,c=emitted(e,False,True);parts.append('namespace '+e['key']+'{'+con+c+'}')
 (out/'TwentyFourthNativeScripts.inc').write_bytes(('namespace TwentyFourthNative{'+''.join(parts)+'}').encode('utf8'))

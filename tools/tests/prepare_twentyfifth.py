"""Real production death observations/transitions with pinned raw-hook controls."""
from pathlib import Path
import ast,json,re,subprocess,sys
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
plan=json.loads((r/'tools/tests/TwentyFifthTenRepairs.json').read_text(encoding='utf8'));base='b7cb7fd2b251ce7114c1b29f09bbf1e33c32141f'
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
def source(e,old):return subprocess.check_output(['git','-C',str(r),'show',base+':'+e['paths'][0]]).decode('utf8')if old else(r/e['paths'][0]).read_text(encoding='utf-8-sig')
def selected(e,old,cls):
 s=source(e,old);marker=re.search(r'(?:struct|class)\s+'+re.escape(cls)+r'\s*:',s).group();s=block(s,marker)
 c=block(s,'void DamageTaken('if old else'void DamageTakenAfterScaling(')
 if e['key']=='LivingSandReturn':c+=block(s,'void DoAction(')
 return c.replace(' override','')
tree=ast.parse((r/'tools/tests/prepare_fifteenth.py').read_text(encoding='utf8'));ns=dict(r=r,source=source,re=re);exec(compile(ast.Module(body=[x for x in tree.body if isinstance(x,ast.FunctionDef)and x.name=='constants'],type_ignores=[]),'enum-reader','exec'),ns);constants=ns['constants']
def emitted(e,old,native):
 codes=[selected(e,old,cls)for cls in e['classes']];con=constants(e,''.join(codes)+(' NPC_CRIMSON_FOG'if e['key']=='AzureFogFreeze'else''))
 if native:con=re.sub(r'constexpr uint32 (?:UNIT_\w+|CRITERIA_TYPE_\w+|REACT_\w+|SELECT_TARGET_RANDOM)=\d+;\n','',con)
 def translate(c):
  if not native:return c
  return c.replace('me->GetAura(','ObservedAura(').replace('Aura const* aura','auto aura').replace('me->Attack(target, true);','ObserveAttack(target);').replace('me->GetMotionMaster()->MoveChase(target);','ObserveChase(target);')
 return con,''.join('struct '+('Frame'if i==0 else'OtherFrame')+':Base{using Base::Base;'+translate(c)+'};'for i,c in enumerate(codes))
if len(sys.argv)>2:
 e=next(e for e in plan if e['key']==sys.argv[2]);con,c=emitted(e,sys.argv[3]=='baseline',False);(out/'Constants.inc').write_bytes(con.encode('utf8'));(out/'Selected.inc').write_bytes(c.encode('utf8'))
else:
 parts=[]
 for e in plan:
  con,c=emitted(e,False,True);parts.append('namespace '+e['key']+'{'+con+c+'}')
 (out/'TwentyFifthNativeScripts.inc').write_bytes(('namespace TwentyFifthNative{'+''.join(parts)+'}').encode('utf8'))

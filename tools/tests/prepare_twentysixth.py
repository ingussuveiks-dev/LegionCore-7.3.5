"""Extract all thirty guards, grouped by ten independent bound lifecycles."""
from pathlib import Path
import ast,json,re,subprocess,sys
r=Path(__file__).resolve().parents[2];out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True)
plan=json.loads((r/'tools/tests/TwentySixthTenRepairs.json').read_text(encoding='utf8'));base='78c6abe9a8154ea4585feb0a2134e4504f0a8f9f'
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
 codes=[selected(e,old,cls)for cls in e['classes']];con=constants(e,''.join(codes));parts=[]
 if native:con=re.sub(r'constexpr uint32 UNIT_\w+=\d+;\n','',con)
 for i,c in enumerate(codes):
  name='Frame'+str(i)
  if native:
   c=c.replace('me->HasAura(','ObservedAura(')
   parts.append('struct '+name+':Base,NullCreatureAI{explicit '+name+'(Creature*c):Base(c),NullCreatureAI(c){}using Base::me;using Base::DoCast;using Base::HealthBelowPct;'+c+'};')
  else:parts.append('struct '+name+':Base{using Base::Base;'+c+'};')
 parts.append('template<class Check>void Each(Creature*c,Check check){')
 for i in range(len(codes)):
  if native:parts.append('{auto*f=new Frame'+str(i)+'(c);Require(c->AIM_Initialize(f),"selected native AI installation");check(*f,'+str(i)+');}')
  else:parts.append('{Frame'+str(i)+' f(c);check(f,'+str(i)+');}')
 parts.append('}')
 return con,''.join(parts)
if len(sys.argv)>2:
 e=next(e for e in plan if e['key']==sys.argv[2]);con,c=emitted(e,sys.argv[3]=='baseline',False);(out/'Constants.inc').write_bytes(con.encode('utf8'));(out/'Selected.inc').write_bytes(c.encode('utf8'))
else:
 parts=[]
 for e in plan:
  con,c=emitted(e,False,True);parts.append('namespace '+e['key']+'{'+con+c+'}')
 (out/'TwentySixthNativeScripts.inc').write_bytes(('namespace TwentySixthNative{'+''.join(parts)+'}').encode('utf8'))

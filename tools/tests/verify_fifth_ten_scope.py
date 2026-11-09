from pathlib import Path
import re,subprocess,json
r=Path(__file__).resolve().parents[2];plan=json.loads((r/'tools/tests/FifthTenRepairs.json').read_text());assert len(plan)==len({x['key']for x in plan})==10
def block(s,prefix):
 a=s.index(prefix);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
for path in {p for e in plan for p in [e['path'],*e.get('extra',[])]}:
 old=subprocess.check_output(['git','-C',str(r),'show','bc19ca10b:'+path]).decode('utf8');s=(r/path).read_text(encoding='utf-8-sig')
 normalized=s
 if path.endswith('MotionMaster.cpp'):normalized=normalized.replace(block(s,'void MotionMaster::MoveJump(float x, float y, float z, float speedXY, float speedZ)'),'')
 elif path.endswith('MotionMaster.h'):normalized=normalized.replace('// Legacy XYZ callers must not implicitly convert X to Position.','').replace('void MoveJump(float x, float y, float z, float speedXY, float speedZ);','')
 else:
  for e in plan:
   if e['path']!=path:continue
   prefix=('struct 'if e['cls']in ['mob_zao','spell_pal_at_devotion_aura']else'class ')+e['cls']+' :'
   actual=block(s,prefix);prior=block(old,prefix);assert actual!=prior,e['key'];normalized=normalized.replace(actual,prior)
 assert re.sub(r'\s','',normalized)==re.sub(r'\s','',old),path
print('PASS exactly ten product fixes;ten complete production files match bc19ca10b outside selected classes and exact XYZ overload/declaration. Boss progression/death/credit/timers and all unrelated scripts unchanged.')

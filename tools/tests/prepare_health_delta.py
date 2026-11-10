from pathlib import Path
import subprocess,sys
r=Path(__file__).resolve().parents[2];s=subprocess.check_output(['git','-C',str(r),'show','0f4776548:src/server/game/Entities/Unit/Unit.cpp']).decode('utf8')
def block(marker):
 a=s.index(marker);b=s.index('{',a)+1;depth=1
 while depth:
  depth+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
body=block('int64 Unit::ModifyHealth(').replace('Unit::ModifyHealth','BeforeModifyHealth')+'\n'+block('int64 Unit::GetHealthGain(').replace('Unit::GetHealthGain','BeforeGetHealthGain')+'\n'
old='victim->ModifyHealth(-static_cast<int32>(damage), this);';assert s.count(old)==1
body+='void BeforeNonlethalDamage(Unit* victim,uint32 damage) { '+old+' }\n'
out=Path(sys.argv[1]);out.mkdir(parents=True,exist_ok=True);(out/'HealthBefore.inc').write_bytes(body.encode('utf8'))

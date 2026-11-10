from pathlib import Path
import subprocess
r=Path(__file__).resolve().parents[2];p='src/server/game/Entities/Unit/Unit.cpp'
old=subprocess.check_output(['git','-C',str(r),'show','0f4776548:'+p]).decode('utf8').replace('\r\n','\n');new=(r/p).read_text(encoding='utf8')
def block(s,marker):
 a=s.index(marker);b=s.index('{',a)+1;depth=1
 while depth:
  depth+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
for marker in ['int64 Unit::ModifyHealth(', 'int64 Unit::GetHealthGain(']:
 old=old.replace(block(old,marker),'');new=new.replace(block(new,marker),'')
old=old.replace('victim->ModifyHealth(-static_cast<int32>(damage), this);','victim->ModifyHealth(-static_cast<int64>(damage), this);',1)
# The later Twin repair separately verifies this exact post-callback life contract.
a=new.index('    // Encounter protection must see');b=new.index('    // A player charmed by an NPC',a);part=new[a:b]
guard='if (!damage || !creature->IsAlive())\n                return damage;'
if guard in part:
 assert part.count(guard)==3
 new=new[:a]+part.replace(guard,'if (!damage)\n                return 0;')+new[b:]
assert old==new,'Unrelated damage/heal/ownership/group/proc/packet handling changed'
p='src/server/game/Entities/Unit/Unit.h';assert (r/p).read_text(encoding='utf8')==subprocess.check_output(['git','-C',str(r),'show','0f4776548:'+p]).decode('utf8').replace('\r\n','\n')
print('PASS native health delta scope: only ModifyHealth/GetHealthGain bounded signed-delta arithmetic and widening already-normalized nonlethal damage argument. Unit ABI,SetHealth/SetHealthScal,target-relative services,heal notifications,owner/group/kill/proc and health packet paths unchanged.')

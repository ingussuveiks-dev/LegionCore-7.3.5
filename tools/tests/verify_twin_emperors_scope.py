from pathlib import Path
import subprocess
r=Path(__file__).resolve().parents[2]
def read_old(p,revision):return subprocess.check_output(['git','-C',str(r),'show',revision+':'+p]).decode('utf8').replace('\r\n','\n')
def block(s,marker):
 a=s.index(marker);b=s.index('{',a)+1;depth=1
 while depth:
  depth+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
p='src/server/scripts/Kalimdor/TempleOfAhnQiraj/boss_twinemperors.cpp';old=read_old(p,'0f4776548');new=(r/p).read_text(encoding='utf8')
before=block(old,'void JustDied(');after=block(new,'void JustDied(');marker='        if (!DontYellWhenDead)'
assert before[before.index(marker):]==after[after.index(marker):],'Original death sound tail changed'
for marker in ['void DamageTaken(', 'void JustDied(']:old=old.replace(block(old,marker),'',1)
for marker in ['Creature* GetLivingBrother(', 'static void RegisterSharedDamage(', 'void CopySharedEligibility(', 'void DamageTakenAfterScaling(', 'void JustDied(']:new=new.replace(block(new,marker),'',1)
for line in ['#include "Player.h"\n','#include <boost/multiprecision/cpp_int.hpp>\n','    bool deathHandled = false;\n','        deathHandled = false;\n']:new=new.replace(line,'',1)
normalize=lambda s:'\n'.join(line.rstrip() for line in s.splitlines() if line.strip())
assert normalize(old)==normalize(new),'Unrelated emperor movement/healing/timers/abilities/immunities/registration changed'
p='src/server/game/Entities/Unit/Unit.cpp';old=read_old(p,'5a55c0163');new=(r/p).read_text(encoding='utf8')
a=old.index('    // Encounter protection must see');b=old.index('    // A player charmed by an NPC',a);part=old[a:b]
before='if (!damage)\n                return 0;';after='if (!damage || !creature->IsAlive())\n                return damage;';assert part.count(before)==3
assert old[:a]+part.replace(before,after)+old[b:]==new,'Only three native post-scaled callback life checks may change'
print('PASS Twin source scope:only shared after-scaling math/ownership/native death helpers,one-use reset checkpoint and dependencies;original healing/abilities/routes/immunities/sounds/registrations retained. Unit changes exactly three instance/current/suspended callback life checks returning real damage;all other numeric/owner/group/proc/packet paths preserved.')

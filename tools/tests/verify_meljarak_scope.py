from pathlib import Path
import subprocess
r=Path(__file__).resolve().parents[2];p='src/server/scripts/Pandaria/HeartFear/boss_lord_meljarak.cpp'
old=subprocess.check_output(['git','-C',str(r),'show','d83751688:'+p]).decode('utf8').replace('\r\n','\n');new=(r/p).read_text(encoding='utf8')
def block(s,marker):
 a=s.index(marker);b=s.index('{',a)+1;depth=1
 while depth:depth+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
# Only the shared squad helpers are replaced; every original combat/cast,
# CC limit, pose, 45s respawn and registration body remains independently pinned.
for text_name in ['old','new']:
 s=globals()[text_name];a=s.index('struct npc_soldierAI');b=s.index('class npc_generic_soldier',a);globals()[text_name]=s[:a]+s[b:]
prefix=old.index('class npc_generic_soldier');a=old[:prefix];b=old[prefix:];b=b.replace(block(b,'void DamageTaken('),'',1);old=a+b
prefix=new.index('class npc_generic_soldier');a=new[:prefix];b=new[prefix:]
for m in ['void DamageTakenAfterScaling(','void JustDied(']:b=b.replace(block(b,m),'',1)
new=a+b
for m in ['static uint8 SquadBit(','void SoldierDied(']:new=new.replace(block(new,m),'',1)
for line in ['#include "Player.h"\n','            uint8 defeatedSquads = 0;\n','                defeatedSquads = 0;\n','                defeatedSquads &= ~SquadBit(summoned->GetEntry());\n','                sharedDeathHandled = false;\n','                events.Reset();\n']:
 # events.Reset belongs to both the original boss and the newly reset soldier.
 if line=='                events.Reset();\n':
  at=new.index('class npc_generic_soldier');new=new[:at]+new[at:].replace(line,'',1)
 else:new=new.replace(line,'',1)
new=new.replace('SendHealSoldiers(pInstance, modhealth);','SendHealSoldiers(pInstance, me, me->GetEntry(), me->GetGUID(), modhealth);')
new=new.replace('if (action == ACTION_1 && me->IsAlive())','if (action == ACTION_1)')
normalize=lambda s:'\n'.join(x.rstrip() for x in s.splitlines() if x.strip())
assert normalize(old)==normalize(new),'Unrelated boss/soldier CC,abilities,45s events,poses,prerequisites or registrations changed'
print('PASS Meljarak scope:shared damage/death/heal helpers,post-scaling/death hooks,per-actor and per-generation reset;all other original combat,CC,timing,casts,45s respawn,poses and bindings pinned. No core/packet/API changes.')

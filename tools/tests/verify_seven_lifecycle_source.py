"""Limit this batch to seven complete authored script classes and one admission guard."""
from pathlib import Path
import subprocess,hashlib,json
root=Path(__file__).resolve().parents[2]
classes={'spell_generic.cpp':['spell_gen_touch_of_elune','spell_mount_summon_chauffeur','spell_flight_masters_whistle_argus'],'spell_item.cpp':['spell_item_leystone_hoofplates','spell_item_demonsteel_stirrups','spell_item_golem','spell_masters_hunter_seeking_crystal_teleport']}
def redact(s,name):
 a=s.index('class '+name+' ');i=s.index('{',a);depth=1;j=i+1
 while depth:
  if s[j]=='{':depth+=1
  elif s[j]=='}':depth-=1
  j+=1
 assert s[j]==';'
 return s[:a]+'<'+name+'>'+s[j+1:]
for file,names in classes.items():
 path='src/server/scripts/Spells/'+file
 old=subprocess.check_output(['git','show','6dc3bc59f:'+path],cwd=root).decode('utf-8').replace('\r\n','\n')
 new=(root/path).read_text(encoding='utf-8')
 for name in names:old=redact(old,name);new=redact(new,name)
 assert old==new,'Unrelated spell change: '+file
path='src/server/game/Entities/Vehicle/Vehicle.cpp'
old=subprocess.check_output(['git','show','6dc3bc59f:'+path],cwd=root).decode().replace('\r\n','\n')
new=(root/path).read_text();a=old.index('bool VehicleJoinEvent::Execute(')
expected=old[:a]+old[a:].replace('if (!Passenger->IsInWorld())','// Admission is asynchronous: a living caster may die before this event runs.\n    if (!Passenger->IsInWorld() || Passenger->IsPreDelete() || !Passenger->IsAlive())',1)
assert expected==new,'Unrelated vehicle change'
print('PASS complete production file scope: six feature repairs (two matching consumables in one) and one deferred-admission guard; all unrelated source identical to 6dc3bc59f')

"""Certify that this focused ride repair preserves all unrelated native code."""
from pathlib import Path
import hashlib, subprocess
r=Path(__file__).resolve().parents[2]
sha=lambda s:hashlib.sha256(s.encode()).hexdigest()
def block(s,start):
 a=s.index(start);b=s.index('{',a)+1;d=1
 while d:d+=(s[b]=='{')-(s[b]=='}');b+=1
 return s[a:b]
def pinned(path):return subprocess.check_output(['git','-C',str(r),'show','2bee4580c:'+path],encoding='utf-8')
expected={'wave_case': '88228f61668172bc683fe8d879cf64c579f475f1b356cd36b43d6e54767e6fd5', 'preflight': '9eb55e31b13b333beeddadb90a340b71c6d77d549eb99a91adb5aa76984cd65d', 'ack_break': 'b1ca835dceacc6b85b3626ce3f24cc7223bf9ab5aae05e686d68945ca7f7098e', 'defer': '57c58fd8a2b21db91a500422b02fbe1aef58ce3dd0ca6eebe2aa1ae71564aed6', 'vehicle_ack': '3bdd7ec64442d37c0903735e7af989b7ae153bb7a55837b6fc3f65d0f7a57dc5'}
paths={'at':'src/server/game/Entities/AreaTrigger/AreaTrigger.cpp','vehicle':'src/server/game/Entities/Vehicle/Vehicle.cpp'}
at=(r/paths['at']).read_text(encoding='utf-8');v=(r/paths['vehicle']).read_text(encoding='utf-8')
parts={'wave_case':block(at,'        case SharasdalWaveRide::Wave:')}
a=at.index('    bool waveRide =');b=at.index('    bool CallScriptAreaTriggerCast',a);parts['preflight']=at[a:b]
a=at.index('            if (boardedWave)');b=at.index('\n',at.index('break;',a))+1;parts['ack_break']=at[a:b]
a=at.index('    if (waveRide && !boardedWave)\n        return; // A cast attempt');b=at.index('\n',at.index('return;',a))+1;parts['defer']=at[a:b]
a=v.index('    // Commit only after');b=v.index('    if (Target->IsCreature())',a);parts['vehicle_ack']=v[a:b]
for name,part in parts.items():assert sha(part)==expected[name],name+' changed beyond certified repair'
for name in ['wave_case','preflight','ack_break','defer']:at=at.replace(parts[name],'',1)
v=v.replace(parts['vehicle_ack'],'',1)
# Independently paired native lifecycle repair: normalize only this exact guard.
life='// Admission is asynchronous: a living caster may die before this event runs.\n    if (!Passenger->IsInWorld() || Passenger->IsPreDelete() || !Passenger->IsAlive())'
assert v.count(life)==1,'Reviewed deferred life guard missing/duplicated'
v=v.replace(life,'if (!Passenger->IsInWorld())',1)

for s,key in [(at,'at'),(v,'vehicle')]:
 s=s.replace('#include "SharasdalWaveRide.h"','')
 assert ''.join(s.split())==''.join(pinned(paths[key]).split()),'Unreviewed unrelated native code: '+key
assert sha((r/'src/server/game/Entities/AreaTrigger/SharasdalWaveRide.h').read_text(encoding='utf-8'))=='e77168ceb8ffab7b87784a95eab9ec7cb65f05db9be0955ded0768203de488c5'
print('PASS wave ride whole native source scope:only exact owned cadence/ack/defer and vehicle-seat notification;all unrelated cast/charge/counter/actions/vehicle/AI/mover/native admission code pinned2bee4580c retained except exact separately tested deferred life guard')

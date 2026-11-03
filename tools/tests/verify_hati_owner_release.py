"""Player-only Hati lifecycle must reject non-Player before native lookup.

The separate native ASAN counterfactual proves the old null-this failure;
the repaired matrix includes original Player reaction/cleanup behavior.
"""
from pathlib import Path
import hashlib,json,runpy
from sharasdal_erunak_entry_pe import sites
from monk_intro_instance_pe import reachable
p=runpy.run_path(str(Path(__file__).with_name('verify_hati_reaction_release.py')))
n,ins,update=(p[k] for k in ('n','ins','update'))
pairs=[(o.mnemonic,o.op_str) for o in update]
caster,=sites(n,update,'?GetCaster@AuraScript@@')
lookup,=sites(n,update,'?GetCreatureOrPetOrVehicle@ObjectAccessor@@')
guard=next(i for i,o in enumerate(update) if (o.mnemonic,o.op_str)==('cmp','byte ptr [rax + 0x16], 4'))
assert caster<guard<lookup
branch=update[guard+1];assert branch.mnemonic=='jne'
assert pairs[caster+1:guard+2]==[
 ('mov','rsi, rax'),('test','rax, rax'),('je',branch.op_str),
 ('cmp','dword ptr [rax + 0xa18], r13d'),('jne',branch.op_str),
 ('cmp','byte ptr [rax + 0x16], 4'),('jne',branch.op_str)]
failure=reachable(update,branch.operands[0].imm)
for name in ('?GetCreatureOrPetOrVehicle@ObjectAccessor@@','?GetPet@Player@@','?GetReactState@Creature@@',
             '?SetReactState@Creature@@','?CastSpell@Unit@@','?DespawnOrUnsummon@Creature@@'):
 assert all(update[i].address not in failure for i in sites(n,update,name)),('Non-Player branch reaches player lifecycle',name)
assert pairs[guard+2]==('mov','rax, qword ptr [rax + 0x4c8]')
assert update[lookup].address not in reachable(update,update[0].address,{update[guard].address})
print('PASS actual registered Hati update rejects null/dead/non-Player caster before controlled-list selection, every native Player::GetPet call, reaction synchronization, summon and cleanup. Native Player/passive matrix and pinned ASAN counterfactual remain separate; no client/all36 claim.')
print(json.dumps({'world_sha256':hashlib.sha256(n['binary']).hexdigest(),'spell':197344,'required_caster_type':4}))

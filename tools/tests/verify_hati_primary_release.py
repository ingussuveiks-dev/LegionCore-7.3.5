"""Registered Hati update recognizes only its actual selected primary pet.

The native counterfactual/guardian matrix runs separately; this checks the
linked implementation and retains the prior registration/type/passive proofs.
"""
from pathlib import Path
import hashlib,json,runpy
from sharasdal_erunak_entry_pe import sites
p=runpy.run_path(str(Path(__file__).with_name('verify_hati_owner_release.py')))
n,update=(p[k] for k in ('n','update'))
pairs=[(o.mnemonic,o.op_str) for o in update]
primary,reaction,summon=sites(n,update,'?GetPet@Player@@')
lookup,=sites(n,update,'?GetCreatureOrPetOrVehicle@ObjectAccessor@@')
assert lookup<primary<reaction<summon
assert pairs[primary+1:primary+5]==[
 ('cmp','rdi, rax'),('jne',update[primary+2].op_str),
 ('mov','r14b, 1'),('jmp',update[primary+4].op_str)]
assert update[primary+2].operands[0].imm==update[primary+4].operands[0].imm
# RDI is precisely the selected controlled creature, retained across GetPet.
assert pairs[lookup+1]==('mov','rdi, rax')
assert not any(o.mnemonic=='mov' and o.op_str.startswith('rdi,') for o in update[lookup+2:primary])
assert [pair for pair in pairs if pair[0] in ('mov','xor') and pair[1].startswith('r14b,')]==[
 ('xor','r14b, r14b'),('mov','r14b, 1')]
# Existing !findHati && findPet summon and !findPet cleanup both consume
# this exact flag, including the findHati=true out-of-line branch.
checks=[i for i,pair in enumerate(pairs) if pair==('test','r14b, r14b')]
assert len(checks)==2
first,last=checks
assert reaction<first<summon<last
assert update[first+1].mnemonic=='je' and update[last+1].mnemonic=='jne'
cleanup=update[first+1].operands[0].imm
assert update[last+2].address==cleanup
assert pairs[last+2]==('test','rbp, rbp') and update[last+3].mnemonic=='je'
assert update[last+4].mnemonic=='jmp'
despawn_start=update[last+4].operands[0].imm
start=next(i for i,o in enumerate(update) if o.address==despawn_start)
despawn=[i for i in sites(n,update,'?DespawnOrUnsummon@Creature@@') if i>start][0]
assert ('mov','edx, 0x3e8') in pairs[start:despawn]
print('PASS actual registered Hati update: only controlled Creature pointer equal to native Player::GetPet can set findPet; both existing summon/cleanup paths consume that flag, with1000ms cleanup and no guardian substitution. Prior Player/passive/registration checks retained. Actual client remains separate.')
print(json.dumps({'world_sha256':hashlib.sha256(n['binary']).hexdigest(),'primary_identity':'Player::GetPet pointer','interval_ms':2000,'cleanup_ms':1000}))

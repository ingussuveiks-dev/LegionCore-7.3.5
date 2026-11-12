"""Actual linked native222 percentage spread and final Magic cap order."""
from verify_pet_battle_discharge_release import ins,sites,pairs,reaches,n
from pet_variance_scope import verify
from capstone import CS_OP_MEM
import hashlib,json,struct
verify()
effect=ins('?HandleDamageRange@PetBattleAbilityEffect@@')
hit,=sites(effect,'?CalculateHit@PetBattleAbilityEffect@@');calculate,=sites(effect,'?CalculateDamageWithVariance@PetBattleAbilityEffect@@');damage,=sites(effect,'?Damage@PetBattleAbilityEffect@@')
assert hit<calculate<damage
assert {('movzx','edx, word ptr [rax + 0xa]')}<=set(pairs(effect[:hit]))
assert {('movzx','edx, word ptr [r9 + 8]'),('movzx','r9d, word ptr [r9 + 0xc]'),('mov','r8b, 1')}<=set(pairs(effect[hit:calculate])),'Native points/variance and exactly one power-enabled common calculation'
assert pairs(effect[calculate+1:calculate+4])==[('mov','edx, dword ptr [rbx + 0x3c]'),('xor','r9d, r9d'),('mov','r8d, eax')]
dispatch=ins('?Execute@PetBattleAbilityEffect@@')
table=next(x.address+x.size+x.operands[1].mem.disp for x in dispatch if x.mnemonic=='lea'and x.op_str.startswith('r12, [rip')and x.operands[1].type==CS_OP_MEM)
handler,target=struct.unpack_from('<QI',n['binary'],n['offset'](table)+222*16)
assert n['resolve'](handler)==effect[0].address and target==2,'Actual222 table registration retains native enemy selection'
body=ins('?CalculateDamageWithVariance@PetBattleAbilityEffect@@')
rolls=sites(body,'?irand@@');assert len(rolls)==2
roll=rolls[-1] # The earlier native integer roll is the retained critical chance.
assert {('mov','word ptr [rsp + 0x20], r9w'),('movzx','eax, word ptr [rsp + 0x78]'),('test','ax, ax'),('test','ebp, ebp'),('movsxd','r9, ebp'),('mov','ecx, 0xc8'),('sub','rcx, rax'),('lea','rcx, [r8 + 0xc8]'),('mov','edx, 0x7fffffff'),('cmp','r8, rdx'),('cmovl','edx, r8d'),('cmovg','r15d, r10d'),('mov','ecx, r15d')}<=set(pairs(body[:roll]))
assert sum((x.mnemonic,x.op_str)==('imul','rcx, r9')for x in body[:roll])==2,'Both bounds use widened signed multiplication'
assert ('movabs','r11, 0xa3d70a3d70a3d70b')in pairs(body[:roll]),'Actual signed division200 reciprocal remains linked'
for value in [('test','ax, ax'),('test','ebp, ebp')]:
 gate=next(i for i,x in enumerate(body[:roll])if(x.mnemonic,x.op_str)==value);branch=body[gate+1]
 assert branch.mnemonic in('je','jle')and body[roll].address not in reaches(body,branch.operands[0].imm),'Zero-variance ordinary calls never consume the extra roll'
assert {('cmp','dword ptr [rcx + 0x210], 0'),('cmp','eax, 0x23')}<=set(pairs(body[roll:])),'Magic35percent cap remains after the actual variation roll'
print('PASS actual linked222 native points/percentage variance, widened bounded signed range around modified damage, ordinary zero-variance RNG preservation, final Magic cap and original native target/hit/death path; complete retained pet proofs pass')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'native_rows':63,'native_cases':144,'property':222}))

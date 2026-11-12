"""Actual linked native type229 marks, categorical state124 and damage consumer."""
from verify_pet_battle_accuracy_roster_release import ins,sites,pairs,reaches,n
from pet_ward_rot_scope import verify
from capstone import CS_OP_MEM
import hashlib,json,struct
verify()
refresh=ins('?RefreshPetTypeOverride@PetBattle@@')
write,=sites(refresh,'?SetPetState@PetBattle@@')
assert {('cmp','edx, 6'),('cmp','esi, 6'),('cmp','byte ptr [rdx + 0x20], 0'),('cmp','dword ptr [rdx + 0xc], esi'),('cmp','r10d, ebx'),('cmp','dword ptr [rdx], 0x7c'),('lea','r8d, [r9 - 1]'),('cmp','r8d, 9'),('mov','ebx, r10d'),('mov','edi, r9d'),('mov','dword ptr [rsp + 0x28], edi'),('mov','dword ptr [rsp + 0x20], 0x7c'),('mov','r8d, esi'),('mov','edx, r14d')}<=set(pairs(refresh[:write]))
assert ('add','edi, r9d')not in pairs(refresh),'A type is a category, never an additive total'
for cmp in [('cmp','edx, 6'),('cmp','esi, 6')]:
 gate=next(i for i,x in enumerate(refresh)if(x.mnemonic,x.op_str)==cmp)
 branch=refresh[gate+1];assert branch.mnemonic=='jae'
 assert refresh[write].address not in reaches(refresh,branch.operands[0].imm)
handler=ins('?HandleDefensiveTypeAura@PetBattleAbilityEffect@@')
lookup,=sites(handler,'?GetAura@PetBattleAbilityEffect@@')
hit,=sites(handler,'?CalculateHit@PetBattleAbilityEffect@@');add,=sites(handler,'?AddAura@PetBattleAbilityEffect@@')
assert hit<add and handler[hit].address in reaches(handler,handler[lookup+1].address),'Real parent lookup rejoins the one native hit/add tail'
assert {('test','dword ptr [rcx + 0x44], 0x29a'),('cmp','word ptr [rdi + 0x12], 0x7c'),('cmp','word ptr [rdi + 0x10], 0'),('cmp','ecx, 0x23e'),('cmp','ecx, 0x1dc'),('mov','r8d, 0x334'),('mov','r8d, 0x58a'),('mov','edx, dword ptr [rbx + 0x3c]')}<=set(pairs(handler[:lookup]))
assert {('test','rax, rax'),('mov','ecx, dword ptr [rbx + 0x1c]'),('cmp','dword ptr [rax + 8], ecx')}<=set(pairs(handler[lookup:])),'Actual native GetAura result retains original source guard'
gate=next(i for i,x in enumerate(handler)if(x.mnemonic,x.op_str)==('cmp','dword ptr [rax + 8], ecx'))
assert handler[gate+1].mnemonic=='jne'and handler[hit].address not in reaches(handler,handler[gate+1].operands[0].imm)
execute=ins('?Execute@PetBattleAbilityEffect@@')
table=next(x.address+x.size+x.operands[1].mem.disp for x in execute if x.mnemonic=='lea'and x.op_str.startswith('r12, [rip')and x.operands[1].type==CS_OP_MEM)
pointer,target=struct.unpack_from('<QI',n['binary'],n['offset'](table)+229*16)
assert n['resolve'](pointer)==handler[0].address and target==0,'Actual handler229 dispatch with explicit native recipient selection'
damage=ins('?CalculateDamageWithVariance@PetBattleAbilityEffect@@')
assert {('mov','eax, dword ptr [r10 + 0x33c]'),('lea','ecx, [rax - 1]'),('lea','r11d, [rax - 1]'),('cmp','ecx, 9'),('cmova','r11d, ebx')}<=set(pairs(damage)),'Actual common damage uses bounded state124 minus one, retaining species fallback'
apply=ins('?Apply@PetBattleAura@@');call,=sites(apply,'?RefreshPetTypeOverride@PetBattle@@')
assert {('cmp','edi, 0x7c'),('mov','dword ptr [rdx], edi'),('mov','eax, dword ptr [r9]'),('mov','dword ptr [rdx + 4], eax'),('mov','r8d, dword ptr [r13 + 0xc]'),('mov','edx, dword ptr [r13 + 8]')}<=set(pairs(apply[:call]))
expire=ins('?Expire@PetBattleAura@@');call,=sites(expire,'?RefreshPetTypeOverride@PetBattle@@')
assert {('mov','rax, qword ptr [rbx + 0x28]'),('mov','qword ptr [rbx + 0x30], rax'),('mov','r8d, dword ptr [rbx + 0xc]'),('mov','edx, dword ptr [rbx + 8]')}<=set(pairs(expire[:call])),'Inlined Remove clears its applied receipts before rebuilding the actual holder'
reset=ins('?HandleResetState@PetBattleAbilityEffect@@');call,=sites(reset,'?RefreshPetTypeOverride@PetBattle@@')
assert ('cmp','edx, 0x7c')in pairs(reset[:call]) and sites(reset,'?SetState@PetBattleAbilityEffect@@')
rounds=ins('?ProceedRound@PetBattle@@')
calls=sites(rounds,'?Cast@PetBattle@@');start=next(i for i in calls if{('mov','dword ptr [rsp + 0x20], 6'),('mov','r8d, dword ptr [rdx]')}<=set(pairs(rounds[i-9:i])))
assert {('cmp','dword ptr [rax], 0x58a'),('cmp','byte ptr [rdx + 0x20], 0'),('mov','eax, dword ptr [rdx + 0xc]'),('mov','dword ptr [rsp + 0x30], eax'),('mov','edx, dword ptr [rdx + 8]')}<=set(pairs(rounds[:start]))
ordinary=sites(rounds,'?DoCasts@PetBattleTeam@@');assert len([i for i in ordinary if i>start])>=2
remove=ins('?HandleRemoveAura@PetBattleAbilityEffect@@')
constants={o.imm for x in remove for o in x.operands if o.type==2}
assert {820,1418,2834,2837,2831,2833,2838,2846}<=constants,'Exact parent/removal/producer receipts remain in actual cleanup'
print('PASS linked Ward/Rot native229 dispatch, bounded live-holder categorical state124, damage species fallback, applied receipt refresh/removal, reset and real source/owner round-start; complete retained pet proofs pass')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'roots':[574,476],'controllers':[820,1418],'native_cases':50}))

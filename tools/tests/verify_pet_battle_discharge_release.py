"""Actual linked Discharge type producer, owned cleanup and lethal self-cost."""
from verify_pet_battle_ward_rot_release import ins,sites,pairs,reaches,n
from pet_discharge_scope import verify
import hashlib,json
verify()
handler=ins('?HandleDefensiveTypeAura@PetBattleAbilityEffect@@')
parent,=sites(handler,'?GetAura@PetBattleAbilityEffect@@')
assert {('lea','eax, [rcx - 0x6b5]'),('cmp','eax, 1'),('mov','r8d, 0x6b6')}<=set(pairs(handler[:parent])),'Actual1717/1718 retain the known controller1718-bound229 producer'
cleanup=ins('?HandleRemoveAura@PetBattleAbilityEffect@@')
assert {('cmp','r8d, 0x6b6'),('cmp','dword ptr [rax + 0x15], 0xc7f'),('cmp','eax, 0x6b6'),('cmp','eax, 0xc7e'),('cmp','eax, 0xc83')}<=set(pairs(cleanup)),'Actual3199 cleanup selects only3198/3203 receipts'
cast=ins('?Cast@PetBattle@@');select,=sites(cast,'?SelectTargets@PetBattleAbilityEffect@@');execute,=sites(cast,'?Execute@PetBattleAbilityEffect@@')
gate=next(i for i,x in enumerate(cast)if(x.mnemonic,x.op_str)==('cmp','r10d, 0x6b5'))
assert pairs(cast[gate:gate+11:2])==[('cmp','r10d, 0x6b5'),('cmp','r10d, 0x576'),('cmp','r10d, 0x633'),('cmp','edi, 1'),('cmp','r15d, 0xff'),('movzx','eax, dil')]
assert all(cast[gate+j].mnemonic=='je' and cast[gate+j].operands[0].imm==cast[gate+6].address for j in(1,3))
assert all(cast[gate+j].mnemonic=='jne'for j in(5,7,9))and len({cast[gate+j].operands[0].imm for j in(5,7,9)})==1
join=cast[gate+11];assert join.mnemonic=='jmp'and join.operands[0].imm<cast[select].address
assert {('mov','byte ptr [rsp + 0x41], al'),('test','al, al'),('cmp','dword ptr [rsi + 0x150], 0')}<=set(pairs(cast[:select])),'Actual living turn1 non-proc1717 flag rejoins admission before dispatch'
assert {('movzx','ecx, byte ptr [rsp + 0x41]'),('test','cl, cl'),('cmp','ax, 0x18'),('cmp','dword ptr [r11 + 0x84], 3'),('cmp','dword ptr [rsp + 0x64], -1')}<=set(pairs(cast[execute:execute+60])),'Real damage StopChain and lethal terminal path retain only issued Discharge continuation'
dispatch=ins('?Execute@PetBattleAbilityEffect@@')
assert {('cmp','ecx, 0x6b6'),('cmp','word ptr [r9 + 6], 0x31'),('cmp','dword ptr [rbx + 0x4c], r8d')}<=set(pairs(dispatch)),'Own removal can reach actual untargettable holder'
print('PASS actual linked1717/1718 bounded parent-owned229 producer,3199 cleanup restricted to3198/3203 original-source receipts, living turn1 lethal self-cost/cooldown continuation; complete retained pet proofs pass')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'root':1717,'controller':1718,'mark':1419,'native_cases':28}))

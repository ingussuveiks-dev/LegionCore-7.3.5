"""Actual linked four-ability recipient, percentage and terminal sequencing proof."""
from verify_pet_battle_lifecycle_release import ins,sites,values,pairs,reaches,n
import hashlib,json
selector=ins('?SelectTargets@PetBattleAbilityEffect@@')
assert {282,663,836,1025,68,24}<=values(selector)
assert {('cmp','esi, 6'),('mov','esi, dword ptr [rax + 0x70]'),('test','rcx, rcx')}<=set(pairs(selector))
slot=next(i for i,x in enumerate(selector)if(x.mnemonic,x.op_str)==('cmp','dword ptr [rbx + 0x18], 0x401'))
roster=selector[slot:slot+30]
assert {('cmp','eax, 0x18'),('movzx','ecx, byte ptr [rdx + 0x14]'),('dec','ecx'),('cmp','ecx, dword ptr [rdx + 0x58]'),('test','rdx, rdx'),('mov','edx, dword ptr [rdx + 0x130]')}<=set(pairs(roster))
assert len(sites(roster,'?AddTarget@PetBattleAbilityEffect@@'))==1
admission=ins('?AddTarget@PetBattleAbilityEffect@@QEAA_NI@Z')
assert {('cmp','edx, 6'),('test','rcx, rcx')}<=set(pairs(admission[:16]))
percent=ins('?HandleDamageCasterPercent@PetBattleAbilityEffect@@')
assert {282,663,836}<=values(percent)
assert {('mov','sil, 1'),('xor','sil, sil'),('movzx','r8d, sil'),('mov','edx, dword ptr [rdi + 0x3c]')}<=set(pairs(percent))
assert len(sites(percent,'?CalculateDamage@PetBattleAbilityEffect@@'))==1
terminal=ins('?HandleKill@PetBattleAbilityEffect@@')
gate=next(i for i,x in enumerate(terminal)if(x.mnemonic,x.op_str)==('cmp','r8d, 0x11a'))
start=terminal[gate+1].operands[0].imm
assert terminal[gate+1].mnemonic=='je'
body=[x for x in terminal if x.address>=start]
kill,=sites(body,'?Kill@PetBattleAbilityEffect@@');state,=sites(body,'?SetState@PetBattleAbilityEffect@@')
assert state<kill and pairs(body[kill-2:kill])==[('mov','edx, dword ptr [rbx + 0x1c]'),('mov','rcx, rbx')]
assert {('movzx','ecx, word ptr [rax + 0xa]'),('cmp','dword ptr [rax + rcx*4 + 0x14c], 0'),('cmp','r8d, 0x11a'),('cmp','r8d, 0x401'),('mov','r8d, 0x30'),('xor','r9d, r9d')}<=set(pairs(body[:state]))
failed=next(i for i,x in enumerate(body)if(x.mnemonic,x.op_str)==('test','dword ptr [rbx + 0x44], 0x29a'))
branch=body[failed+2];assert branch.mnemonic=='jne'
assert body[kill].address not in reaches(body,branch.operands[0].imm)
cast=ins('?Cast@PetBattle@@')
assert {282,663,836,1025}<=values(cast[:110])
assert {('mov','byte ptr [rsp + 0x40], 1'),('cmp','r12d, 0xff'),('cmp','ebx, 1'),('cmp','dword ptr [r8 + 0x150], r9d')}<=set(pairs(cast[:110]))
execute,=sites(cast,'?Execute@PetBattleAbilityEffect@@');flow=cast[execute:execute+52]
assert {('movzx','r8d, byte ptr [rsp + 0x40]'),('cmp','ax, 0x18'),('cmp','ax, 0x44'),('cmp','byte ptr [rbp - 0x31], 0'),('test','r8b, r8b'),('cmp','dword ptr [rax + 0x84], 3')}<=set(pairs(flow))
print('PASS actual linked four-sacrifice enemy/roster admission, no second power scaling, user-only terminal/native immunity/282+1025Failsafe prohibition and scoped lethal-row continuation; retained native racial proof also passes')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'abilities':[282,663,836,1025],'actual_native_cases':44}))

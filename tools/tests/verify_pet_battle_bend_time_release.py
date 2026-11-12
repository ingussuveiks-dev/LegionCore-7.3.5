"""Actual native Bend Time root, stored-owner swap, cooldown and cleanup proof."""
from verify_pet_battle_unholy_ascension_release import ins,sites,pairs,reaches,n
import hashlib,json
cast=ins('?Cast@PetBattle@@');select,=sites(cast,'?SelectTargets@PetBattleAbilityEffect@@')
assert {('movsxd','r11, dword ptr [rbp + 0x7f]'),('mov','r9d, r8d'),('cmp','r8d, 0x770'),('cmp','esi, 0xa'),('test','ebx, ebx'),('cmp','r11d, 5'),('cmp','dword ptr [rcx + 0x70], r11d'),('cmp','byte ptr [rdx + 0x20], 0'),('mov','ebx, dword ptr [rbp + 0x5f]'),('cmp','dword ptr [rdx], ebx'),('cmp','dword ptr [rdx + 8], r12d'),('cmp','dword ptr [rdx + 0xc], r11d'),('mov','dword ptr [rbp - 0x25], r11d')}<=set(pairs(cast[:select]))
positive=ins('?HandlePositiveAura@PetBattleAbilityEffect@@');remove=ins('?HandleRemoveAura@PetBattleAbilityEffect@@')
for body in (positive,remove):
 hit,=sites(body,'?CalculateHit@PetBattleAbilityEffect@@')
 gate=next(i for i,x in enumerate(body[:hit])if(x.mnemonic,x.op_str)==('test','cx, cx'))
 branch=body[gate+1];assert branch.mnemonic=='je'
 assert body[hit].address not in reaches(body,branch.operands[0].imm),'Native zero accuracy must skip the hit roll'
assert ('test','dword ptr [r12 + 0x44], 0x29a')in pairs(remove),'Native removal retains existing failure flags'
swap=ins('?SwapPet@PetBattle@@')
assert {('cmp','dword ptr [r8 + 0x12c], r13d'),('mov','dword ptr [rsp + 0x30], ecx'),('mov','dword ptr [rsp + 0x20], 0xa'),('mov','r8d, 0x770'),('mov','edx, dword ptr [rdx + 8]')}<=set(pairs(swap))
assert sites(swap,'?CanSwap@PetBattleTeam@@')and sites(swap,'?Cast@PetBattle@@')
selector=ins('?SelectTargets@PetBattleAbilityEffect@@')
assert {('mov','ecx, 0xf6'),('cmp','ax, cx'),('cmp','eax, 0x770'),('movsxd','rax, dword ptr [rbx + 0x4c]'),('cmp','eax, 5'),('mov','eax, dword ptr [rcx + 0x12c]'),('mov','edx, dword ptr [rdx + 0x70]')}<=set(pairs(selector))
execute=ins('?Execute@PetBattleAbilityEffect@@');dispatch,=sites(execute,'?HandleBendTimeCooldown@PetBattleAbilityEffect@@')
assert {('mov','eax, 0xf6'),('cmp','cx, ax'),('cmp','eax, 0x770'),('cmp','rax, 4'),('mov','dword ptr [rbx + 0x3c], eax')}<=set(pairs(execute[:dispatch]))
assert {('cmp','eax, 0x770'),('cmp','word ptr [r8 + 6], 0x31'),('cmp','dword ptr [rbx + 0x4c], ecx')}<=set(pairs(execute[dispatch:])),'The actual holder cleanup must bypass hostile-target hit gating'
cooldown=ins('?HandleBendTimeCooldown@PetBattleAbilityEffect@@')
assert {('test','dword ptr [rcx + 0x44], 0x29a'),('movsxd','rax, dword ptr [rcx + 0x4c]'),('cmp','eax, 5'),('cmp','ecx, 6'),('cmp','ax, 2'),('cmp','word ptr [rcx + 0xa], 1'),('mov','r8d, dword ptr [rbx + 0x18]'),('cmp','ecx, 0x770'),('cmp','ecx, 0x78c'),('cmp','dword ptr [r8 + 8], eax'),('cmp','dword ptr [r11 + 0x70], eax'),('cmp','dword ptr [rbx + 0x3c], r10d'),('cmp','dword ptr [rdx + 0x12c], r9d'),('mov','eax, dword ptr [rdx + rcx*4 + 0x54]'),('cmp','dword ptr [rdx + r8*4 + 0x134], 1'),('mov','dword ptr [rdx + r8*4 + 0x134], 1')}<=set(pairs(cooldown))
assert len(sites(cooldown,'?GetAura@PetBattleAbilityEffect@@'))==1
stores=[x for x in cooldown if x.mnemonic=='mov'and x.op_str.startswith('dword ptr [rdx + r8*4 + ')]
assert pairs(stores)==[('mov','dword ptr [rdx + r8*4 + 0x134], 1')],'Only the actual cooldown array is modified; lockdowns stay native'
death=ins('?Kill@PetBattle@@');expire,=sites(death,'?Expire@PetBattleAura@@')
guard=next(i for i,x in enumerate(death[:expire])if(x.mnemonic,x.op_str)==('cmp','eax, 0x770'))
branch=death[guard+1];assert branch.mnemonic=='je'and branch.operands[0].imm==death[expire+1].address
print('PASS actual linked Bend Time zero-accuracy root, bounded actual stored-owner/changed-front context, owner-team swap,246single-recipient dispatch, learned cooldown1-only write, preserved lockdowns, owned cleanup and selective death retention; all retained pet linked proofs pass')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'root':1905,'aura':1904,'property':246,'native_cases':26}))

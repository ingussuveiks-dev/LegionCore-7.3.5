"""Actual linked inherent191/200 thresholds and strict attack immunity."""
from verify_pet_battle_roboflauge_release import ins,sites,pairs,reaches,n
from pet_damage_threshold_scope import verify
import hashlib,json
verify()
add=ins('?AddPet@PetBattle@@')
required={('mov','ecx, dword ptr [rax + 0x44]'),('mov','ebx, dword ptr [rax + 0x428]'),
 ('cmp','ecx, 0x5e3'),('cmp','ecx, 0x758'),('cmp','ebx, 0x56e'),('mov','r11d, 0xbf'),
 ('cmp','ecx, 0x75a'),('cmp','ebx, 0x68a'),('mov','r11d, 0xc8'),
 ('cmp','eax, ebx'),('cmp','eax, r11d'),('mov','r10d, dword ptr [rcx + 4]'),
 ('test','r10d, r10d'),('add','dword ptr [rax + rcx*4 + 0x14c], r10d')}
assert required<=set(pairs(add)),'Actual native species/183 binding selects191/200 and adds the matching native positive ability-state Value'
write=next(i for i,x in enumerate(add)if(x.mnemonic,x.op_str)==('add','dword ptr [rax + rcx*4 + 0x14c], r10d'))
guard=next(i for i,x in enumerate(add[:write])if(x.mnemonic,x.op_str)==('test','r10d, r10d'))
assert add[guard+1].mnemonic=='jle' and add[guard+1].operands[0].imm>add[write].address
damage=ins('?Damage@PetBattleAbilityEffect@@');block=sites(damage,'?ConsumeBlockCharge@PetBattleAbilityEffect@@')[0]
region=damage[:block];p=pairs(region)
assert {('cmp','ebp, dword ptr [rsi + 0x1c]'),('cmp','eax, 0x1c3'),('cmp','eax, 0x2f6'),('cmp','word ptr [rax + 6], 0x44')}<=set(p),'Only real self-targeted451/758 native68 payments bypass the attack thresholds'
assert {('mov','r9d, dword ptr [rax + 0x448]'),('mov','edx, dword ptr [rax + 0x46c]'),('test','r9d, r9d'),('test','edx, edx')}<=set(p),'Actual target native191/200 values are read with positive-state guards'
low=next(i for i,x in enumerate(region)if(x.mnemonic,x.op_str)==('cmp','r15d, r9d'))
high=next(i for i,x in enumerate(region)if(x.mnemonic,x.op_str)==('cmp','r15d, edx'))
immune=next(i for i,x in enumerate(region)if(x.mnemonic,x.op_str)==('bts','ecx, 9'))
assert region[low+1].mnemonic=='jl' and region[low+1].operands[0].imm==region[immune].address,'Below is strictly less, so equality remains an ordinary hit'
assert region[high+1].mnemonic=='jle' and region[high+1].operands[0].imm>region[immune].address,'Above is strictly greater, so equality remains an ordinary hit'
health=sites(region,'?SetHealth@PetBattleAbilityEffect@@')
assert len(health)==1 and health[0]<block and region[health[0]].address in reaches(damage,region[immune].address),'Actual threshold immunity reaches the shared unchanged-health return even when its tail is merged backward'
tail=reaches(damage,region[immune].address)
assert any(x.mnemonic=='ret' and x.address in tail for x in region)
assert not any(damage[i].address in tail for name in ('?ConsumeBlockCharge@PetBattleAbilityEffect@@','?Cast@PetBattle@@','?Kill@PetBattleAbilityEffect@@')for i in sites(damage,name)),'Ignored attack returns without consuming a block or issuing damage/death reactions'
assert pairs(region[health[0]+1:health[0]+2])==[('xor','al, al')],'Ignored attack returns false after unchanged native health event'
print('PASS linked actual native species183 threshold initialization,191/200 strict boundaries, preserved clone68 payments and ignored-attack return before finite blocks/damage/death callbacks; all retained pet proofs pass')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'native_cases':78,'abilities':[1390,1674],'states':[191,200]}))

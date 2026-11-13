"""Native ordinary/periodic Rocko admission before miss,blocks and reactions."""
from verify_pet_battle_crumble_release import ins,sites,pairs,reaches,n
from pet_crumble_immunity_scope import verify
import hashlib,json
verify()
rows=ins('?Damage@PetBattleAbilityEffect@@');source,=sites(rows,'?IsCrumbleSource@PetBattle@@')
block=sites(rows,'?ConsumeBlockCharge@PetBattleAbilityEffect@@')[0]
assert {('mov','ebp, edx'),('movzx','ebx, r9b'),('mov','r15d, r8d'),('mov','rsi, rcx')}<=set(pairs(rows[:source])),'Actual target,explicit bypass,damage and effect receiver retain their native parameter roles'
assert {('bt','edx, 0x10'),('movzx','ecx, word ptr [rax + 6]'),('cmp','cx, 0x18'),('mov','eax, 0xb3'),('cmp','cx, ax'),('test','edx, 0x29a'),('cmp','ebp, dword ptr [rsi + 0x1c]'),('test','bl, bl'),('test','al, al'),('mov','rcx, qword ptr [rsi + 0x10]'),('mov','edx, ebp')}<=set(pairs(rows[:source])),'Only successful hostile ordinary or actual native24/179 periodic contexts enter the source check'
immune=next(i for i,x in enumerate(rows)if(x.mnemonic,x.op_str)==('or','dword ptr [rsi + 0x44], 0x200'))
health=next(i for i in sites(rows,'?SetHealth@PetBattleAbilityEffect@@')if i>immune)
assert source<immune<health<block and rows[source+2].mnemonic=='je'
assert rows[immune].address not in reaches(rows,rows[source+2].operands[0].imm),'A failed source binding cannot enter the intrinsic immunity receipt'
assert {('mov','r8d, dword ptr [r8 + 0x60]'),('mov','edx, ebp'),('mov','rcx, rsi')}<=set(pairs(rows[immune:health])),'Native health receipt preserves the actual recipient health'
tail=reaches(rows,rows[immune].address)
assert any(x.mnemonic=='ret'and x.address in tail for x in rows[:block])
assert not any(rows[i].address in tail for name in('?ConsumeBlockCharge@PetBattleAbilityEffect@@','?Cast@PetBattle@@','?Kill@PetBattleAbilityEffect@@')for i in sites(rows,name)),'Intrinsic immunity returns before finite charges,damage reactions or death'
assert pairs(rows[health+1:health+2])==[('xor','al, al')],'Ignored native damage returns failure after the unchanged-health event'
for condition in [('test','edx, 0x29a'),('cmp','ebp, dword ptr [rsi + 0x1c]')]:
 gate=next(i for i,x in enumerate(rows[:source])if(x.mnemonic,x.op_str)==condition)
 branch=rows[gate+1];assert branch.mnemonic in('je','jne')and rows[source].address not in reaches(rows,branch.operands[0].imm),'Failed hits and actual self payments bypass the new protection'
bypass=next(i for i,x in enumerate(rows[:source])if(x.mnemonic,x.op_str)==('test','al, al'))
assert rows[bypass+1].mnemonic=='je'and rows[source].address not in reaches(rows,rows[bypass+1].operands[0].imm),'A true explicit nonperiodic bypass retains its original path'
print('PASS linked actual native1811/1586 ordinary and24/179 periodic hostile immunity before zero-miss,finite blocks and damage/death callbacks;original miss,self-payment,explicit nonperiodic bypass and all retained proofs remain')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'native_cases':84,'ordinary_periodic_properties':[24,179],'calculation_formula_changed':False,'full_percentage_unavoidable_policy_verified':False}))

"""Actual native Stench/Inebriate enemy roster targeting and retained pipelines."""
from verify_pet_battle_cheat_death_release import ins,sites,pairs,reaches,n
from pet_accuracy_roster_scope import verify
import hashlib,json
verify()
selector=ins('?SelectTargets@PetBattleAbilityEffect@@')
gates={value:next(i for i,x in enumerate(selector)if(x.mnemonic,x.op_str)==('cmp','eax, '+hex(value)))for value in(321,527,1010)}
common=selector[gates[527]+1];assert common.mnemonic=='je'
prop=next(i for i,x in enumerate(selector)if x.address==common.operands[0].imm)
assert selector[gates[321]+1].operands[0].imm==selector[prop].address
assert selector[gates[1010]+1].mnemonic=='jne'and gates[1010]+2==prop
assert (selector[prop].mnemonic,selector[prop].op_str)==('cmp','cx, 0x32')and selector[prop+1].mnemonic=='jne'
body=[x for x in selector if x.address in reaches(selector,selector[prop+2].address)]
assert {('movzx','r9d, byte ptr [r8 + 0x14]'),('dec','r9d'),('shr','r9d, 1'),('mov','eax, dword ptr [rbx + 0x1c]'),('mov','eax, 0xd0'),('mov','r8d, 0xd8'),('cmp','dword ptr [rcx + 0x12c], 0'),('cmove','eax, r8d'),('cmp','r9d, dword ptr [rdx + 0x58]'),('mov','rdx, qword ptr [rdx + rax*8 + 0x28]'),('test','rdx, rdx'),('mov','edx, dword ptr [rdx + 0x130]')}<=set(pairs(body)),'Actual odd-order slot, enemy team, count/non-null and real recipient ID remain native'
assert len(sites(body,'?AddTarget@PetBattleAbilityEffect@@'))==1
assert ('mov','eax, dword ptr [rcx + 0x18]')in pairs(selector[:gates[321]])
print('PASS actual linked527/1010 property50 fixed odd-order enemy slots through real count/non-null/ID/living admission; retained321 and complete earlier pet linked scopes pass; native48 cases and client UI remain separate')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'roots':[527,1010],'auras':[526,1009],'native_cases':48}))

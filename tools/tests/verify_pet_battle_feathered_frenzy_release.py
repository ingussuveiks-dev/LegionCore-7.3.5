"""Actual linked1398 self recipient,1399 parent and scoped lethal222 continuation."""
from verify_pet_battle_variance_release import ins,sites,pairs,reaches,n
from pet_feathered_frenzy_scope import verify
import hashlib,json
verify()
selector=ins('?SelectTargets@PetBattleAbilityEffect@@')
gate=next(i for i,x in enumerate(selector)if(x.mnemonic,x.op_str)==('cmp','eax, 0x576'))
branch=selector[gate+1];assert branch.mnemonic=='je'
start=next(i for i,x in enumerate(selector)if x.address==branch.operands[0].imm)
assert pairs(selector[start:start+2])==[('mov','edx, dword ptr [rbx + 0x1c]'),('mov','rcx, rbx')]
assert start+2 in sites(selector,'?AddTarget@PetBattleAbilityEffect@@'),'Actual1398 type229 recipient is its native living user'
handler=ins('?HandleDefensiveTypeAura@PetBattleAbilityEffect@@')
gate=next(i for i,x in enumerate(handler)if(x.mnemonic,x.op_str)==('cmp','ecx, 0x576'))
assert handler[gate+1].mnemonic=='jne'and pairs(handler[gate+2:gate+3])==[('mov','r8d, 0x577')],'Only actual1398 selects its1399 parent; retained inlined holder/source guards are independently proved'
cast=ins('?Cast@PetBattle@@');execute,=sites(cast,'?Execute@PetBattleAbilityEffect@@')
gate=next(i for i,x in enumerate(cast)if(x.mnemonic,x.op_str)==('mov','r8d, 0xde'))
assert gate>execute and pairs(cast[gate-2:gate])==[('test','cl, cl'),('je',cast[gate-1].op_str)]
assert cast[gate-1].operands[0].imm==cast[gate+3].address and pairs(cast[gate+3:gate+4])==[('xor','al, al')]
assert pairs(cast[gate+1:gate+2])==[('cmp','ax, r8w')]and cast[gate+2].mnemonic=='je','Only issued self-type flag allows native222 damage continuation'
assert {('cmp','word ptr [r9 + 0x12], 0x7c'),('cmp','word ptr [r9 + 0x10], 0'),('cmp','dword ptr [rax], 0')}<=set(pairs(handler))
print('PASS actual linked1398 native living caster recipient,1399 real owned parent, preserved chain-failure guard and scoped222 lethal continuation; retained Discharge/variance and all earlier pet proofs pass')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'root':1398,'controller':1399,'mark':1420,'native_cases':24}))

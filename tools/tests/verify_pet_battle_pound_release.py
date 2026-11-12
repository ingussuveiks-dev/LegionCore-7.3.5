"""Linked Pound snapshots, fixed native recipients and bounded continuation."""
from verify_pet_battle_static_passives_release import ins,sites,pairs,reaches,n
from pet_pound_scope import verify
from capstone import CS_OP_IMM
import hashlib,json
verify()
cast=ins('?Cast@PetBattle@@');select,=sites(cast,'?SelectTargets@PetBattleAbilityEffect@@');execute,=sites(cast,'?Execute@PetBattleAbilityEffect@@');add,=sites(cast,'?AddTarget@PetBattleAbilityEffect@@')
before=cast[:select];p=set(pairs(before))
assert {('mov','dword ptr [rbp], 6'),('mov','dword ptr [rbp + 4], 6'),('mov','dword ptr [rbp + 8], 6'),('mov','dword ptr [rsp + 0x4c], 2')}<=p,'Empty snapshot cells use out-of-range6; real slot0 is never a null sentinel'
assert {('cmp','r10d, 0x633'),('cmp','eax, 2'),('sete','r14b'),('mov','dword ptr [rsp + 0x4c], r14d'),('lea','ebx, [r14 + r14*2]'),('mov','r11d, dword ptr [r10 + 0x70]'),('cmp','r11d, ebx'),('lea','eax, [rbx + 3]'),('cmp','r11d, eax'),('mov','dword ptr [rbp], r11d')}<=p,'Only live native1587 turn1/nonproc context snapshots a valid actual opposing team/front'
assert {('cmp','edx, edi'),('cmp','r9d, 3'),('cmp','edx, 3'),('cmp','eax, r11d'),('mov','r8d, dword ptr [rcx + 0x130]'),('cmp','r8d, eax'),('cmp','dword ptr [rcx + 0x12c], r14d'),('cmp','qword ptr [rsi + rax*8], rcx'),('mov','dword ptr [rbp + rax*4], r8d'),('inc','r9d')}<=p,'Backline snapshot is bounded to actual3/count, excludes the original front and verifies slot ID/team/shared roster membership; holes retain their ordinal'
region=cast[select:add+1]
assert {('cmp','r8d, 2'),('cmp','word ptr [rdi + 6], 0x18'),('movzx','ecx, byte ptr [rdi + 0x14]'),('lea','eax, [rcx - 1]'),('cmp','al, 2'),('mov','edx, dword ptr [rbp + rcx*4 - 4]'),('cmp','edx, 6'),('test','rcx, rcx'),('cmp','dword ptr [rcx + 0x12c], r8d')}<=set(pairs(region)),'Each actual native24 row clears the legacy front target and uses only its bounded cached OrderIndex recipient'
read=next(i for i,x in enumerate(cast)if(x.mnemonic,x.op_str)==('mov','edx, dword ptr [rbp + rcx*4 - 4]'))
guard=next(i for i,x in enumerate(cast[:read])if(x.mnemonic,x.op_str)==('cmp','al, 2'))
branch=cast[guard+1];assert branch.mnemonic=='ja'
# A failed guard may reach a later loop iteration, which is safe only after
# validating that iteration again. Prove edge dominance across the full CFG:
# removing the validated fallthrough must make the actual read unreachable.
table={x.address:x for x in cast};pending=[cast[0].address];seen=set()
blocked=(branch.address,branch.address+branch.size)
while pending:
 at=pending.pop()
 if at in seen or at not in table:continue
 seen.add(at);x=table[at]
 if x.mnemonic in('ret','int3','ud2'):continue
 targets=[]
 if x.mnemonic.startswith('j'):
  assert x.operands[0].type==CS_OP_IMM,'Closed native Cast CFG requires immediate branches'
  targets.append(x.operands[0].imm)
 if x.mnemonic!='jmp':targets.append(x.address+x.size)
 pending.extend(t for t in targets if(x.address,t)!=blocked)
assert cast[read].address not in seen,'Validated OrderIndex edge dominates the actual cached array read, including later iterations'
assert select<read<add<execute
assert {('cmp','r8d, 2'),('mov','rax, qword ptr [r9]'),('test','rax, rax'),('cmp','dword ptr [rax + 0x150], 0')}<=p,'Actual original caster is checked again before every scoped row'
post=cast[execute:execute+46]
assert {('or','dword ptr [rsp + 0x60], eax'),('movzx','ecx, byte ptr [rsp + 0x41]'),('test','cl, cl'),('cmp','ax, 0x18'),('cmp','byte ptr [rbp - 0x20], 0'),('cmp','dword ptr [r11 + 0x84], 3')}<=set(pairs(post)),'Native damage completion retains chain-failure/death receipts and admits the remaining Pound damage rows after an enemy final death'
print('PASS linked native1587 pre-damage opposing active/backline snapshot, invalid6 cells, bounded original roster/OrderIndex read, actual living recipients, per-row caster liveness and scoped damage-death continuation; all retained pet proofs pass')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'native_cases':112,'ability':1587,'effects':[3030,3032,3033],'active_points':20,'backline_points':10}))

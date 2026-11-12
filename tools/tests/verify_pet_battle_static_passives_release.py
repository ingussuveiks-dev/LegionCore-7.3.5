"""Linked personal species183 state loading and idempotent native crit base."""
from verify_pet_battle_damage_threshold_release import ins,sites,pairs,reaches,n
from pet_static_passives_scope import verify
from five_ten_20261111_pe import decoder
from capstone import CS_OP_IMM,CS_OP_MEM
import hashlib,json
verify()
# Keep the shared64-byte unknown-leaf decoder limit unchanged. Independently
# bound and prove this named stack-free getter, including its closed loop CFG.
start=n['symbol']('?GetInitialCritChance@BattlePetInstance@@');end=min(a for a in n['symbols'].values()if a>start)
assert 0<end-start<=512
crit=list(decoder.disasm(n['binary'][n['offset'](start):n['offset'](start)+end-start],start))
assert crit and crit[0].address==start and crit[-1].address+crit[-1].size==end
while crit[-1].mnemonic=='int3':crit.pop()
assert crit[-1].mnemonic=='ret'
addresses={x.address for x in crit}
for x in crit:
 assert x.mnemonic not in ('call','push','pop','enter','leave')
 assert all(not(o.type==CS_OP_MEM and x.reg_name(o.mem.base)in('rsp','rbp'))for o in x.operands)
 if x.mnemonic.startswith('j'):assert x.operands[0].type==CS_OP_IMM and x.operands[0].imm in addresses
assert {('cmp','dword ptr [rcx + 0x44], 0x4eb'),('mov','r10d, 5'),('mov','r11d, 0xfb'),('cmp','dword ptr [rcx + 0x428], r11d'),('cmp','word ptr [rdx + 9], r11w'),('cmp','byte ptr [rdx + 8], 0x28'),('add','r10d, dword ptr [rdx + 4]'),('mov','eax, r10d')}<=set(pairs(crit)),'Only actual1259/183=251 adds the native40 row to the preserved base5'
for field in ('dword ptr [rcx + 0x44], 0x4eb','dword ptr [rcx + 0x428], r11d'):
 i=next(i for i,x in enumerate(crit)if(x.mnemonic,x.op_str)==('cmp',field));branch=next(x for x in crit[i+1:]if x.mnemonic.startswith('j'))
 assert branch.mnemonic=='jne' and pairs([next(x for x in crit if x.address==branch.operands[0].imm)])==[('mov','eax, r10d')]
add=ins('?AddPet@PetBattle@@');p=set(pairs(add))
assert {('mov','r15d, 0xfb'),('cmp','ecx, 0x4eb'),('cmp','ebx, r15d'),('mov','esi, 0x29'),('cmp','ecx, 0x58c'),('cmp','ebx, 0x1e5'),('mov','esi, 0x17'),('cmp','ecx, 0x759'),('cmp','ebx, 0x689'),('mov','esi, 0x49'),('cmp','eax, ebx'),('cmp','eax, esi'),('mov','eax, dword ptr [r11 + 4]'),('add','dword ptr [rcx + rdx*4 + 0x14c], eax')}<=p,'Actual native personal bindings select41/23/73 and write only matching ability-state Value'
assert {('mov','r10d, 5'),('cmp','dword ptr [rax + 0x44], 0x4eb'),('cmp','dword ptr [rax + 0x428], r15d'),('cmp','word ptr [r8 + 9], r15w'),('cmp','byte ptr [r8 + 8], 0x28'),('add','r10d, dword ptr [r8 + 4]'),('mov','dword ptr [rax + 0x1ec], r10d')}<=p,'Actual AddPet includes the same native crit value before Begin and initial snapshots'
initialize=ins('?HandleInitialization@PetBattleAbilityEffect@@');get,=sites(initialize,'?GetInitialCritChance@BattlePetInstance@@')
setcrit=next(i for i in sites(initialize,'?SetState@PetBattleAbilityEffect@@')if i>get)
assert {('mov','r9d, eax'),('mov','r8d, 0x28'),('mov','edx, dword ptr [rbx + 0x3c]')}<=set(pairs(initialize[get:setcrit])),'Native291 restores the getter result to the actual target40, rather than overwriting the passive with5'
print('PASS linked actual personal species1259/251,1420/485,1881/1673 native state41/23/73 loading before snapshots, shared bounded base5+native40 getter and idempotent actual291 target crit; all retained pet proofs pass')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'native_cases':120,'abilities':[251,485,1673],'verified_stack_free_leaf_bytes':end-start}))

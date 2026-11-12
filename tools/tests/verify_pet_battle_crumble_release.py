"""Bounded native Rocko source, self controller and authored removal lifecycle."""
from verify_pet_battle_final_health_release import ins,sites,pairs,n
from verify_pet_battle_lifecycle_release import reaches
from pet_crumble_scope import verify
from five_ten_20261111_pe import decoder
from capstone import CS_OP_IMM,CS_OP_MEM
import hashlib,json
verify()
# Independently bound this specific stack-free leaf; retain the shared64-byte
# fallback and all earlier handler-table/CFG bounds unchanged.
start=n['symbol']('?IsCrumbleSource@PetBattle@@');end=min(a for a in n['symbols'].values()if a>start)
assert 0<end-start<=512
source=list(decoder.disasm(n['binary'][n['offset'](start):n['offset'](start)+end-start],start))
assert source and source[0].address==start and source[-1].address+source[-1].size==end
while source[-1].mnemonic=='int3':source.pop()
assert source[-1].mnemonic=='ret'
addresses={x.address for x in source}
for x in source:
 assert x.mnemonic not in('call','push','pop','enter','leave')
 assert all(not(o.type==CS_OP_MEM and x.reg_name(o.mem.base)in('rsp','rbp'))for o in x.operands)
 if x.mnemonic.startswith('j'):assert x.operands[0].type==CS_OP_IMM and x.operands[0].imm in addresses
assert {('cmp','edx, 6'),('cmp','dword ptr [r8 + 0x130], edx'),('cmp','dword ptr [r8 + 0x44], 0x713'),('cmp','dword ptr [r8 + 0x428], 0x632'),('cmp','dword ptr [r8 + 0x150], 0'),('cmp','byte ptr [r8 + 0x129], 0'),('cmp','byte ptr [r8 + 0x128], 0'),('cmp','eax, 2'),('test','rcx, rcx'),('cmp','eax, edx'),('cmp','qword ptr [rcx], r8'),('cmp','eax, 3')}<=set(pairs(source))
read=next(x for x in source if(x.mnemonic,x.op_str)==('mov','r8, qword ptr [rcx + rax*8]'))
assert source[1].mnemonic=='jae'and read.address not in reaches(source,source[1].operands[0].imm),'Invalid source ID cannot reach the indexed pet read'
begin=ins('?Begin@PetBattle@@');guard,=sites(begin,'?IsCrumbleSource@PetBattle@@');casts=sites(begin,'?Cast@PetBattle@@')
issued=next(i for i in casts if('mov','r8d, 0x632')in pairs(begin[i-10:i]))
assert guard<issued and {('mov','dword ptr [rsp + 0x30], 0xffffffff'),('xor','r9d, r9d'),('mov','dword ptr [rsp + 0x28], 0xffffffff'),('mov','dword ptr [rsp + 0x20], ebp'),('mov','edx, ebx')}<=set(pairs(begin[guard:issued]))
assert all(issued<i for i in sites(begin,'?SendPetBattleInitialUpdate@WorldSession@@')),'Real native producer precedes actual first snapshots'
selector=ins('?SelectTargets@PetBattleAbilityEffect@@');guard,=sites(selector,'?IsCrumbleSource@PetBattle@@');add=min(i for i in sites(selector,'?AddTarget@PetBattleAbilityEffect@@')if i>guard)
assert {('cmp','eax, 0x632'),('cmp','word ptr [rcx + 6], 0x32'),('cmp','dword ptr [rcx + 0x15], 0xc9a')}<=set(pairs(selector[:guard]))
assert pairs(selector[add-2:add])==[('mov','edx, dword ptr [rbx + 0x1c]'),('mov','rcx, rbx')],'Actual3226 uses its real caster as recipient'
negative=ins('?HandleNegativeAura@PetBattleAbilityEffect@@');guard,=sites(negative,'?IsCrumbleSource@PetBattle@@');add,=sites(negative,'?AddAura@PetBattle@@')
assert {('cmp','dword ptr [rcx + 0x18], 0x632'),('cmp','dword ptr [rax + 0x15], 0xc9a'),('cmp','dword ptr [rcx + 0x3c], edx')}<=set(pairs(negative[:guard]))
assert {('cmp','dword ptr [rdx], 0x6cb'),('cmp','dword ptr [rdx + 8], r8d'),('cmp','dword ptr [rdx + 0xc], r8d')}<=set(pairs(negative[guard:add]))
dup=next(i for i,x in enumerate(negative)if(x.mnemonic,x.op_str)==('cmp','dword ptr [rdx + 0xc], r8d'))
branch=negative[dup+1];assert branch.mnemonic=='je'and negative[add].address not in reaches(negative,branch.operands[0].imm),'Existing source controller cannot reach refresh/add,including after expiry'
terminal=ins('?HandleKill@PetBattleAbilityEffect@@');gate=next(i for i,x in enumerate(terminal)if(x.mnemonic,x.op_str)==('cmp','r8d, 0x6cb'))
body=[x for x in terminal if x.address in reaches(terminal,terminal[gate+2].address)]
kill,=sites(body,'?Kill@PetBattleAbilityEffect@@');guard,=sites(body,'?IsCrumbleSource@PetBattle@@')
assert {('cmp','dword ptr [rax + 0x15], 0xc9b'),('cmp','dword ptr [rbx + 0x3c], edx'),('cmp','dword ptr [rbx + 0x4c], edx'),('test','dword ptr [rbx + 0x44], 0x29a'),('cmp','byte ptr [rdx + 0x20], 0'),('cmp','dword ptr [rdx], 0x6cb'),('cmp','dword ptr [rdx + 8], r9d'),('cmp','dword ptr [rdx + 0xc], r9d'),('cmp','dword ptr [rdx + 4], 0xc9a'),('cmp','dword ptr [rdx + 0x1c], 9'),('test','r8b, r8b')}<=set(pairs(body[:kill]))
assert pairs(body[kill-2:kill])==[('mov','edx, dword ptr [rbx + 0x1c]'),('mov','rcx, rbx')],'Issued removal kills its actual source only'
assert body[guard+2].mnemonic=='je'and body[kill].address not in reaches(body,body[guard+2].operands[0].imm),'Rejected source cannot reach terminal kill'
print('PASS linked native1811/183=1586 bounded source,initial3226 self controller,once-only countdown,issued1739/3227 actual source-only removal and scoped early-death ordering;all retained proofs pass;immunity/client scopes separate')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'native_lifecycle_cases':76,'duration_from_native3226':9,'natural_expiry_round':10,'verified_stack_free_leaf_bytes':end-start,'immunity_repaired':False}))

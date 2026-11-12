"""Actual linked native1504/183/1389 passive source and owned proc dispatch."""
from verify_pet_battle_feathered_frenzy_release import ins,sites,pairs,reaches,n
from pet_roboflauge_scope import verify
from five_ten_20261111_pe import decoder
from capstone import CS_OP_IMM,CS_OP_MEM,CS_OP_REG
import hashlib,json
verify()
# This pure predicate is a real stack-free x64 leaf longer than64 bytes and
# therefore has no .pdata unwind entry. Keep the general ins() limit intact;
# independently prove this particular next-symbol bound and closed leaf CFG.
start=n['symbol']('?IsRoboflaugeSource@PetBattle@@')
end=min(a for a in n['symbols'].values()if a>start)
assert 0<end-start<=512
source=list(decoder.disasm(n['binary'][n['offset'](start):n['offset'](start)+end-start],start))
assert source and source[0].address==start and source[-1].address+source[-1].size==end
while source[-1].mnemonic=='int3':source.pop()
assert source[-1].mnemonic=='ret'
addresses={x.address for x in source}
for x in source:
 assert x.mnemonic not in('call','push','pop','enter','leave')
 assert all(not(o.type==CS_OP_MEM and x.reg_name(o.mem.base)in('rsp','rbp'))for o in x.operands),'Verified leaf has no hidden stack frame'
 if x.mnemonic.startswith('j'):
  assert x.operands[0].type==CS_OP_IMM and x.operands[0].imm in addresses,'Every leaf branch stays within proved instruction bounds'
assert {('cmp','edx, 6'),('cmp','dword ptr [r8 + 0x130], edx'),('cmp','dword ptr [r8 + 0x44], 0x5e0'),('cmp','dword ptr [r8 + 0x428], 0x56d'),('cmp','dword ptr [r8 + 0x150], 0'),('cmp','byte ptr [r8 + 0x129], 0'),('cmp','byte ptr [r8 + 0x128], 0'),('cmp','eax, 2'),('test','rcx, rcx'),('cmp','eax, edx'),('cmp','qword ptr [rcx], r8'),('cmp','eax, 3')}<=set(pairs(source))
firstRead=next(x for x in source if(x.mnemonic,x.op_str)==('mov','r8, qword ptr [rcx + rax*8]'))
assert source[1].mnemonic=='jae'and firstRead.address not in reaches(source,source[1].operands[0].imm),'Actual source bounds dominate the indexed read'
selector=ins('?SelectTargets@PetBattleAbilityEffect@@')
guard,=sites(selector,'?IsRoboflaugeSource@PetBattle@@')
region=selector[guard:next(i for i,x in enumerate(selector)if i>guard and(x.mnemonic,x.op_str)==('cmp','eax, 0x1dc'))]
assert {('mov','ecx, dword ptr [rax + 0x12c]'),('mov','r13, qword ptr [rdx + rcx*8 + 0xd0]'),('cmp','edi, dword ptr [r13 + 0x58]'),('test','r14, r14'),('mov','r14d, dword ptr [r14 + 0x130]'),('cmp','r14d, 6'),('test','rcx, rcx'),('cmp','edi, 3')}<=set(pairs(region))
assert len(sites(region,'?IsAlive@BattlePetInstance@@'))==1,'Real recipient alive admission remains in the bounded own roster loop'
handler=ins('?HandleDefensiveTypeAura@PetBattleAbilityEffect@@');guard,=sites(handler,'?IsRoboflaugeSource@PetBattle@@');hit,=sites(handler,'?CalculateHit@PetBattleAbilityEffect@@')
assert guard<hit and ('cmp','dword ptr [rdx + 0x12c], ecx')in pairs(handler[guard:hit]),'Direct execution retains actual source authority and same-team ownership'
begin=ins('?Begin@PetBattle@@');calls=sites(begin,'?Cast@PetBattle@@')
initial=next(i for i in calls if('mov','r8d, 0x56d')in pairs(begin[i-10:i]))
assert all(initial<i for i in sites(begin,'?SendPetBattleInitialUpdate@WorldSession@@')),'Initial native marks precede actual initial snapshots'
rounds=ins('?ProceedRound@PetBattle@@');calls=sites(rounds,'?Cast@PetBattle@@')
renew=next(i for i in calls if{('mov','r8d, 0x56d'),('mov','dword ptr [rsp + 0x20], 6')}<=set(pairs(rounds[i-10:i])))
assert all(renew<i for i in sites(rounds,'?DoCasts@PetBattleTeam@@')),'Native passive renewal precedes ordinary ability actions'
swap=ins('?SwapPet@PetBattle@@');assert sites(swap,'?IsRoboflaugeSource@PetBattle@@')and ('cmp','dword ptr [rax + 0x12c], r12d')in pairs(swap)
cast=ins('?Cast@PetBattle@@');assert len(sites(cast,'?IsRoboflaugeSource@PetBattle@@'))==1
assert {('cmp','r8d, 0x56d'),('cmp','r14d, -1'),('cmp','dword ptr [rbp + 0x88], -1'),('mov','eax, 0x441'),('bt','eax, r15d')}<=set(pairs(cast)),'Actual native source receipt, empty aura-owner and exact apply/start/swap proc mask retained'
print('PASS linked native1504/183/1389 bounded real living roster source, same-team229 targets, initial-before-snapshot/round-before-actions/own-swap native callbacks and exact proc authority; all retained pet proofs pass')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'species':1504,'passive_state':183,'ability':1389,'native_cases':48,'verified_stack_free_leaf_bytes':end-start}))

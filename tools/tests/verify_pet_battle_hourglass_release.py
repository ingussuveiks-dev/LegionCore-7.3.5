"""Actual native Hourglass producer, owner, roster and round-end pipeline."""
from verify_pet_battle_bend_time_release import ins,sites,pairs,reaches,n
from pet_hourglass_scope import verify
import hashlib,json,struct
verify()
cast=ins('?Cast@PetBattle@@');select,=sites(cast,'?SelectTargets@PetBattleAbilityEffect@@');execute,=sites(cast,'?Execute@PetBattleAbilityEffect@@')
assert {('cmp','r8d, 0x78c'),('cmp','esi, 7'),('mov','ebx, dword ptr [rbp + 0x5f]'),('cmp','dword ptr [rdx], ebx'),('cmp','dword ptr [rdx + 8], r12d'),('cmp','dword ptr [rdx + 0xc], r11d'),('cmp','byte ptr [rdx + 0x20], 0'),('cmp','r11d, 5'),('mov','dword ptr [rbp - 0x25], r11d'),('cmp','r9d, 0x787'),('mov','byte ptr [rsp + 0x42], cl')}<=set(pairs(cast[:select]))
assert {('or','dword ptr [rsp + 0x48], eax'),('movzx','edx, byte ptr [rsp + 0x42]'),('test','dl, dl'),('cmp','ax, 0x18'),('cmp','dword ptr [r8 + 0x84], 3')}<=set(pairs(cast[execute:execute+55])),'Actual kill row continuation retains native chain-failure receipt'
producer=ins('?HandleForbiddenHourglass@PetBattleAbilityEffect@@')
assert {('test','dword ptr [rcx + 0x44], 0x29a'),('cmp','dword ptr [rcx + 0x18], 0x787'),('cmp','dword ptr [rax + 0x15], 0xd9b'),('test','byte ptr [rax], 1'),('cmp','dword ptr [rcx + 0x3c], eax'),('mov','eax, dword ptr [rcx + 0x1c]'),('mov','ecx, dword ptr [rax + 0x70]'),('movzx','edx, word ptr [rax + 0xa]'),('movzx','r9d, word ptr [rax + 0xe]'),('movzx','r8d, word ptr [rax + 0xc]')}<=set(pairs(producer))
alive,=sites(producer,'?IsAlive@BattlePetInstance@@');hit,=sites(producer,'?CalculateHit@PetBattleAbilityEffect@@');aura,=sites(producer,'?AddAura@PetBattleAbilityEffect@@')
assert alive<hit<aura and pairs(producer[alive+1:alive+3])==[('test','al, al'),('jne',producer[alive+2].op_str)]
assert producer[aura].address not in reaches(producer,producer[alive+2].operands[0].imm),'Immediate racial survival must not issue1932'
dispatch=ins('?Execute@PetBattleAbilityEffect@@')
table_load=next(x for x in dispatch if x.mnemonic=='lea'and x.op_str.startswith('r12, [rip + '))
table=table_load.address+table_load.size+table_load.operands[1].mem.disp
handler,target=struct.unpack_from('<QI',n['binary'],n['offset'](table)+230*16)
assert n['resolve'](handler)==producer[0].address and target==0,'Native230 is registered with NONE; only1927 explicit selection admits the scoped producer'
selector=ins('?SelectTargets@PetBattleAbilityEffect@@')
assert {('cmp','eax, 0x78c'),('movsxd','r11, dword ptr [rbx + 0x4c]'),('cmp','r11d, 5'),('lea','eax, [rsi - 1]'),('cmp','al, 5'),('cmp','dword ptr [rdx + 0x130], r11d'),('cmp','r9d, 3'),('mov','eax, 0xaaaaaaab'),('mul','ecx'),('shr','edx, 1'),('cmp','r8d, r9d'),('dec','edx'),('mov','edx, dword ptr [rdx + 0x130]')}<=set(pairs(selector)),'Six authored rows map by order/3 to fixed roster slots excluding actual holder'
assert sites(selector,'?AddTarget@PetBattleAbilityEffect@@'),'Actual native living-target admission remains in use'
rounds=ins('?ProceedRound@PetBattle@@');casts=sites(rounds,'?Cast@PetBattle@@')
issued=next(i for i in casts if ('mov','dword ptr [rsp + 0x20], 7')in pairs(rounds[i-9:i]))
assert {('mov','eax, dword ptr [rbx + 0xc]'),('mov','dword ptr [rsp + 0x30], eax'),('mov','r8d, dword ptr [rbx]'),('mov','edx, dword ptr [rbx + 8]'),('xor','r9d, r9d'),('mov','dword ptr [rsp + 0x28], 0xffffffff')}<=set(pairs(rounds[issued-10:issued]))
assert {('cmp','byte ptr [r12 + 0x20], 0'),('mov','eax, dword ptr [r12]'),('cmp','eax, 0x78c'),('mov','qword ptr [r15], r12'),('cmp','byte ptr [rbx + 0x20], 0')}<=set(pairs(rounds[:issued])),'Only originally issued unexpired1932 instances enter this phase'
tick=next(i for i,x in enumerate(rounds)if(x.mnemonic,x.op_str)==('add','rcx, 0x134'))
assert issued<tick and {('cmp','eax, -1'),('dec','eax'),('mov','dword ptr [rcx], eax')}<=set(pairs(rounds[tick:tick+16])),'Authored1 becomes native ready0 before the packet sender'
death=ins('?Kill@PetBattle@@');expire,=sites(death,'?Expire@PetBattleAura@@')
guard=next(i for i,x in enumerate(death[:expire])if(x.mnemonic,x.op_str)==('cmp','eax, 0x78c'))
assert death[guard+1].mnemonic=='je'and death[guard+1].operands[0].imm==death[expire+1].address
print('PASS actual linked native1927 final-kill receipt, scoped registered230 producer, actual owned1932 round-end7 snapshot, fixed two-other-living-pet246 roster selection, native1->0 cooldown timing, holder cleanup and selective death retention; retained Bend/racial/sacrifice/Unholy proofs pass')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'root':1927,'controller':1932,'native_cases':34}))

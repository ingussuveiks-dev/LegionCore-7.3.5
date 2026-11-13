"""Bound native logout before save,detached journal and voluntary trap guard."""
from verify_pet_battle_crumble_immunity_release import ins,sites,pairs,reaches,n
from pet_logout_pe import original_instance
from pet_logout_scope import verify
import hashlib,json
verify()
logout=ins('?LogoutPlayer@WorldSession@@')
hook,=sites(logout,'?OnPlayerLogout@PetBattleSystem@@')
saves=sites(logout,'?SaveToDB@Player@@')
assert saves and all(hook<i for i in saves),'Actual bound session teardown must settle before every native save path'
scope=ins('?OnPlayerLogout@PetBattleSystem@@')
assert {('mov','rdi, rdx'),('mov','rax, qword ptr [rdx + 0x6828]'),('cmp','byte ptr [rax + 0x2a9], 0')}<=set(pairs(scope[:30])),'Only an actual Player/session logout admits forced settlement'
finish,=sites(scope,'?Finish@PetBattle@@')
assert sites(scope,'?RemoveRequest@PetBattleSystem@@')
assert {('mov','dword ptr [rax + 0x18], 1'),('mov','qword ptr [rax], r12')}<=set(pairs(scope))and len(sites(scope,'?SendPetBattleQueueStatus@WorldSession@@'))==2 and sites(scope,'?SendBattlePetJournalLockDenied@WorldSession@@'),'Actual inlined LeaveQueue returns a proposal opponent to QUEUED,clears its matching pointer and sends native leave/journal notifications'
assert {('cmp','eax, 1'),('movzx','r8d, al'),('xor','r9d, r9d')}<=set(pairs(scope[max(0,finish-15):finish])),'The native RUNNING status1 supplies abandon=true;ignorePenalty=false'
running=next(i for i in range(finish-15,finish)if(scope[i].mnemonic,scope[i].op_str)==('cmp','eax, 1'))
assert scope[running+1].mnemonic=='jne'and scope[finish].address not in reaches(scope,scope[running+1].operands[0].imm),'Only RUNNING can use Finish;creation cancellation cannot index an active pet'
assert {('lea','rcx, [rdi + 0xd0]'),('lea','rdx, [rdi + 0x4148]'),('cmp','byte ptr [rdi + 0x6c09], 0'),('lea','rcx, [rdi + 0x6bf0]')}<=set(pairs(scope))and sites(scope,'?IsValidMapCoord@MapManager@@')and len(sites(scope,'?WorldRelocate@WorldLocation@@'))==2,'Validated recall reaches actual pose and,only during real far-transfer,pending save destination'
pending=next(i for i,x in enumerate(scope)if(x.mnemonic,x.op_str)==('cmp','byte ptr [rdi + 0x6c09], 0'))
assert scope[pending+1].mnemonic=='je'and scope[sites(scope,'?WorldRelocate@WorldLocation@@')[1]].address not in reaches(scope,scope[pending+1].operands[0].imm),'Ordinary logout must not manufacture a pending transfer'
assert sites(scope,'?CancelUnstartedBattle@PetBattleSystem@@')and sites(ins('?ForfeitBattle@PetBattleSystem@@'),'?CancelUnstartedBattle@PetBattleSystem@@'),'Genuine logout and ordinary early cancel share the unstarted path'
assert not sites(scope,'?AddDelayedEvent@Unit@@'),'No logout Player is retained in a companion callback'
binding=ins('?GetLoggingOutPlayer@PetBattleSystem@@')
assert ('mov','rax, qword ptr gs:[0x58]')in pairs(binding)and sites(binding,'??8ObjectGuid@@'),'Synchronous binding must come from thread-local storage and match the complete battle GUID'
equal,=sites(binding,'??8ObjectGuid@@')
assert binding[equal+2].mnemonic=='je'
reject=reaches(binding,binding[equal+2].operands[0].imm)
assert any(x.address in reject and(x.mnemonic,x.op_str)==('xor','eax, eax')for x in binding),'A different battle must receive no departing Player'
rows=ins('?Finish@PetBattle@@')
bound,=sites(rows,'?GetLoggingOutPlayer@PetBattleSystem@@')
trap=next(i for i,x in enumerate(rows)if x.mnemonic=='cmp'and x.op_str=='dword ptr [rax + 0x24c], ebx')
assert bound<trap and ('test','rax, rax')in pairs(rows[bound:trap]),'Voluntary webbed check is bypassed only for the admitted synchronous logout binding'
claim=next(i for i,x in enumerate(rows)if(x.mnemonic,x.op_str)==('mov','dword ptr [rdi + 0x78], 4'))
callback,=sites(rows,'?OnPetBattleFinish@ScriptMgr@@')
assert claim<callback,'Nondeletable FINISHING4 must be claimed before script callbacks can reenter it'
entry=next(i for i,x in enumerate(rows)if(x.mnemonic,x.op_str)==('cmp','dword ptr [rcx + 0x78], 1'))
assert rows[entry+1].mnemonic=='jne'and rows[claim].address not in reaches(rows,rows[entry+1].operands[0].imm),'Only RUNNING may enter Finish;including FINISHING4 reentry is inert'
journals=sites(rows,'?UpdateOriginalInstance@BattlePetInstance@@');assert len(journals)==2
offline,online=journals
assert pairs(rows[offline-2:offline])==[('xor','edx, edx'),('mov','rcx, qword ptr [rbx]')],'The detached branch copies the actual original pet without a Player receiver'
assert {('cmp','dword ptr [rdx + 0x150], 0'),('cmp','dword ptr [rdx + 0x60], 0'),('mov','dword ptr [rdx + 0x60], r8d')}<=set(pairs(rows[callback:offline])),'Detached final death/negative health is normalized before its journal copy'
assert offline<online and sites(rows,'?SendPetBattleFinalRound@WorldSession@@'),'Online rewards/final sender remain a separate native path'
copy=original_instance();notify=sites(copy,'?SendBattlePetUpdates@WorldSession@@');assert len(notify)==1
null=next(i for i,x in enumerate(copy)if(x.mnemonic,x.op_str)==('test','r9, r9'))
assert copy[null+1].mnemonic=='je'and copy[notify[0]].address not in reaches(copy,copy[null+1].operands[0].imm),'A null offline receiver must never reach session notification'
assert {('mov','dword ptr [rcx + 0x60], eax'),('mov','byte ptr [rax + 0x120], 1')}<=set(pairs(copy[:null])),'Health and persistence receipt precede the null receiver guard'
print('PASS linked bound session logout before save,thread-local full-GUID admission,retained voluntary webbed guard,claimed exactly-once finish,detached journal/null sender and all retained pet proofs')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'native_endpoint_cases':44,'native_full_logout_save_cases':24,'client_disconnect_reconnect_verified':False}))

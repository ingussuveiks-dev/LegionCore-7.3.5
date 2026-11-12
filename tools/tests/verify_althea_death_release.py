from linked_player_abi import unit_queue_offset,unit_queue_operand
"""Selected Althea death receiver and preserved Garona handoff."""
from pathlib import Path
import runpy,json,hashlib
from sharasdal_erunak_entry_pe import sites,values
p=runpy.run_path(str(Path(__file__).with_name('verify_conversation_world_admission_release.py')))
n,ins,reachable=(p[k] for k in ('n','ins','reachable'))
table,refs,resolved=(p['p'][k] for k in ('table','refs','resolved'))
pairs=lambda body:[(op.mnemonic,op.op_str) for op in body]
suffix='npc_sister_althea_ebonlockeAI@npc_sister_althea_ebonlocke@@'
body=ins('?JustDied@'+suffix)
owner,=sites(n,body,'?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@')
nearest,=sites(n,body,'?GetVictoryPlayer@'+suffix)
assert not sites(n,body,'?FindNearestPlayer@WorldObject@@')
credit,=sites(n,body,'?UpdateAchievementCriteria@Player@@')
create,=sites(n,body,'?CreateConversation@Conversation@@')
garona,=sites(n,body,'?FindNearestCreature@WorldObject@@')
queue,=sites(n,body,'?AddDelayedEvent@FunctionProcessor@@')
assert owner<nearest<credit<create<garona<queue
assert pairs(body[owner-3:owner])==[('test','rdx, rdx'),('je',body[owner-2].op_str),('mov','rcx, rdx')]
assert body[owner].address not in reachable(body,body[owner-2].operands[0].imm)
assert {('mov','rdi, rdx'),('test','rax, rax'),('cmovne','rdi, rax')}<=set(pairs(body[:nearest]))
assert {('mov','edx, 0x5c'),('mov','r8d, 0xc99d'),('mov','rcx, rax')}<=set(pairs(body[nearest:credit]))
guard=next(i for i,op in enumerate(body[credit:create],credit) if op.mnemonic=='test' and op.op_str=='rdi, rdi')
assert body[guard+1].mnemonic=='je'
null=reachable(body,body[guard+1].operands[0].imm)
assert body[create].address not in null and body[garona].address in null and body[queue].address in null
assert {('lea','rbx, [rdi + 0xd0]'),('mov','r9, rdi'),('mov','r8d, 0x97f'),('mov','qword ptr [rsp + 0x28], rbx')}<=set(pairs(body[guard:create]))
assert body[create+1].op_str=='al, al' and body[create+2].mnemonic=='jne'
assert ('call','qword ptr [rax]') in pairs(body[create+3:garona])
assert {108214,5000}<=values(body[garona-5:queue])
assert {('mov','qword ptr [rsp + 0x58], rax'),('lea',unit_queue_operand('rcx, [rax + 0x36a0]'))}<=set(pairs(body[garona:queue]))
vt,raw=table('??_7'+suffix+'6B@',82)
assert resolved(raw)[0x130//8]==n['symbol']('?JustDied@'+suffix)
assert vt in refs(ins('?GetAI@npc_sister_althea_ebonlocke@@'))
assert any(op.mnemonic=='call' and op.op_str.endswith('+ 0x130]') for op in ins('?Kill@Unit@@'))
print('PASS registered Althea death130/native Kill dispatch: null skips owner lookup; successful owner replaces pet/charmed speaker, unresolved NPC retains original speaker. Qualified native GetVictoryPlayer precedes the sole51613 call. Null speaker skips2431 allocation/position binding and still queues original Garona108214/5s. False factory delete retained. Recipient eligibility, scoped corpse actor and2432 voice data have separate regressions; no client/full scenario claim.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'scope':'Althea null/owner speech and qualified victory dispatch'}))

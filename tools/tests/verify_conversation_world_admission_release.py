"""Actual Conversation factory success requires native world admission."""
from pathlib import Path
import hashlib,json,runpy
from sharasdal_erunak_entry_pe import sites
p=runpy.run_path(str(Path(__file__).with_name('verify_ragnvald_second_battle_release.py')))
n,ins,reachable=(p[k] for k in ('n','ins','reachable'))
body=ins('?CreateConversation@Conversation@@')
pairs=lambda rows:[(op.mnemonic,op.op_str) for op in rows]
added,=sites(n,body,'??$AddToMap@VConversation@@@Map@@')
world,=sites(n,body,'?IsInWorld@Object@@')
remove,=sites(n,body,'??$RemoveFromMap@VConversation@@@Map@@')
success,=[i for i,op in enumerate(body) if op.mnemonic=='mov' and op.op_str=='al, 1']
assert added<world<remove<success
assert body[added+1].op_str=='al, al' and body[added+2].mnemonic=='jne'
assert body[world].address in reachable(body,body[added+2].operands[0].imm)
assert body[success].address not in reachable(body,body[added+3].address)
assert pairs(body[world-1:world])==[('mov','rcx, rdi')]
assert body[world+1].op_str=='al, al' and body[world+2].mnemonic=='jne'
assert body[world+2].operands[0].imm==body[success].address
assert pairs(body[remove-3:remove])==[('xor','r8d, r8d'),('mov','rdx, rdi'),('mov','rcx, qword ptr [rdi + 8]')]
assert body[success].address not in reachable(body,body[remove+1].address)
assert ('xor','al, al') in pairs([op for op in body if op.address in reachable(body,body[remove+1].address)])
assert not sites(n,body[world+3:remove],'??3@'), 'Factory retains caller allocation on false'
assert ('mov','rdx, rdi') in pairs(body[added-3:added])
print('PASS actual linked native Conversation factory: AddToMap success reaches IsInWorld; only admitted world object returns true. Refused admission calls real Map::RemoveFromMap(this,false) and returns false without deleting caller allocation; predecessor/native payload/normal controls and preceding artifact PE retained. DB-spawn loader/Player/client/full chains remain separate.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'scope':'CreateConversation','caller_retains_failed_allocation':True}))

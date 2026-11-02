"""Verify actual instance-owned first-wave death and native criteria receipts."""
from pathlib import Path
import hashlib,json,runpy
from sharasdal_erunak_entry_pe import sites,values

p=runpy.run_path(str(Path(__file__).with_name('verify_warrior_intro_handoffs_release.py')))
n,ins,refs,table,resolved,reachable=(p[k] for k in ('n','ins','refs','table','resolved','reachable'))
def pairs(body):return [(op.mnemonic,op.op_str) for op in body]
def needs(body,*names):
    for name in names:assert sites(n,body,name),('Missing actual first-wave dependency',name)
scope='instance_broken_shore_InstanceMapScript@instance_broken_shore@@'
death='?Death@State@WarriorFirstWave@@'
update='?Update@State@WarriorFirstWave@@'
current='?Current@State@WarriorFirstWave@@'
progress='?Progress@State@WarriorFirstWave@@'
leaf='?Leaf@State@WarriorFirstWave@@'
vt,raw=table('??_7'+scope+'6B@',80)
assert vt in refs(ins('?GetInstanceScript@instance_broken_shore@@'))
for callback,dependency in (('CreatureDies',death),('Update',update)):
    body=ins('?'+callback+'@'+scope)
    assert n['symbol']('?'+callback+'@'+scope) in resolved(raw)
    assert len(sites(n,body,dependency))==1
hook=ins('?CreatureDies@'+scope)
first,=sites(n,hook,death);reinforcement,=sites(n,hook,'?Death@State@WarriorReinforcement@@')
finale,=sites(n,hook,'?MalgalorDeath@'+scope)
assert first<reinforcement<finale
assert pairs(hook[first-1:first])==[('add','rcx, 0x348')]
assert ('lea','rcx, [rbp + 0x2e0]') in pairs(hook[first:reinforcement])
needs(ins('?Update@'+scope),'?Update@State@WarriorReinforcement@@')

body=ins(death);capture,=sites(n,body,'?AddQuestDelayedEvent@Player@@')
needs(body,progress,'?Quest@MalgalorFinale@@','?GetQuestStatus@Player@@',
      '?GetReqKillOrCastCurrentCount@Player@@','?InSamePhase@WorldObject@@',
      '?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@',update)
for operand,jump in (('byte ptr [r12 + 0x34], 0','jne'),('eax, 1','jne'),
                     ('qword ptr [r15 + 8], rax','jne'),('dword ptr [r15 + 0xa18], 0','je'),
                     ('eax, 0x1923a','jne'),('qword ptr [r15 + 0x3a88], 0x582b2','jne')):
    at=next(i for i,op in enumerate(body[:capture]) if op.mnemonic=='cmp' and op.op_str==operand)
    assert body[at+1].mnemonic==jump
    assert body[capture].address not in reachable(body,body[at+1].operands[0].imm)
for dependency,jump in (('?IsInWorld@Object@@','je'),('?IsPreDelete@Object@@','jne'),('?HasFlag@Object@@','jne')):
    at,=[i for i in sites(n,body[:capture],dependency) if ('mov','rcx, r15') in pairs(body[i-3:i])]
    assert pairs(body[at+1:at+2])==[('test','al, al')] and body[at+2].mnemonic==jump
    assert body[capture].address not in reachable(body,body[at+2].operands[0].imm)
assert {102970,361138,103661,0x380}<=values(body)
assert {('movups','xmm1, xmmword ptr [rax]'),('movups','xmmword ptr [rdx], xmm1'),
        ('mov','dword ptr [rdx + 0x10], esi'),('mov','dword ptr [rdx + 0x14], ecx'),
        ('lock inc','dword ptr [rax + 0xc]')}<=set(pairs(body))
needs(ins(current),'?Original@MalgalorFinale@@','?GetQuestStatus@Player@@',
      '?GetReqKillOrCastCurrentCount@Player@@','?GetCreature@Map@@','?InSamePhase@WorldObject@@')
assert 103661 in values(ins(current))
assert {1500,933}<=values(ins(progress))
needs(ins(progress),'?GetScenario@ScenarioMgr@@')

body=ins(update);events=sites(n,body,'?UpdateAchievementCriteria@Player@@')
assert len(events)==2
assert {46878,47474,45771,46175,2000,92}<=values(body)
needs(body,progress,current,leaf,'?outMessage@Log@@')
needs(body,'?Retained@State@WarriorFirstWave@@')
assert b'Warrior first wave: real361138 death46878/47474 not acknowledged' in n['binary']
one,two=events
assert ('mov','r8d, 0xb71e') in pairs(body[one-6:one]) and ('mov','rcx, r14') in pairs(body[one-3:one])
assert ('mov','r8d, 0xb972') in pairs(body[two-6:two]) and ('mov','rcx, r15') in pairs(body[two-3:two])
assert sites(n,body[one+1:two],progress) and sites(n,body[one+1:two],current)
assert pairs(body)[one-9:one-7]==[('test','al, al'),('jne',body[one-8].op_str)]
assert body[one-8].operands[0].imm==body[one+1].address,'Accepted45771 must skip only its own native46878 event'
# All gates immediately before the second event jump past it, retaining the
# freshly resolved Player, native current step and both actual leaf tests.
for operand,mnemonic,jump in (('eax, 1','cmp','jne'),('r15, r15','test','je'),('al, al','test','je')):
    at=next(i for i in range(one+1,two) if body[i].mnemonic==mnemonic and body[i].op_str==operand)
    assert body[at+1].mnemonic==jump
    assert body[at+1].operands[0].imm==body[two+1].address
last=two-9
assert pairs(body[last:last+2])==[('test','al, al'),('jne',body[last+1].op_str)]
assert body[last+1].operands[0].imm==body[two+1].address,'Accepted46175 must skip its own native47474 event'
assert ('mov','dword ptr [rbx + 0x30], 0x7d0') in pairs(body)
for forbidden in ('?KilledMonsterCredit@Player@@','?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@','?FindNearestPlayer@WorldObject@@'):
    assert not sites(n,body,forbidden) and not sites(n,ins(death),forbidden)
danica=ins('?IsSummonedBy@npc_danica_the_reclaimerAI@')
assert not sites(n,danica,'?FindNearestPlayer@WorldObject@@')
assert not sites(n,danica,'?UpdateAchievementCriteria@Player@@'),'Danica summon must not own first-wave player credit'
print('PASS actual registered shared-instance death/update: real102970/spawn361138, map1500/native933 step1/dead/world/predelete/immunity guards precede original quest-lifetime capture. Full128-bit participant GUID and weak attempt, owner/pet/guardian priority, refreshed original Player and actual45771/46175 receipts gate native46878/47474,2000ms retry skips accepted leaves. Danica summon contains no nearest-player or first-wave credit call; existing Paladin/reinforcement/finale and2/3s actor paths retained. No character/item/quest grants; full933/client/opening and ritual remain separate.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'actual_death':102970,'spawn':361138,'native_events':[46878,47474]}))

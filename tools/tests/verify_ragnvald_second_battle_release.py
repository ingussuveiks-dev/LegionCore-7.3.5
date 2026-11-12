from linked_player_abi import inherited_operand
from linked_player_abi import player_offset,player_operand
"""Prove the registered native933 original-participant second-battle speech owner."""
from pathlib import Path
import hashlib,json,runpy
from sharasdal_erunak_entry_pe import sites,values,fields

p=runpy.run_path(str(Path(__file__).with_name('verify_warrior_ritual_release.py')))
n,ins,refs,table,resolved,reachable=(p[k] for k in ('n','ins','refs','table','resolved','reachable'))
def pairs(body):return [(op.mnemonic,op.op_str) for op in body]
def method(name):return '?'+name+'@State@RagnvaldSecondBattle@@'
def needs(body,*targets):
    for target in targets:assert sites(n,body,target),('Native second-battle dependency',target)
def gate(body,operand,jump,target,after=0):
    at=next(i for i,op in enumerate(body[after:],after) if op.mnemonic in ('cmp','test') and op.op_str==operand)
    branch=at+1
    while body[branch].mnemonic in ('mov','lea','nop'):branch+=1
    assert body[branch].mnemonic==jump,(operand,body[branch].mnemonic)
    assert target not in reachable(body,body[branch].operands[0].imm),('Rejected path reaches receipt/factory',operand)

scope='instance_broken_shore_InstanceMapScript@instance_broken_shore@@'
vt,raw=table('??_7'+scope+'6B@',80);slots=resolved(raw)
for slot,name in ((0x98,'SetGuidData'),(0x130,'Update')):
    # Update's native slot is resolved from its registered vtable rather than assumed.
    if name=='Update':assert n['symbol']('?Update@'+scope) in slots
    else:assert slots[slot//8]==n['symbol']('?'+name+'@'+scope)
ctor=ins('??0'+scope)
assert {('mov','ecx, 0x60'),('mov','qword ptr [rax], rdi'),('mov','qword ptr [rsi + 0x520], rbx')}<=set(pairs(ctor))
update=ins('?Update@'+scope)
ritual,=sites(n,update,'?Update@State@WarriorRitual@@');speech,=sites(n,update,method('Update'))
assert ritual<speech and update[speech].mnemonic=='jmp'
assert ('mov','rcx, qword ptr [rdi + 0x520]') in pairs(update[ritual+1:speech])

# Begin is inlined in the native full-GUID bridge. It snapshots weak original
# participants after the consumed ritual, without externally creating speech.
begin=ins('?SetGuidData@'+scope)
mark=next(i for i,op in enumerate(begin) if op.mnemonic=='mov' and op.op_str=='byte ptr [rdi + 0x58], 1')
assert {93332,93334,46912,46929,3}<=values(begin)
assert {('lea','rbx, [rcx + 0x3d8]'),('mov','rdi, qword ptr [rcx + 0x520]'),
        ('movups','xmm0, xmmword ptr [r8]'),('movups','xmmword ptr [rdi + 0x18], xmm0'),
        ('movups','xmm0, xmmword ptr [rbx + 0x58]'),('movups','xmmword ptr [rdi + 0x28], xmm0'),
        ('lea','rsi, [rbx + 0xc8]'),('lea','rcx, [rdi + 0x38]'),
        ('mov','qword ptr [rdi + 8], rbx'),('mov','qword ptr [rdi + 0x10], rbp'),
        ('mov','dword ptr [rdi + 0x50], eax')}<=set(pairs(begin[:mark]))
needs(begin,'?Progress@State@WarriorRitual@@','?Leaf@State@WarriorRitual@@','??9ObjectGuid@@','??$_Assign_counted_range@PEAUMember@MalgalorFinale@@','?getMSTime@@')
for operand,jump in (('byte ptr [rdi + 0x58], 0','jne'),('qword ptr [rbx], rcx','jne'),
                     ('byte ptr [rbx + 0x106], 0','je'),('byte ptr [rbx + 0xfc], 0','jne'),
                     ('byte ptr [rbx + 0x105], 0','je'),('qword ptr [rsi], rax','je'),('eax, 3','jne')):
    gate(begin,operand,jump,begin[mark].address)
assert not sites(n,begin,'?CreateConversation@Conversation@@')

valid=ins(method('Valid'));actor=ins(method('Actor'));retained=ins(method('Retained'));contact=ins(method('Contact'))
assert {1500,933,3,4,46912,46929}<=values(valid)
needs(valid,'?GetScenario@ScenarioMgr@@','?GetCurrentStep@Scenario@@',method('Leaf'),method('Retained'),'??9ObjectGuid@@')
assert {('cmp','byte ptr [rcx + 0x18ae8], 0'),('cmp','byte ptr [rcx + 0x106], 0'),
        ('cmp','byte ptr [rcx + 0xfc], 0'),('lea','rdx, [rdi + 0x28]'),('lea','rdx, [rdi + 0x18]'),
        ('cmp','rax, qword ptr [rdi + 0x10]'),('cmp','eax, 3'),('cmp','eax, 4')}<=set(pairs(valid))
assert {97167,361094}<=values(actor)
needs(actor,'?GetCreature@Map@@','??8ObjectGuid@@','?IsInWorld@Object@@','?IsPreDelete@Object@@','?GetUInt32Value@Object@@')
needs(retained,'?FindPlayer@ObjectAccessor@@','?getQuestStatus@Player@@','?Quest@MalgalorFinale@@','?GetQuestStatus@Player@@','?GetQuestObjectiveData@Player@@')
assert {103661,3}<=values(retained) and any(op.mnemonic=='lock cmpxchg' for op in retained)
assert {('cmp','qword ptr [r8 + 0x18], rax'),('cmp','qword ptr [rdi + 8], rax')}<=set(pairs(retained))
needs(contact,'?FindPlayer@ObjectAccessor@@','?getQuestStatus@Player@@','?IsInWorld@Object@@','?CanContact@Player@@',
      '?InSamePhase@WorldObject@@','??8ObjectGuid@@','?canSeeOrDetect@WorldObject@@','?GetVisibilityRange@WorldObject@@','?IsWithinDistInMap@WorldObject@@')
assert {0x478,0x488,0x4a0,player_offset(0x6848),player_offset(0x6849),player_offset(0x6851)}<=fields(contact)
assert ('test',inherited_operand('byte ptr [rdi + 0x4ff0], 1')) in pairs(contact)

body=ins(method('Update'));factory,=sites(n,body,'?CreateConversation@Conversation@@')
accept=next(i for i,op in enumerate(body) if op.mnemonic=='mov' and op.op_str=='byte ptr [rdi + 0x59], 1')
assert {1412,2000,120000}<=values(body[:factory])
for operand,jump in (('byte ptr [rcx + 0x58], 0','je'),('byte ptr [rsi], 0','jne'),
                     ('byte ptr [rcx + 0x59], 0','jne'),('byte ptr [rcx + 0x5b], 0','jne'),('ecx, 0x7d0','jb')):
    gate(body,operand,jump,body[factory].address)
assert {('lea','rsi, [rcx + 0x5a]'),('mov','byte ptr [rsi], 1'),('mov','byte ptr [rdi + 0x5c], 1'),
        ('mov','dword ptr [rdi + 0x54], eax'),('mov','r9, r15'),('mov','r8d, 0x584'),
        ('mov','rcx, r14'),('mov','qword ptr [rsp + 0x20], rsi')}<=set(pairs(body[:factory]))
needs(body[:factory],method('Valid'),method('Actor'),method('Contact'),'?getMSTime@@')
assert body[factory+1].op_str=='al, al' and body[factory+2].mnemonic=='jne'
assert ('call','qword ptr [rax]') in pairs(body[factory+3:factory+12]),'False factory deletes its allocation'
assert body[accept].address not in reachable(body,body[factory+3].address)
receipt=body[factory+3:accept]
needs(receipt,method('Actor'),method('Valid'),method('Contact'),'?IsInWorld@Object@@','?GetConversation@ObjectAccessor@@',
      '?GetCreatures@Conversation@@','?GetCreature@Conversation@@','?GetLowPart@ObjectGuid@@','?GetHighPart@ObjectGuid@@')
for operand,jump in (('rbx, rbx','je'),('rbx, r15','jne'),('qword ptr [r14 + 8], rax','jne'),('rax, r14','jne'),('rax, 6','jae')):
    gate(body,operand,jump,body[accept].address,after=factory+3)
assert len(sites(n,body,method('Discard')))==2
assert pairs(body[accept+1:accept+2])[0][0]=='jmp'
assert ('mov','byte ptr [rax], 0') in pairs(body), 'RAII busy reset on ordinary completion'

discard=ins(method('Discard'));world,=sites(n,discard,'?IsInWorld@Object@@');remove,=sites(n,discard,'?Remove@Conversation@@')
grid,=sites(n,discard,'??$RemoveFromMap@VConversation@@@Map@@')
assert world<remove<grid and discard[world+2].mnemonic=='je'
assert {('mov','rcx, qword ptr [rbx + 8]'),('mov','r8b, 1'),('mov','rdx, rbx')}<=set(pairs(discard[remove+1:grid]))
assert discard[grid].address in reachable(discard,discard[world+2].operands[0].imm)
assert discard[remove].address not in reachable(discard,discard[world+2].operands[0].imm)

# Original source alone bypasses its raw1412 factory; its native battle still runs.
raw_actor=ins('?DoAction@npc_ragnvald_dragonbornAI@');raw_factory,=sites(n,raw_actor,'?CreateConversation@Conversation@@')
spawn=next(i for i,op in enumerate(raw_actor[:raw_factory]) if op.mnemonic=='cmp' and op.op_str==inherited_operand('qword ptr [rax + 0x3a88], 0x58286'))
assert raw_actor[spawn+1].mnemonic=='je'
branch=raw_actor[spawn+1].operands[0].imm
assert raw_actor[raw_factory].address not in reachable(raw_actor,branch)
battle,=sites(n,raw_actor,'?AttackBattleTarget@npc_ragnvald_dragonbornAI@')
assert raw_actor[battle].address in reachable(raw_actor,branch)
for candidate in (begin,body,valid,actor,retained,contact,discard):
    for forbidden in ('?FindNearestPlayer@WorldObject@@','?UpdateAchievementCriteria@Player@@','?KilledMonsterCredit@Player@@',
                      '?CompleteQuest@Player@@','?AddItem@Player@@','?TeleportTo@Player@@','?CastSpell@Unit@@','?AttackBattleTarget@'):
        assert not sites(n,candidate,forbidden)
for message in (b'native1412 factory refused',b'native1412 has no live original actor/participant receipt',
                b'native1412 selected a different actor',b'original1412 conversation not accepted within120s'):
    assert message in n['binary']
print('PASS registered instance-owned1412: consumed genuine ritual snapshot/full original GUID and weak group attempts, original source-only factory exclusion, after-battle native update,2s retry/120s bound, phase3..4 and real leaves, contact/privacy/native visibility, true world/accessor/single original actor receipt, false delete/true uninserted Map cleanup and accepted-once. Native combat/criteria/quest/item/teleport grants absent; component/native and client/full933 evidence remain separate.')
print(json.dumps({'exe_sha256':hashlib.sha256(n['binary']).hexdigest(),'instance_pointer':0x520,'state_bytes':96,'conversation':1412}))

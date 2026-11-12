from linked_player_abi import inherited_operand
from linked_player_abi import player_offset,player_operand
"""Verify the selected Garona intro AI, GUID resolution and owned event queue."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, instructions = previous['native'], previous['instructions']
scope = 'npc_garona_halforcenAI@npc_garona_halforcen@@'
factory = instructions('?GetAI@npc_garona_halforcen@@')
los = instructions('?MoveInLineOfSight@' + scope)
update = instructions('?UpdateAI@' + scope)


def sites(body, prefix):
    target = native['symbol'](prefix)
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == target]


tables = []
for entry in factory:
    if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        slots = struct.unpack_from('<82Q', native['binary'], native['offset'](address))
        targets = [native['resolve'](value) for value in slots]
        if native['symbol']('?UpdateAI@' + scope) in targets and native['symbol']('?MoveInLineOfSight@' + scope) in targets:
            tables.append(slots)
assert len(tables) == 1, 'Factory does not select repaired LOS and UpdateAI'
for method, caller in [('UpdateAI', '?Update@Creature@@'), ('MoveInLineOfSight', '?MoveInLineOfSight_Safe@CreatureAI@@')]:
    slot, = [i for i, value in enumerate(tables[0]) if native['resolve'](value) == native['symbol']('?' + method + '@' + scope)]
    assert any(entry.mnemonic in ('call', 'jmp') and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == slot * 8
               for entry in instructions(caller)), 'Native dispatcher does not use actual Garona callback slot'

from sharasdal_erunak_entry_pe import sites as linked_sites, values, floats
from merlis_intro_pe import reachable
method=lambda name:'?'+name+'@'+scope
eligible=instructions(method('OpeningPlayer'));recipient=instructions(method('GetRecipient'));leaf=instructions(method('OpeningLeaf'))
pairs=lambda rows:{(x.mnemonic,x.op_str) for x in rows}
assert any(entry.op_str=='ecx, 0x78' for entry in factory)
assert {108015,1620,1123,42504,42627}<=values(eligible)
for prefix,count in [('?IsInWorld@Object@@',2),('?IsPreDelete@Object@@',2),('?IsQuestObjectExpired@WorldObject@@',2),('?CanContact@Player@@',1),('?InSamePhase@WorldObject@@',1),('?CanTargetQuestObject@WorldObject@@',2),('?GetQuestRewardStatus@Player@@',1),('?GetQuestStatus@Player@@',2)]:
 assert len(linked_sites(native,eligible,prefix))==count,prefix
assert {('test',inherited_operand('byte ptr [rbx + 0x4ff0], 1')),('cmp','al, 4'),('cmp','qword ptr [rbx + 8], rax'),('cmp',player_operand('byte ptr [rbx + 0x6848], 0')),('cmp',player_operand('byte ptr [rbx + 0x6849], 0')),('cmp',player_operand('byte ptr [rbx + 0x6851], 0'))}<=pairs(eligible)
lookup,=linked_sites(native,recipient,'?GetPlayer@ObjectAccessor@@');identity,=linked_sites(native,recipient,'??8ObjectGuid@@');qualified,=linked_sites(native,recipient,method('OpeningPlayer'))
assert lookup<identity<qualified and recipient[qualified].mnemonic=='jmp'
assert recipient[identity+1].op_str=='al, al' and recipient[identity+2].mnemonic=='je'
assert recipient[qualified].address not in reachable(recipient,recipient[identity+2].operands[0].imm)
assert ('movups','xmm0, xmmword ptr [rbx + 0x39]') in pairs(recipient)
assert {1123}<=values(leaf) and linked_sites(native,leaf,'?GetCriteriaProgressMap@')
assert ('cmp','qword ptr [rax + 0x28], 0') in pairs(leaf)
assert linked_sites(native,los,method('OpeningPlayer'))[0]<linked_sites(native,los,'?CanUpdateCriteria@Scenario@@')[0]
assert {48526,49374,15000}<=values(los) and 20.0 in floats(native,los)
assert len(linked_sites(native,los,'?ScheduleEvent@EventMap@@QEAAXII'))==1
assert not linked_sites(native,los,'?AddDelayedEvent@FunctionProcessor@@')
# Actual accepted duration -> max(original15s floor, native duration) -> first
# EventMap argument; a refused/invalid payload cannot replace the floor.
created,=linked_sites(native,los,'?CreateConversation@Conversation@@')
assert los[created+1].op_str=='al, al' and los[created+2].mnemonic=='jne'
read,=[op for op in los if (op.mnemonic,op.op_str)==('mov','eax, dword ptr [r15 + 0x4dc]')]
assert read.address==los[created+2].operands[0].imm
assert read.address not in reachable(los,los[created+3].address)
assert {('mov','esi, 0x3a98'),('test','eax, eax'),('cmp','eax, esi'),('cmova','esi, eax'),('mov','r8d, esi')}<=pairs(los)
first,=linked_sites(native,los,'?ScheduleEvent@EventMap@@QEAAXII')
assert {1}<=values(los[first-5:first]) and 30000 not in values(los)
assert ('movups','xmmword ptr [rdi + 0x39], xmm0') in pairs(los)
execute,=linked_sites(native,update,'?ExecuteEvent@EventMap@@')
credits=linked_sites(native,update,'?UpdateAchievementCriteria@Player@@');assert len(credits)==2
checks=linked_sites(native,update,method('OpeningLeaf'));assert len(checks)==4
receivers=linked_sites(native,update,method('GetRecipient'));assert len(receivers)==4
assert checks[0]<credits[0]<receivers[1]<checks[1]<credits[1]<checks[2]<checks[3]
for at,asset in zip(credits,(51418,52672)):
 assert {asset,92}<=values(update[at-9:at])
 assert update[checks[credits.index(at)]+2].mnemonic=='jne'
remove=[at for at in linked_sites(native,update,'?DespawnOrUnsummon@Creature@@') if 4000 in values(update[max(0,at-7):at])]
assert len(remove)==1 and checks[3]<remove[0]
# Bound the current event iteration at the next ExecuteEvent call; the actual
# native scheduler separately proves that a scheduled1s retry is not due now.
def iteration(start):
 indexed={x.address:i for i,x in enumerate(update)};seen=set();todo=[start];stop=update[execute].address
 while todo:
  at=todo.pop()
  if at in seen or at not in indexed or at==stop:continue
  seen.add(at);i=indexed[at];op=update[i]
  if op.mnemonic.startswith('ret') or op.mnemonic in ('int3','ud2'):continue
  if op.mnemonic.startswith('j') and op.operands[0].type==CS_OP_IMM:
   todo.append(op.operands[0].imm)
   if op.mnemonic=='jmp':continue
  if i+1<len(update):todo.append(update[i+1].address)
 return seen
retry=linked_sites(native,update,'?ScheduleEvent@EventMap@@QEAAXII')
assert 1000 in values(update) and retry
for at in checks[2:]:
 assert update[at+1].op_str=='al, al' and update[at+2].mnemonic=='je'
 rejected=iteration(update[at+2].operands[0].imm)
 assert update[remove[0]].address not in rejected
 assert any(update[i].address in rejected for i in retry)
# The second timer is scheduled by the actual middle event, after its
# optional factory; this prevents a coarse tick/recovered recipient from
# consuming both speech phases in one update.
created=[i for i in linked_sites(native,update,'?CreateConversation@Conversation@@') if 2377 in values(update[i-10:i])]
assert len(created)==1
at=created[0];assert update[at+1].op_str=='al, al' and update[at+2].mnemonic=='jne'
read,=[op for op in update if (op.mnemonic,op.op_str)==('mov','eax, dword ptr [r14 + 0x4dc]')]
assert read.address==update[at+2].operands[0].imm
assert read.address not in iteration(update[at+3].address)
assert {('mov','r15d, 0x3a98'),('test','eax, eax'),('cmp','eax, r15d'),('cmova','r15d, eax'),('mov','r8d, r15d')}<=pairs(update)
second=[i for i in retry if ('mov','r8d, r15d') in pairs(update[max(0,i-6):i])]
assert len(second)==1 and {2}<=values(update[second[0]-6:second[0]])
assert update[second[0]].address in iteration(update[at+3].address)
assert {('lea','rax, [rcx + 0x72]'),('mov','byte ptr [rax], 1'),('mov','byte ptr [rax], 0'),('mov','byte ptr [rsi + 0x71], 1')}<=pairs(update)
for prefix in ('?AddItem@Player@@','?CompleteQuest@Player@@'):
 assert not linked_sites(native,update,prefix)
print('PASS registered opening Garona: qualified native Rogue/quest/scenario/source admission before one-shot; fullGUID fresh recipient plus delayed eligibility; accepted native2361/2377 durations with15s phase floors; second EventMap starts only from actual middle/failure recovery; original actors; missing-only51418/52672 and actual native counters before4s departure,1s no-receipt retry/current-iteration cleanup exclusion, safe reentry and no item/quest shortcut. Native/client clock and whole-chain boundaries remain separate.')

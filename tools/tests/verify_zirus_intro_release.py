from linked_player_abi import inherited_operand
"""Registered Zirus1884 qualification, acknowledged one-shot and stationary retry."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values, floats
from merlis_intro_pe import reachable

p = runpy.run_path(str(Path(__file__).with_name('verify_devourer_opening_release.py')))
n, ins = p['n'], p['ins']
method = lambda name: '?' + name + '@npc_devourer_zirusAI@npc_devourer_zirus@@'
pairs = lambda rows: {(x.mnemonic, x.op_str) for x in rows}
def refs(rows):
    return {x.address + x.size + o.mem.disp for x in rows for o in x.operands
            if o.type == CS_OP_MEM and o.mem.base == X86_REG_RIP}
text = (n['args'].release / 'worldserver.map').read_text(errors='replace')
table, = [int(x,16) for x in re.findall(r'\s+\w+:\w+\s+\?\?_7npc_devourer_zirus@@6BScriptObject@@@\s+([0-9a-fA-F]{16})',text)]
factory = ins('?GetAI@npc_devourer_zirus@@')
assert table in refs(ins('?AddSC_Devourer@@'))
assert struct.unpack_from('<12Q',n['binary'],n['offset'](table))[10] == n['symbol']('?GetAI@npc_devourer_zirus@@')
callbacks = {n['symbol'](method(x)) for x in ('Reset','MoveInLineOfSight','UpdateAI','EnterCombat','JustDied')}
tables = [a for a in refs(factory) if callbacks <= set(struct.unpack_from('<82Q',n['binary'],n['offset'](a)))]
assert len(tables)==1
assert {('mov','ecx, 0xd8'),('mov','qword ptr [rsi + 0xc8], rbp'),('mov','word ptr [rsi + 0xd0], bp'),('mov','byte ptr [rsi + 0x38], bpl')} <= pairs(factory)
eligible, attempt, update, reset, los, enter = [ins(method(x)) for x in ('IntroPlayer','TryIntro','UpdateAI','Reset','MoveInLineOfSight','EnterCombat')]
assert {105542,1607,1078,41924,1,3,4} <= values(eligible) and 25.0 in floats(n,eligible)
for target,count in [('?IsInWorld@Object@@',2),('?IsPreDelete@Object@@',2),('?IsQuestObjectExpired@WorldObject@@',2),('?CanTargetQuestObject@WorldObject@@',2),('?GetQuestStatus@Player@@',2),('?GetInstanceScript@WorldObject@@',1),('?CanContact@Player@@',1),('?InSamePhase@WorldObject@@',1),('?InSamePhaseId@WorldObject@@Q',1),('?GetExactDist2d@Position@@',1),('?IsWithinDistInMap@WorldObject@@',1),('?IsWithinLOSInMap@WorldObject@@',1),('?GetQuestRewardStatus@Player@@',1),('?GetScenario@ScenarioMgr@@',1),('?GetScenarioId@Scenario@@',1),('?GetCurrentStep@Scenario@@',1)]:
    assert len(sites(n,eligible,target))==count,target
assert {('cmp','dword ptr [rcx + 0xa18], 0'),('cmp','dword ptr [rbx + 0xa18], 0'),('test',inherited_operand('byte ptr [rbx + 0x4ff0], 1')),('cmp','al, 4'),('cmp','qword ptr [rbx + 8], rax')} <= pairs(eligible)
assert {('cmp','byte ptr [rdx + 0x16], 4'),('cmovne','rdx, rax')} <= pairs(los) and len(sites(n,los,method('TryIntro')))==2
create, = sites(n,attempt,'?CreateConversation@Conversation@@')
assert 1884 in values(attempt[:create]) and ('mov','r9, r13') in pairs(attempt[:create])
latch, = [x.address for x in attempt if (x.mnemonic,x.op_str)==('mov','byte ptr [rsi + 0x38], 1')]
assert attempt[create+1].op_str=='al, al' and attempt[create+2].mnemonic=='jne'
assert latch not in reachable(attempt,attempt[create+3].address)
assert latch in reachable(attempt,attempt[create+2].operands[0].imm)
lookup, = sites(n,attempt,'?GetPlayer@ObjectAccessor@@')
identity, = sites(n,attempt,'??9ObjectGuid@@')
receipts=sites(n,attempt,'?GetQuestCancellationReceipt@Player@@')
assert len(receipts)==2 and receipts[0]<create<lookup<identity<receipts[1]
assert len(sites(n,attempt,method('IntroPlayer')))==2
# Compiler carries all post-publication denials in r12b, then branches to
# Remove versus the only latch. Pin both flag assignments and their join.
denied, = [x.address for x in attempt if (x.mnemonic,x.op_str)==('mov','r12b, 1')]
assert attempt[identity+2].mnemonic=='jne' and attempt[identity+2].operands[0].imm==denied
assert {('movups','xmm0, xmmword ptr [rax]'),('movaps','xmmword ptr [rsp + 0x70], xmm0'),('cmp','r12d, dword ptr [rsi + 0xcc]'),('movzx','eax, byte ptr [rax]'),('cmp','qword ptr [rax], rcx'),('xor','r12b, r12b'),('test','r12b, r12b'),('mov','byte ptr [rbx], 0'),('mov','dword ptr [rsi + 0xc8], 0x3e8')} <= pairs(attempt)
remove, = sites(n,attempt,'?Remove@Conversation@@')
assert attempt[remove-3].op_str=='r12b, r12b' and attempt[remove-2].mnemonic=='je' and attempt[remove-2].operands[0].imm==latch
assert (attempt[remove-1].mnemonic,attempt[remove-1].op_str)==('mov','rcx, r15')
assert {('inc','dword ptr [rcx + 0xcc]'),('mov','dword ptr [rcx + 0xc8], 0'),('mov','byte ptr [rcx + 0xd1], 0')} <= pairs(reset)
assert not any(x.mnemonic=='mov' and x.op_str.startswith('byte ptr [') and '+ 0x38]' in x.op_str for x in reset)
assert sites(n,reset,'?Reset@EventMap@@') and sites(n,reset,'?DespawnAll@SummonList@@')
# UpdateIntro is inlined before the unchanged native combat dispatcher.
grid, = sites(n,update,'?GetPlayerListInGrid@WorldObject@@')
qualified, = sites(n,update,method('IntroPlayer'))
retry, = sites(n,update,method('TryIntro'))
victim, = sites(n,update,'?UpdateVictim@CreatureAI@@')
assert grid<qualified<retry<victim and len(sites(n,update,'?GetDistance@WorldObject@@'))==2
assert 25.0 in floats(n,update) and ('mov','dword ptr [rbp + 0xc8], 0x3e8') in pairs(update)
assert {194519,210157,8000,11000} <= values(update) and {8000,11000} <= values(enter)
assert len(sites(n,update,'?CastSpell@Unit@@'))==3 and sites(n,update,'?DoCast@UnitAI@@') and sites(n,update,'?ExecuteEvent@EventMap@@')
for body in (eligible,attempt,los,update):
    for forbidden in ('?UpdateAchievementCriteria@Player@@','?AddItem@Player@@','?CompleteQuest@Player@@'):
        assert not sites(n,body,forbidden)
print('PASS selected registered Zirus1884: qualified Rogue/quest1078-step3/full scope/phase/LOS/25yd admission, native Player factory accepted before one-shot; fresh fullGUID/current receipt/generation/cancel revalidation, denied publication removed, bounded1s stationary nearest-qualified retry before original8/11s combat, reset/no replay/no progression grant. Client playback/full route separate.')

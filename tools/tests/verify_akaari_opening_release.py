"""Registered opening Akaari1864 qualification, acknowledged one-shot and stationary retry."""
from pathlib import Path
import re
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values, floats
from merlis_intro_pe import reachable

p = runpy.run_path(str(Path(__file__).with_name('verify_event_processor_abort_release.py')))
n, ins = p['n'], p['ins']
method = lambda name: '?' + name + '@npc_akaari_shadowgoreAI@npc_akaari_shadowgore@@'
pairs = lambda rows: {(x.mnemonic, x.op_str) for x in rows}
def refs(rows):
    return {x.address + x.size + o.mem.disp for x in rows for o in x.operands
            if o.type == CS_OP_MEM and o.mem.base == X86_REG_RIP}
text = (n['args'].release / 'worldserver.map').read_text(errors='replace')
table, = [int(x,16) for x in re.findall(r'\s+\w+:\w+\s+\?\?_7npc_akaari_shadowgore@@6BScriptObject@@@\s+([0-9a-fA-F]{16})',text)]
factory = ins('?GetAI@npc_akaari_shadowgore@@')
assert table in refs(ins('?AddSC_Devourer@@'))
assert struct.unpack_from('<12Q',n['binary'],n['offset'](table))[10] == n['symbol']('?GetAI@npc_akaari_shadowgore@@')
callbacks = {n['symbol'](method(x)) for x in ('Reset','MoveInLineOfSight','UpdateAI','EnterCombat','DamageTakenAfterScaling')}
tables = [a for a in refs(factory) if callbacks <= set(struct.unpack_from('<82Q',n['binary'],n['offset'](a)))]
assert len(tables)==1
assert {('mov','ecx, 0xa0'),('mov','qword ptr [rsi + 0x94], rbp'),('mov','word ptr [rsi + 0x9c], bp'),('mov','byte ptr [rsi + 0x38], bpl')} <= pairs(factory)
eligible, attempt, update, reset, enter = [ins(method(x)) for x in ('IntroPlayer','TryIntro','UpdateAI','Reset','EnterCombat')]
# LOS is a tiny leaf with a tail jump and no x64 unwind record.
decoder=Cs(CS_ARCH_X86,CS_MODE_64);decoder.detail=True
address=n['symbol'](method('MoveInLineOfSight'));start=n['offset'](address)
los=list(decoder.disasm(n['binary'][start:start+40],address))
los=los[:next(i for i,x in enumerate(los) if x.mnemonic=='jmp')+1]
assert {('cmp','byte ptr [rcx + 0x90], 0'),('cmp','byte ptr [rcx + 0x91], 0')} <= pairs(eligible)
step,=sites(n,eligible,'?GetCurrentStep@Scenario@@')
assert eligible[step+1].op_str=='eax, eax' and eligible[step+2].mnemonic=='je'
assert {105536,1607,1078,41924,1,3,4} <= values(eligible) and 25.0 in floats(n,eligible)
for target,count in [('?IsInWorld@Object@@',2),('?IsPreDelete@Object@@',2),('?IsQuestObjectExpired@WorldObject@@',2),('?CanTargetQuestObject@WorldObject@@',2),('?GetQuestStatus@Player@@',1),('?GetInstanceScript@WorldObject@@',1),('?CanContact@Player@@',1),('?InSamePhase@WorldObject@@',1),('?InSamePhaseId@WorldObject@@Q',1),('?GetExactDist2d@Position@@',1),('?IsWithinDistInMap@WorldObject@@',1),('?IsWithinLOSInMap@WorldObject@@',1),('?GetQuestRewardStatus@Player@@',1),('?GetScenario@ScenarioMgr@@',1),('?GetScenarioId@Scenario@@',1),('?GetCurrentStep@Scenario@@',1)]:
    assert len(sites(n,eligible,target))==count,target
assert {('cmp','dword ptr [rcx + 0xa18], 0'),('cmp','dword ptr [rbx + 0xa18], 0'),('test','byte ptr [rbx + 0x4ff0], 1'),('cmp','al, 4'),('cmp','qword ptr [rbx + 8], rax')} <= pairs(eligible)
assert {('cmp','byte ptr [rdx + 0x16], 4'),('cmovne','rdx, rax')} <= pairs(los) and len(sites(n,los,method('TryIntro')))==1
create, = sites(n,attempt,'?CreateConversation@Conversation@@')
assert 1864 in values(attempt[:create]) and ('mov','r9, r13') in pairs(attempt[:create])
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
assert {('movups','xmm0, xmmword ptr [rax]'),('movaps','xmmword ptr [rsp + 0x70], xmm0'),('cmp','r12d, dword ptr [rsi + 0x98]'),('movzx','eax, byte ptr [rax]'),('cmp','qword ptr [rax], rcx'),('xor','r12b, r12b'),('test','r12b, r12b'),('mov','byte ptr [rbx], 0'),('mov','dword ptr [rsi + 0x94], 0x3e8')} <= pairs(attempt)
remove, = sites(n,attempt,'?Remove@Conversation@@')
assert attempt[remove-3].op_str=='r12b, r12b' and attempt[remove-2].mnemonic=='je' and attempt[remove-2].operands[0].imm==latch
assert (attempt[remove-1].mnemonic,attempt[remove-1].op_str)==('mov','rcx, r15')
assert {('inc','dword ptr [rbx + 0x98]'),('mov','dword ptr [rbx + 0x94], r14d'),('mov','byte ptr [rbx + 0x9d], r14b')} <= pairs(reset)
clones,=sites(n,reset,method('ResetClones'))
assert clones<sites(n,reset,'?Reset@EventMap@@')[0]
assert not any(x.mnemonic=='mov' and x.op_str.startswith('byte ptr [') and '+ 0x38]' in x.op_str for x in reset)
assert sites(n,reset,'?Reset@EventMap@@')
# UpdateIntro is inlined before the unchanged native combat dispatcher.
grid, = sites(n,update,'?GetPlayerListInGrid@WorldObject@@')
qualified, = sites(n,update,method('IntroPlayer'))
retry, = sites(n,update,method('TryIntro'))
victim, = sites(n,update,'?UpdateVictim@CreatureAI@@')
assert grid<qualified<retry<victim and len(sites(n,update,'?GetDistance@WorldObject@@'))==2
assert 25.0 in floats(n,update) and ('mov','dword ptr [r14 + 0x94], 0x3e8') in pairs(update)
assert {11000,5000,10000,209238,105540,189170} <= values(update) and 12000 in values(enter)
assert sites(n,update,'?CastSpell@Unit@@') and sites(n,update,'?DoCast@UnitAI@@') and sites(n,update,'?ExecuteEvent@EventMap@@')
for body in (eligible,attempt,los):
    for forbidden in ('?UpdateAchievementCriteria@Player@@','?AddItem@Player@@','?CompleteQuest@Player@@'):
        assert not sites(n,body,forbidden)
print('PASS registered opening Akaari1864: Rogue41924/1078-stage0 live source/full phase/LOS/25yd scope, native Player factory acknowledgement before one-shot, fullGUID/current receipt/generation/cancel revalidation and stale publication removal; bounded1s nearest-qualified stationary retry, accepted no replay on Reset. Original capture clocks/owner/teleport remain separately checked; client playback/full route separate.')

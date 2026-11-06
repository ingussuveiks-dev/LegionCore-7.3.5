"""Registered shared AI owns Lucas's five-second cue through its destructor."""
from pathlib import Path
import runpy
import struct
from sharasdal_erunak_entry_pe import sites, values, floats
from merlis_intro_pe import reachable
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

p = runpy.run_path(str(Path(__file__).with_name('verify_kingslayers_smoke_offer_release.py')))
n, ins = p['n'], p['ins']
scope = 'npc_kingslayers_small_eventsAI@npc_kingslayers_small_events@@'
factory = ins('?GetAI@npc_kingslayers_small_events@@')
los, update, destroy = [ins(prefix + scope) for prefix in ('?MoveInLineOfSight@', '?UpdateAI@', '??_G')]
pairs = lambda rows: {(op.mnemonic, op.op_str) for op in rows}
assert ('mov', 'ecx, 0x80') in pairs(factory)
assert {('mov', 'dword ptr [rsi + 0x60], ebp'), ('mov', 'word ptr [rsi + 0x64], bp'),
        ('mov', 'qword ptr [rsi + 0x78], rbp')} <= pairs(factory)
tables = []
for op in factory:
    for arg in op.operands:
        if arg.type == CS_OP_MEM and arg.mem.base == X86_REG_RIP:
            raw = struct.unpack_from('<82Q', n['binary'], n['offset'](op.address + op.size + arg.mem.disp))
            if n['symbol']('?UpdateAI@' + scope) in raw:
                tables.append(raw)
assert len(tables) == 1 and n['resolve'](tables[0][0]) == n['symbol']('??_G' + scope)
schedule, = sites(n, los, '?ScheduleEvent@EventMap@@QEAAXII')
assert {108399, 2483, 5000} <= values(los[:schedule]) and 130.0 in floats(n, los[:schedule])
assert {('lea', 'rcx, [rdi + 0x60]'), ('mov', 'edx, 1'), ('mov', 'r8d, 0x1388')} <= pairs(los[schedule - 6:schedule])
assert not sites(n, los, '?AddDelayedEvent@FunctionProcessor@@')
execute, = sites(n, update, '?ExecuteEvent@EventMap@@')
created, = sites(n, update, '?CreateConversation@Conversation@@')
move, = sites(n, update, '?MovePoint@MotionMaster@@')
assert execute < created < move
assert {('add', 'rcx, 0x60'), ('add', 'dword ptr [rcx], edx')} <= pairs(update[:execute])
assert {('cmp', 'eax, 1'), ('jne', update[execute + 3].op_str)} <= pairs(update[execute + 1:execute + 4])
assert update[created].address not in reachable(update, update[execute + 3].operands[0].imm)
assert update[move].address not in reachable(update, update[execute + 3].operands[0].imm)
assert 2476 in values(update[execute:created])
assert {-8527.94, 442.0, 106.01} <= floats(n, update[created:move])
assert update[created + 2].mnemonic == 'jne'
assert update[move].address in reachable(update, update[created + 2].operands[0].imm)
assert {('mov', 'rax, qword ptr [rcx + 0x68]'), ('lea', 'rcx, [rdi + 0x68]'),
        ('mov', 'edx, 0x80')} <= pairs(destroy)
assert sites(n, destroy, '??$_Erase_tree@')
for prefix in ('?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@',
               '?CompleteQuest@Player@@', '?AddItem@Player@@'):
    assert not sites(n, update[:move + 1], prefix)
print('PASS registered Lucas108399 cue: AI-owned EventMap initialized/dispatched/destroyed in the selected vtable; original2483 then5s2476/MovePoint once, failed optional conversation preserves movement, no creature-owned freed-AI lambda or progression grant. Full client traversal remains separate.')

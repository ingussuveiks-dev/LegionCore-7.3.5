"""Verify installed tracked wave deaths, release barrier and advance ordering."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_magnar_finale_release.py')))
native = previous['native']
instructions = previous['instructions']
suffix = '@npc_magnar_icebreakerAI@npc_magnar_icebreaker@@'
death = '?SummonedCreatureDies' + suffix
advance = '?AdvanceWaveIfReady' + suffix
launch = '?LaunchEventWave' + suffix


def calls_to(body, dependency):
    target = native['symbol'](dependency)
    return [index for index, entry in enumerate(body) if entry.mnemonic in ('call', 'jmp')
            and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == target]


body = instructions(death)
assert body[0].op_str == 'rdx, rdx' and body[1].mnemonic == 'je' and body[1].operands[0].imm == body[-1].address
alive = next(index for index, entry in enumerate(body) if entry.op_str == 'dword ptr [rdx + 0xa18], 0')
assert body[alive + 1].op_str == 'r13, r8', 'Actual killer not retained across GUID accounting'
assert body[alive + 4].mnemonic == 'je', 'Living summon can consume a death'
erase = calls_to(body, '?_Erase@?$_Tree@V?$_Tset_traits@VObjectGuid@@')[0]
assert any(entry.op_str == 'rcx, [r15 + 0xd8]' for entry in body[erase - 5:erase]), 'Erase is not scoped to this actors active GUID set'
claimed = next(index for index, entry in enumerate(body[erase:], erase) if entry.mnemonic == 'test' and entry.op_str == 'rax, rax')
assert body[claimed + 1].mnemonic == 'je'
assert body[claimed + 2].op_str == 'eax, byte ptr [r15 + 0xd4]'
assert body[claimed + 3].op_str == 'al, al' and body[claimed + 4].mnemonic == 'je'
assert body[claimed + 1].operands[0].imm == body[claimed + 4].operands[0].imm == body[alive + 4].operands[0].imm
assert body[claimed + 5].mnemonic == 'dec' and body[claimed + 5].op_str == 'al'
assert body[claimed + 6].op_str == 'rdx, r13', 'Counted death lost killer ownership context'
assert body[claimed + 8].op_str == 'byte ptr [r15 + 0xd4], al' and calls_to(body, advance) == [claimed + 9]

policy = instructions(advance)
rejection = None
for displacement, branch in ((0xd5, 'jne'), (0xd4, 'jne'), (0xd1, 'je'), (0xd0, 'je')):
    position = next(index for index, entry in enumerate(policy) if entry.mnemonic == 'cmp' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == displacement)
    guarded = position + (3 if displacement == 0xd5 else 1)
    assert policy[guarded].mnemonic == branch
    destination = policy[guarded].operands[0].imm
    rejection = rejection or destination
    assert destination == rejection
assert any(entry.mnemonic == 'lea' and entry.op_str == 'eax, [rcx - 2]' for entry in policy)
limit = next(index for index, entry in enumerate(policy) if entry.mnemonic == 'cmp' and entry.op_str == 'al, 4')
assert policy[limit + 1].mnemonic == 'ja' and policy[limit + 1].operands[0].imm == rejection
assert policy[limit + 2].op_str == 'cl, 6' and policy[limit + 3].mnemonic == 'jae'
assert policy[limit + 4].op_str == 'eax, [rcx + 1]' and policy[limit + 7].op_str == 'byte ptr [rbx + 0xd3], al'
assert calls_to(policy, launch) == [limit + 11] and policy[limit + 11].mnemonic == 'jmp'
selection = calls_to(policy, '?GetEncounterPlayer' + suffix)[0]
assert policy[selection - 3].op_str == 'r8d, 3' and policy[selection - 2].op_str == 'rdx, rdi'
assert policy[selection + 1].op_str == 'rdi, rax' and policy[selection + 2].op_str == 'rax, rax'
assert policy[selection + 3].mnemonic == 'je' and policy[selection + 3].operands[0].imm == rejection, 'Missing recipient consumes wave completion'
for operand in ('eax, 0x17722', 'dword ptr [rax + 0xe0], 0x5d7'):
    position = next(index for index, entry in enumerate(policy) if entry.mnemonic == 'cmp' and entry.op_str == operand)
    assert policy[position + 1].mnemonic == 'jne' and policy[position + 1].operands[0].imm == rejection
claim = next(index for index, entry in enumerate(policy) if entry.mnemonic == 'inc' and entry.op_str == 'byte ptr [rbx + 0xd3]')
unprotect = calls_to(policy, '?RemoveFlag@Object@@')[0]
credit = calls_to(policy, '?UpdateAchievementCriteria@Player@@')[0]
assert selection < claim < unprotect < credit
assert policy[unprotect - 2].op_str == 'r8d, 0x2000180' and policy[unprotect - 1].op_str == 'word ptr [rbx + 0xd0], 0', 'Duel state must be claimed before reentrant flag or credit callbacks'
assert policy[credit - 2].op_str == 'r8d, 0xb55b' and policy[credit - 1].op_str == 'rcx, rdi'
assert any(entry.op_str == 'edx, 0x5c' for entry in policy[credit - 8:credit])
assert not calls_to(policy, '?FindNearestPlayer@WorldObject@@')

released = instructions(launch)
armed = next(index for index, entry in enumerate(released) if entry.op_str == 'byte ptr [r13 + 0xd5], 1')
finished = next(index for index, entry in enumerate(released) if entry.op_str == 'byte ptr [r13 + 0xd5], 0')
assert armed < finished < calls_to(released, advance)[0]
maps = calls_to(released, '?GetCreature@Map@@')
assert len(maps) == 2 and armed < maps[0] < maps[1] < finished
for position in maps:
    assert released[position + 2].op_str == 'rax, rax' and released[position + 3].mnemonic == 'je'
    assert released[position + 4].op_str == 'dword ptr [rax + 0xa18], 0' and released[position + 5].mnemonic == 'jne'
    assert released[position + 3].operands[0].imm == released[position + 5].operands[0].imm
assert any(entry.mnemonic == 'mov' and entry.op_str == 'ecx, 0x20' for entry in released[armed:maps[0]]), 'Release no longer allocates the independent list snapshot'
assert any(entry.mnemonic == 'movups' and entry.op_str == 'xmm0, xmmword ptr [rbx + 0x10]' for entry in released[armed:maps[0]]), 'Snapshot GUID payload copy missing'
insertions = {native['resolve'](address) for name, address in native['symbols'].items() if name.startswith('?_Insert_node@') and 'ObjectGuid' in name}
insert = next(index for index, entry in enumerate(released) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) in insertions)
assert maps[0] < insert < calls_to(released, '?RemoveFlag@Object@@')[0], 'Actor made attackable before active GUID registration'
values = {operand.imm for entry in released for operand in entry.operands if operand.type == CS_OP_IMM}
assert {191032, 191821, 64367, 96468} <= values and 46427 not in values
assert not calls_to(released, '?UpdateAchievementCriteria@Player@@'), 'Release switch still bypasses guarded stage3 completion'
assert any(entry.mnemonic in ('movss', 'mulss', 'divss', 'comiss') and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP
           and struct.unpack_from('<f', native['binary'], native['offset'](entry.address + entry.size + operand.mem.disp))[0] == 80.0
           for entry in released for operand in entry.operands), 'Original final80pct request changed'
switch = next(index for index, entry in enumerate(released) if entry.mnemonic == 'sub' and entry.op_str == 'edx, 1')
for offset, count in ((0, 2), (2, 2), (4, 1), (6, 2)):
    assert released[switch + offset].mnemonic == 'sub' and released[switch + offset].op_str == 'edx, 1'
    branch = released[switch + offset + 1]
    assert branch.mnemonic == 'je'
    target = next(entry for entry in released if entry.address == branch.operands[0].imm)
    assert target.mnemonic == 'mov' and target.op_str == f'r12b, {count}'
assert released[switch + 8].op_str == 'edx, 1' and released[switch + 8].mnemonic == 'cmp'
assert released[switch + 9].mnemonic == 'jne' and released[switch + 10].op_str == 'r12b, 3'
assert released[switch + 11].op_str == 'byte ptr [rbp + 0x6f], dl', 'Fifth wave no longer enables Windwall/health work'

combat = instructions('?EnterCombat' + suffix)
schedules = calls_to(combat, '?ScheduleEvent@EventMap@@QEAAXIIGG@Z')
assert len(schedules) == 4
for position, identity, delay in zip(schedules, (1, 2, 3, 4), (7000, 14000, 15000, 33000)):
    assert any(entry.op_str == f'edx, {identity}' for entry in combat[position - 7:position])
    assert any(entry.op_str == f'r8d, {hex(delay)}' for entry in combat[position - 7:position])
phase_gate = next(index for index, entry in enumerate(combat) if entry.op_str == 'byte ptr [rbx + 0xd1], sil')
assert schedules[2] < phase_gate < schedules[3] and combat[phase_gate + 1].mnemonic == 'je'
assert combat[phase_gate + 1].operands[0].imm == combat[schedules[3] + 1].address, 'Final-duel re-entry still arms the first-phase33000ms protection/jump'

updated = instructions('?UpdateAI' + suffix)
victim = calls_to(updated, '?UpdateVictim@CreatureAI@@')[0]
retry = calls_to(updated, advance)[0]
pending = next(index for index, entry in enumerate(updated) if entry.op_str == 'byte ptr [rbx + 0xd3], 6')
assert pending < retry < victim and updated[pending + 1].mnemonic == 'jne'
assert updated[pending + 1].operands[0].imm == updated[retry + 1].address and updated[retry - 2].op_str == 'edx, edx', 'Tick retry must be pending-wave-only with no retained raw killer pointer'
assert updated[victim + 1].op_str == 'al, al' and updated[victim + 2].mnemonic == 'je'
assert updated[victim + 3].op_str == 'byte ptr [rbx + 0xd0], 0' and updated[victim + 4].mnemonic == 'jne'
assert updated[victim + 2].operands[0].imm == updated[victim + 4].operands[0].imm
speech = calls_to(updated, '?CreateConversation@Unit@@')
assert len(speech) == 1 and updated[speech[0] - 1].op_str == 'edx, 0x1f2'
assert victim < calls_to(updated, '?ExecuteEvent@EventMap@@')[0] < speech[0], '498 no longer follows the resumed combat event queue'
start = calls_to(updated, launch)[0]
assert updated[start - 2].op_str == 'eax, [rdx + 1]' and updated[start - 1].op_str == 'byte ptr [rbx + 0xd3], al', 'Initial wave ID still increments after reentrant release'
reset = instructions('?Reset' + suffix)
assert all(any(entry.op_str == f'byte ptr [rbx + {hex(displacement)}], 0' for entry in reset) for displacement in (0xd5, 0xd4))
assert native['symbol']('?clear@?$_Tree@V?$_Tset_traits@VObjectGuid@@') in native['calls']('?Reset' + suffix)
cleanup = calls_to(reset, '?DespawnAll@SummonList@@')[0]
phase = next(index for index, entry in enumerate(reset) if entry.op_str == 'byte ptr [rbx + 0xd1], 0' and reset[index + 1].op_str == 'byte ptr [rbx + 0xd4], 0')
assert reset[phase + 2].mnemonic == 'je' and reset[phase + 2].operands[0].imm == reset[cleanup + 1].address, 'Final phase is rewound or cleaned as an incomplete wave'
assert reset[cleanup - 1].op_str == 'rcx, [rbx + 0x38]', 'Cleanup does not use Magnars actual owned SummonList'
reserve = calls_to(reset, '?clear@?$list@VObjectGuid@@')[0]
wave_one = next(index for index, entry in enumerate(reset) if entry.op_str == 'byte ptr [rbx + 0xd3], 1')
assert phase < wave_one < reserve < cleanup
for displacement in (0xe8, 0xd5):
    claimed = next(index for index, entry in enumerate(reset) if entry.op_str == f'byte ptr [rbx + {hex(displacement)}], 1')
    released_claim = next(index for index, entry in enumerate(reset) if entry.op_str == f'byte ptr [rbx + {hex(displacement)}], 0')
    assert claimed < phase < cleanup < released_claim, 'Reset permits reentrant start/advance while despawning the old roster'
assert reset[phase + 1].op_str == 'byte ptr [rbx + 0xd4], 0'
despawn = instructions('?DespawnAll@SummonList@@')
lookup = calls_to(despawn, '?GetCreature@Unit@@SAPEAVCreature@@AEAVWorldObject@@VObjectGuid@@')[0]
unspawn = calls_to(despawn, '?DespawnOrUnsummon@Creature@@')[0]
assert despawn[lookup - 1].op_str == 'rcx, qword ptr [rdi + 0x60]', 'Native list lost its owner lookup context'
assert despawn[lookup + 1].op_str == 'rsi, rax' and despawn[lookup + 2].op_str == 'rax, rax'
assert despawn[lookup + 3].mnemonic == 'je'
assert despawn[lookup + 4].op_str == 'dword ptr [rax + 0x30e8], 0x200' and despawn[lookup + 5].mnemonic == 'jne'
assert despawn[lookup + 3].operands[0].imm == despawn[lookup + 5].operands[0].imm, 'Native accessory/missing actor cleanup policy changed'
assert any(entry.mnemonic == 'dec' and entry.op_str == 'qword ptr [rdi + 8]' for entry in despawn[lookup:unspawn]), 'Owned GUID is not erased before its actor callback'
assert despawn[unspawn - 2].op_str == 'edx, edx' and despawn[unspawn - 1].op_str == 'rcx, rsi'
tables = []
for entry in instructions('?GetAI@npc_magnar_icebreaker@@'):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            offset = native['offset'](entry.address + entry.size + operand.mem.disp)
            if struct.unpack_from('<Q', native['binary'], offset + 0x158)[0] == native['symbol'](death): tables.append(offset)
assert len(tables) == 1
start_action = '?DoAction' + suffix
assert struct.unpack_from('<Q', native['binary'], tables[0] + 0x48)[0] == native['symbol'](start_action), 'Magnar factory does not install guarded start'
assert struct.unpack_from('<Q', native['binary'], tables[0] + 0x128)[0] == native['symbol']('?EnterCombat' + suffix)
started = instructions(start_action)
assert started[0].op_str == 'edx, 1' and started[1].mnemonic == 'jne' and started[1].operands[0].imm == started[-1].address
duplicate = next(index for index, entry in enumerate(started) if entry.op_str == 'byte ptr [rcx + 0xe8], 0')
assert started[duplicate + 2].mnemonic == 'jne'
rejection = started[duplicate + 2].operands[0].imm
world = calls_to(started, '?IsInWorld@Object@@')[0]
assert started[world + 1].op_str == 'al, al' and started[world + 2].mnemonic == 'je' and started[world + 2].operands[0].imm == rejection
for operand in ('dword ptr [rcx + 0xa18], 0', 'byte ptr [rcx + 0x39c5], 0', 'eax, 0x17722', 'dword ptr [rax + 0xe0], 0x5d7'):
    position = next(index for index, entry in enumerate(started) if entry.mnemonic == 'cmp' and entry.op_str == operand)
    assert started[position + 1].mnemonic == 'jne' and started[position + 1].operands[0].imm == rejection
selection = calls_to(started, '?GetEncounterPlayer' + suffix)[0]
assert started[selection - 3].op_str == 'edx, edx' and started[selection - 2].op_str == 'r8d, 3'
assert started[selection + 1].op_str == 'rax, rax' and started[selection + 2].mnemonic == 'je' and started[selection + 2].operands[0].imm == rejection
companion = calls_to(started, '?FindNearestCreature@WorldObject@@')[0]
assert started[companion - 2].op_str == 'edx, 0x178d4'
assert started[companion - 4].op_str == 'r9b, 1', 'Companion lookup no longer requests a living actor'
assert started[companion + 1].op_str == 'r14, rax' and started[companion + 2].op_str == 'rax, rax'
assert started[companion + 3].mnemonic == 'je'
missing = started[companion + 3].operands[0].imm
for offset, operand, branch in ((4, 'dword ptr [rax + 0xa18], 0', 'jne'), (6, 'byte ptr [rax + 0x858], 0', 'je'), (8, 'qword ptr [rax + 0x9c0], 0', 'je')):
    assert started[companion + offset].op_str == operand and started[companion + offset + 1].mnemonic == branch
    assert started[companion + offset + 1].operands[0].imm == missing
claim = next(index for index, entry in enumerate(started) if entry.mnemonic == 'mov' and entry.op_str == 'byte ptr [rbx + 0xe8], 1')
assert companion + 9 < claim < calls_to(started, '?clear@?$list@VObjectGuid@@')[0]
summons = calls_to(started, '?SummonCreature@WorldObject@@QEBAPEAVTempSummon@@IAEBUPosition@@W4TempSummonType@@IHVObjectGuid@@')
assert len(summons) == 6 and all(claim < position for position in summons)
assert all(claim < calls_to(started, dependency)[0] for dependency in ('?RemoveFlag@Object@@', '?SetReactState@Creature@@', '?SetUInt32Value@Object@@', '?MoveJump@MotionMaster@@QEAAXMMMMMMI_NPEBUJumpArrivalCastArgs@@'))
values = {operand.imm for entry in started for operand in entry.operands if operand.type == CS_OP_IMM}
assert {97123, 94707, 94813} <= values
assert len([entry for entry in started if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x10]) == 1
combat = next(index for index, entry in enumerate(started) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x128)
assert started[combat - 2].op_str == 'rdx, r14' and combat > summons[-1], 'Combat no longer uses prevalidated original companion'
ranges = [struct.unpack_from('<f', native['binary'], native['offset'](entry.address + entry.size + operand.mem.disp))[0]
          for entry in started if entry.mnemonic == 'movss' for operand in entry.operands if operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP]
assert any(abs(value - 70.0) < 0.001 for value in ranges), 'Native companion admission range changed'
assert any(entry.op_str == 'byte ptr [rbx + 0xe8], 0' for entry in reset), 'Reset does not clear start claim'
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x158 for entry in instructions('?Kill@Unit@@'))
print('PASS: canonical tracked death callback forwards actual killer after once-only GUID consumption; bounded wave6 completion uses eligible owner-first stage3 selection before duel claim/flag release/46427, missing recipient stays pending, and native UpdateAI retries before victim/cast gates. Independent snapshot/release barrier,2/2/1/2/3, Windwall/80pct and resumed498 retained. Full actor recovery/group criteria/client VFX not certified.')
print('PASS: actual Magnar start slot0x48 admits only action1/live96034/map1495/eligible stage3 and live enabled companion96468 within70yd. The start byte is claimed before list/spawn/flag/jump/combat side effects and blocks duplicate requests without delaying timers; Reset clears the scoped claim. Missing actor automatic recovery/full reset/client visuals not certified.')
print('PASS: installed first-phase Reset holds start/release claims, empties active/count/reserve and restores wave1 before native owned SummonList::DespawnAll; final phase bypasses that rewind. Actual native cleanup resolves owned GUIDs, skips accessories/missing actors and erases before immediate despawn. Not a gossip/pose/health/final-despawn or client recovery certificate.')

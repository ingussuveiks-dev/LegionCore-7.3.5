"""Verify actual post-scaled final Vigfus thresholds, claims and original requests."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_spectre_rush_release.py')))
native = previous['native']
instructions = previous['instructions']
post = '?DamageTakenAfterScaling@npc_vigfus_bladewind_lastAI@npc_vigfus_bladewind_last@@'
body = instructions(post)
values = {operand.imm for entry in body for operand in entry.operands if operand.type == CS_OP_IMM}
assert {98602, 1511, 944, 5, 15, 7} <= values, 'Final threshold scope or native percentages changed'
for dependency in ('?GetHealth@Unit@@QEBA_KPEAV1@@', '?CountPctFromMaxHealth@Unit@@', '?GetScenario@ScenarioMgr@@',
                   '?GetScenarioId@Scenario@@', '?GetCurrentStep@Scenario@@', '?MovePoint@MotionMaster@@'):
    assert native['symbol'](dependency) in native['calls'](post), f'Missing actual scaled-threshold dependency: {dependency}'
for forbidden in ('?SetHealth@Unit@@', '?UpdateAchievementCriteria@Player@@', '?AddItem@Player@@'):
    matches = {native['resolve'](address) for name, address in native['symbols'].items() if name.startswith(forbidden)}
    assert not matches & native['calls'](post), f'Threshold adds a health/progression bypass: {forbidden}'
for mnemonic, operands in (('xor', 'edi, edi'), ('sub', 'rcx, rdx'), ('cmp', 'rdx, rax'), ('cmovb', 'rdi, rcx')):
    assert any(entry.mnemonic == mnemonic and entry.op_str == operands for entry in body), 'Projected health does not saturate lethal damage at zero'
thresholds = [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
              and native['resolve'](entry.operands[0].imm) == native['symbol']('?CountPctFromMaxHealth@Unit@@')]
talks = [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
         and native['resolve'](entry.operands[0].imm) == native['symbol']('?Talk@CreatureAI@@QEAAXE')]
assert len(thresholds) == len(talks) == 2, 'Both native final low-health requests must remain'
for threshold, talk, displacement in zip(thresholds, talks, (0xc4, 0xc5)):
    assert any(entry.mnemonic == 'mov' and entry.op_str == 'r8, rsi' for entry in body[threshold - 4:threshold]), 'Percentage uses raw/template rather than attacker-relative maximum'
    assert body[threshold + 1].op_str == 'rdi, rax' and body[threshold + 2].mnemonic == 'jae', 'Strict below15/7 boundary changed'
    claim = next(index for index, entry in enumerate(body) if entry.mnemonic == 'mov' and entry.op_str == f'byte ptr [rbx + {displacement:#x}], 1')
    assert threshold < claim < talk, 'Reentrant speech runs before once-only phase claim'
    assert body[talk + 1].mnemonic == 'cmp' and body[talk + 1].op_str == 'ebp, dword ptr [rbx + 0xc8]' and body[talk + 2].mnemonic == 'jne', 'Reset during speech continues obsolete phase requests'
tables = []
for entry in instructions('?GetAI@npc_vigfus_bladewind_last@@'):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            offset = native['offset'](entry.address + entry.size + operand.mem.disp)
            slots = struct.unpack_from('<128Q', native['binary'], offset)
            if slots[0xa8 // 8] == native['symbol'](post):
                tables.append(slots)
assert len(tables) == 1, 'Final boss factory does not select the post-scaled callback'
assert tables[0][0xa0 // 8] != native['symbol'](post), 'Final threshold still runs in the pre-scaled damage slot'
assert native['symbol']('?SendChat@CreatureTextMgr@@') in native['calls']('?Talk@CreatureAI@@QEAAXE'), 'Native Talk no longer reaches creature text'
assert native['symbol']('?SendText@CreatureTextMgr@@') in native['calls']('?SendChat@CreatureTextMgr@@'), 'Native chat no longer delegates selected text'
for dependency in ('?Write@PlaySound@Misc@WorldPackets@@', '?SendNonChatPacket@CreatureTextMgr@@'):
    assert native['symbol'](dependency) in native['calls']('?SendText@CreatureTextMgr@@'), 'Native selected text lost its inlined sound-packet request'
print('PASS: canonical final98602 post-scaled vtable/1511-944-5 scope, projected attacker-relative strict15/7 thresholds, claims-before-Talk/reset guards and original native speech/move requests; no health or quest grant. Native text/sound request path is not audible or lethal-survival/client proof.')

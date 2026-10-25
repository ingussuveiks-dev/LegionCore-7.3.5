"""Verify actual Holy pickup opening, report-use and completed-claim admission."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_presentation_release.py')))
native = previous['native']
instructions = previous['instructions']
helper = '?CanTake@SilverHandAcquisition@@'
gossip = '?OnGossipHello@go_art_silverhand@@'
ai_gossip = '?GossipHello@AI@go_art_silverhand@@'
factory = '?GetAI@go_art_silverhand@@'
loot = '?OnLootStateChanged@go_art_silverhand@@'
body = instructions(helper)


def calls(prefix):
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == native['symbol'](prefix)]


deny = next(entry.address for index, entry in enumerate(body[:-1]) if entry.op_str == 'rbx, qword ptr [rsp + 0x40]'
            and body[index + 1].mnemonic == 'xor' and body[index + 1].op_str == 'al, al')


def reject(position, branch):
    assert body[position + 1].mnemonic == branch and body[position + 1].operands[0].imm == deny, 'Admission does not reach no-effect exit'


for operands, branch in (('rcx, rcx', 'je'), ('rdx, rdx', 'je'), ('dword ptr [rbx + 0xa18], 0', 'jne'),
                         ('byte ptr [rbx + 0x6848], 0', 'jne'), ('byte ptr [rbx + 0x6849], 0', 'jne'),
                         ('dword ptr [rdi + 0xe0], 0x603', 'jne'), ('rax, rax', 'je'),
                         ('rax, qword ptr [rdi + 8]', 'jne'), ('dword ptr [rbx + 0x3b8], eax', 'jne')):
    position = next(index for index, entry in enumerate(body) if entry.mnemonic in ('cmp', 'test') and entry.op_str == operands)
    reject(position, branch)
for prefix, branch in (('?IsInWorld@Object@@', 'je'), ('?CanContact@Player@@', 'je'), ('?isSpawned@GameObject@@', 'je'),
                       ('?InSamePhase@WorldObject@@', 'je'), ('?canSeeOrDetect@WorldObject@@', 'je'),
                       ('?IsWithinLOSInMap@WorldObject@@', 'je'), ('?IsWithinDistInMap@WorldObject@@', 'je'),
                       ('?HasFlag@Object@@', 'jne'), ('?GetQuestRewardStatus@Player@@', 'jne')):
    positions = calls(prefix)
    assert len(positions) == (2 if prefix == '?IsInWorld@Object@@' else 1), f'Wrong native admission consumers: {prefix}'
    for position in positions:
        assert body[position + 1].op_str == 'al, al'
        reject(position + 1, branch)
for prefix, expected in (('?GetByteValue@Object@@', 'al, 2'), ('?GetUInt32Value@Object@@', 'eax, 0x3cfe0'),
                         ('?GetScenarioId@Scenario@@', 'eax, 0x444')):
    position, = calls(prefix)
    assert body[position + 1].op_str == expected
    reject(position + 1, 'jne')
distance, = calls('?GetInteractionDistance@GameObject@@')
assert body[distance + 1].op_str == 'xmm2, xmm0' and distance < calls('?IsWithinDistInMap@WorldObject@@')[0]
flags, = calls('?HasFlag@Object@@')
assert all(any(entry.op_str == value for entry in body[flags - 3:flags]) for value in ('edx, 0x11', 'r8d, 0x10'))
statuses = calls('?GetQuestStatus@Player@@')
assert len(statuses) == 2 and all(any(entry.op_str == 'edx, 0xa488' for entry in body[index - 2:index]) for index in statuses)
assert body[statuses[0] + 1].op_str == 'eax, 3' and body[statuses[0] + 2].mnemonic == 'je'
assert body[statuses[0] + 2].operands[0].imm == body[statuses[1] + 3].address
assert body[statuses[1] + 1].op_str == 'eax, 1'
reject(statuses[1] + 1, 'jne')
scenario, = calls('?GetScenario@ScenarioMgr@@')
assert any(entry.op_str == 'edx, dword ptr [rdi + 0x3b8]' for entry in body[scenario - 3:scenario])
assert body[scenario + 2].op_str == 'rax, rax'
reject(scenario + 2, 'je')
steps = calls('?GetCurrentStep@Scenario@@')
assert len(steps) == 2 and body[steps[0] + 1].op_str == 'eax, 0xa' and body[steps[0] + 2].mnemonic == 'je'
assert body[steps[1] + 1].op_str == 'eax, 0xb'
reject(steps[1] + 1, 'jne')
owned, = calls('?GetItemByEntry@Player@@')
assert owned > steps[1] and all(any(entry.op_str == value for entry in body[owned - 4:owned]) for value in ('edx, 0x1f737', 'r8d, r8d'))
assert body[owned + 1].op_str == 'rax, rax'
reject(owned + 1, 'je')
assert body[steps[0] + 2].operands[0].imm == body[owned + 3].address, 'Fresh final step must not require an already owned weapon'
for prefix in (gossip, ai_gossip):
    callback = instructions(prefix)
    gate = next(index for index, entry in enumerate(callback) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
                and native['resolve'](entry.operands[0].imm) == native['symbol'](helper))
    assert callback[gate + 1].op_str == 'al, al' and callback[gate + 2].mnemonic == 'je'
    rejected = callback[gate + 2].operands[0].imm
    assert any(entry.address == rejected and entry.op_str == 'al, 1' and entry.mnemonic == 'mov' for entry in callback)
    state = next(index for index, entry in enumerate(callback) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
                 and native['resolve'](entry.operands[0].imm) == native['symbol']('?getLootState@GameObject@@'))
    assert callback[state + 1].op_str == 'eax, 3' and callback[state + 2].mnemonic == 'je' and callback[state + 2].operands[0].imm == rejected
callback = instructions(loot)
gate = next(index for index, entry in enumerate(callback) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == native['symbol'](helper))
assert callback[gate + 1].op_str == 'al, al' and callback[gate + 2].mnemonic == 'je'
rejected = callback[gate + 2].operands[0].imm
for operands in ('r8d, 3', 'eax, 3'):
    position = next(index for index, entry in enumerate(callback[:gate]) if entry.mnemonic == 'cmp' and entry.op_str == operands)
    assert callback[position + 1].mnemonic == 'jne' and callback[position + 1].operands[0].imm == rejected
effects = [index for index, entry in enumerate(callback) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
           and native['resolve'](entry.operands[0].imm) == native['symbol']('?UpdateAchievementCriteria@Player@@')]
assert not effects, 'Completed-loot callback still directly awards scenario progression'
for forbidden in ('?AddItem@Player@@', '?KilledMonsterCredit@Player@@'):
    assert native['symbol'](forbidden) not in native['calls'](loot), f'Completed claim bypasses native loot/presentation: {forbidden}'
tables = []
for entry in instructions('?AddSC_SilverHand@@'):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            address = entry.address + entry.size + operand.mem.disp
            if all(struct.unpack_from('<Q', native['binary'], native['offset'](address) + slot)[0] == native['symbol'](prefix)
                   for slot, prefix in ((0x10, gossip), (0x50, loot), (0x60, factory))):
                tables.append(address)
assert len(tables) == 1, 'Registration does not select guarded callbacks/factory'
ai_tables = []
assert any(entry.op_str == 'qword ptr [rax + 8], rbx' for entry in instructions(factory))
for entry in instructions(factory):
    if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x48)[0] == native['symbol'](ai_gossip):
            ai_tables.append(address)
assert len(ai_tables) == 1, 'Native AI factory does not install guarded opening/report callback'
assert native['symbol']('?GetGameObjectAI@ScriptMgr@@') in native['calls']('?SelectGameObjectAI@FactorySelector@@')
assert any(entry.mnemonic == 'jmp' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x60
           for entry in instructions('?GetGameObjectAI@ScriptMgr@@'))
for prefix in ('?HandleGameobjectReportUse@WorldSession@@', '?SendLoot@Spell@@'):
    dispatch = instructions(prefix)
    slot = next(index for index, entry in enumerate(dispatch) if entry.mnemonic == 'mov' and entry.op_str == 'r9, qword ptr [rcx + 0x48]')
    invocation = next(index for index in range(slot + 1, slot + 6) if dispatch[index].mnemonic == 'call')
    assert dispatch[invocation].op_str == 'r9' and any(entry.op_str == 'rcx, rax' for entry in dispatch[slot + 1:invocation])
    assert not any(entry.op_str.startswith('r9,') for entry in dispatch[slot + 1:invocation])
    if prefix == '?SendLoot@Spell@@':
        assert dispatch[invocation + 1].op_str == 'al, al' and dispatch[invocation + 2].mnemonic == 'jne', 'Native opening ignores rejection'
    else:
        assert dispatch[invocation + 1].op_str != 'al, al', 'ReportUse return handling changed; review its generic use criteria separately'
assert native['symbol']('?OnGossipHello@ScriptMgr@@QEAA_NPEAVPlayer@@PEAVGameObject@@') in native['calls']('?Use@GameObject@@QEAAXPEAVUnit@@')
assert native['symbol']('?Use@GameObject@@QEAAXPEAVUnit@@') in native['calls']('?HandleGameObjectUse@WorldSession@@')
print('PASS: installed Silver Hand script/AI factories guard native Use/opening and actual completed-state3 with live Paladin1539/1092/active42120, phase/visibility/LOS/range; fresh10 or carried128823-only replay11. ReportUse selects the AI but discards its bool. No direct AddItem/scenario/kill credit in the claim; real storage/presentation/return are separately checked.')

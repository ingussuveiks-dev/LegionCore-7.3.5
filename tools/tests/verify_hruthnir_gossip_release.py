"""Verify installed Hruthnir gossip admission and actual raw-option dispatch."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_magnar_waves_release.py')))
native = previous['native']


def instructions(name):
    body = previous['instructions'](name)
    if name.startswith('?MovementInform@npc_hrutnirAI@npc_hrutnir@@'):
        body = body[:max(index for index, entry in enumerate(body) if entry.mnemonic == 'ret') + 1]
    return body


helper = '?CanStart@HruthnirGossip@@'
hello = '?OnGossipHello@npc_hrutnir@@'
select = '?OnGossipSelect@npc_hrutnir@@'
ai_select = '?sGossipSelect@npc_hrutnirAI@npc_hrutnir@@'


def positions(body, dependency):
    target = native['symbol'](dependency)
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call'
            and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == target]


body = instructions(helper)
deny = next(entry.address for index, entry in enumerate(body[:-1])
            if entry.op_str == 'rbx, qword ptr [rsp + 0x40]' and body[index + 1].op_str == 'al, al')
for operands, branch in (('rcx, rcx', 'je'), ('rdx, rdx', 'je'),
                         ('dword ptr [rbx + 0xa18], 0', 'jne'), ('byte ptr [rbx + 0x39c5], 0', 'jne'),
                         ('eax, 0x178d4', 'jne'), ('dword ptr [rbx + 0xe0], 0x5d7', 'jne'),
                         ('al, 1', 'jne'), ('rax, qword ptr [rbx + 8]', 'jne'),
                         ('dword ptr [rdi + 0x3b8], eax', 'jne'), ('rax, rbx', 'jne'),
                         ('eax, 1', 'jne'), ('eax, 0x38d', 'jne')):
    check = next(index for index, entry in enumerate(body) if entry.mnemonic in ('test', 'cmp') and entry.op_str == operands)
    assert body[check + 1].mnemonic == branch and body[check + 1].operands[0].imm == deny, f'Missing admission gate: {operands}'
for dependency, branch, expected_count in (('?IsInWorld@Object@@', 'je', 2), ('?CanContact@Player@@', 'je', 1),
                                            ('?InSamePhase@WorldObject@@', 'je', 1), ('?canSeeOrDetect@WorldObject@@', 'je', 1),
                                            ('?IsWithinLOSInMap@WorldObject@@', 'je', 1), ('?GetQuestRewardStatus@Player@@', 'jne', 1)):
    checks = positions(body, dependency)
    assert len(checks) == expected_count
    for check in checks:
        assert body[check + 1].op_str == 'al, al'
        assert body[check + 2].mnemonic == branch and body[check + 2].operands[0].imm == deny
interaction = positions(body, '?GetNPCIfCanInteractWith@Player@@')[0]
assert any(entry.op_str == 'r8d, 1' for entry in body[interaction - 6:interaction]), 'Native interaction lost primary gossip flag'
assert body[interaction + 1].op_str == 'rax, rbx', 'Interaction must resolve this exact companion'
statuses = positions(body, '?GetQuestStatus@Player@@')
assert len(statuses) == 2 and body[statuses[0] + 1].op_str == 'eax, 3'
assert body[statuses[0] + 2].mnemonic == 'je' and body[statuses[0] + 2].operands[0].imm == body[statuses[1] + 3].address
assert sum(entry.op_str == 'edx, 0x9917' for entry in body) == 3
scenario = positions(body, '?GetScenario@ScenarioMgr@@')[0]
assert any(entry.op_str == 'edx, dword ptr [rbx + 0x3b8]' for entry in body[scenario - 3:scenario])
assert body[scenario + 2].op_str == 'rax, rax' and body[scenario + 3].mnemonic == 'je' and body[scenario + 3].operands[0].imm == deny
step = positions(body, '?GetCurrentStep@Scenario@@')[0]
assert body[step + 1].op_str == 'eax, 3' and body[step + 2].mnemonic == 'jne' and body[step + 2].operands[0].imm == deny

body = instructions(ai_select)
movement = positions(body, '?MovePoint@MotionMaster@@QEAAXIMMM_NM@Z')[0]
gate = positions(body, helper)[0]
assert body[0].op_str == 'r8d, 0x4937' and body[1].mnemonic == 'jne' and body[1].operands[0].imm > body[movement].address
index = next(index for index, entry in enumerate(body) if entry.op_str == 'r9d, r9d')
assert body[index + 1].mnemonic == 'jne' and body[index + 1].operands[0].imm > body[movement].address
assert body[gate + 1].op_str == 'al, al' and body[gate + 2].mnemonic == 'je' and body[gate + 2].operands[0].imm > body[movement].address
item = positions(body, '?GetItem@GossipMenu@@')[0]
assert body[item - 3].op_str == 'edx, edx' and body[item - 3].mnemonic == 'xor'
for operands, branch in (('eax, 0x4937', 'jne'), ('rdi, rdi', 'je'), ('byte ptr [rdi], 0', 'jne'),
                         ('dword ptr [rdi + 4], 0', 'jne'), ('dword ptr [rdi + 8], 0', 'jne'),
                         ('byte ptr [rdi + 0x50], 0', 'jne'), ('dword ptr [rdi + 0xc], 0', 'jne')):
    check = next(index for index, entry in enumerate(body) if entry.mnemonic in ('test', 'cmp') and entry.op_str == operands)
    assert body[check + 1].mnemonic == branch and body[check + 1].operands[0].imm > body[movement].address, f'Missing cached-menu guard: {operands}'
claim = positions(body, '?RemoveFlag@Object@@')[0]
clear = positions(body, '?ClearMenus@PlayerMenu@@')[0]
close = positions(body, '?SendCloseGossip@PlayerMenu@@')[0]
assert gate < item < claim < clear < close < movement
assert any(entry.op_str == 'edx, 0x7d' for entry in body[claim - 4:claim])
assert any(entry.op_str == 'r8d, 1' for entry in body[claim - 4:claim]), 'Claim must remove only primary gossip bit'
assert any(entry.op_str == 'edx, 7' for entry in body[movement - 9:movement])
coordinates = [struct.unpack_from('<f', native['binary'], native['offset'](entry.address + entry.size + entry.operands[1].mem.disp))[0]
               for entry in body if entry.mnemonic == 'movss' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP]
assert all(any(abs(actual - expected) < 0.001 for actual in coordinates) for expected in (4720.80, 347.91, -37.71))
assert not positions(body, '?UpdateAchievementCriteria@Player@@') and not positions(body, '?AddItem@Player@@')

body = instructions(hello)
gate = positions(body, helper)[0]
clear = positions(body, '?ClearMenus@PlayerMenu@@')[0]
close = positions(body, '?SendCloseGossip@PlayerMenu@@')[0]
assert body[gate + 1].op_str == 'al, al' and body[gate + 2].mnemonic == 'je'
assert body[gate + 2].operands[0].imm == body[clear - 1].address
assert any(entry.mnemonic == 'xor' and entry.op_str == 'al, al' for entry in body[gate + 3:clear]) and clear < close
address = native['symbol'](select)
assert native['binary'][native['offset'](address):native['offset'](address) + 3] == b'\xb0\x01\xc3', 'Legacy sender/action endpoint must consume without mutation'


def selected_tables(prefix, slots):
    tables = []
    for entry in instructions(prefix):
        if entry.mnemonic != 'lea':
            continue
        for operand in entry.operands:
            if operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
                address = entry.address + entry.size + operand.mem.disp
                offset = native['offset'](address)
                if all(struct.unpack_from('<Q', native['binary'], offset + slot)[0] == native['symbol'](dependency) for slot, dependency in slots):
                    tables.append(address)
    return tables


assert len(selected_tables('?AddSC_EarthWarder@@', ((0x10, hello), (0x18, select), (0x50, '?GetAI@npc_hrutnir@@')))) == 1
assert len(selected_tables('?GetAI@npc_hrutnir@@', ((0xc8, ai_select),))) == 1
dispatch = instructions('?HandleGossipSelectOption@WorldSession@@')
raw = next(index for index, entry in enumerate(dispatch) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0xc8)
legacy = positions(dispatch, '?OnGossipSelect@ScriptMgr@@QEAA_NPEAVPlayer@@PEAVCreature@@')[0]
assert raw < legacy, 'Installed native handler must call raw AI selection before legacy sender/action callback'
print('PASS: canonical Hruthnir script/AI slots, raw18743/0 and cached option checks, live39191/1495/909-3 interaction/phase/visibility gates, gossip-only claim before clear/close/original point7. No new credit/item grants; client motion/dialogue/retry still pending.')

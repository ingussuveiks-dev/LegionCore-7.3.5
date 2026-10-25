"""Verify registered Truthguard opening, AI and activation admission in Release."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_truthguard_surrender_release.py')))
native = previous['native']
instructions = previous['instructions']
helper = '?CanTake@TruthguardAcquisition@@'
gossip = '?OnGossipHello@go_sr_truthguard@@'
ai_gossip = '?GossipHello@AI@go_sr_truthguard@@'
factory = '?GetAI@go_sr_truthguard@@'
loot = '?OnLootStateChanged@go_sr_truthguard@@'
body = instructions(helper)


def calls(prefix):
    return [index for index, item in enumerate(body) if item.mnemonic == 'call' and item.operands[0].type == CS_OP_IMM
            and native['resolve'](item.operands[0].imm) == native['symbol'](prefix)]


deny = next(item.address for index, item in enumerate(body[:-1]) if item.op_str == 'rbx, qword ptr [rsp + 0x40]'
            and body[index + 1].op_str == 'al, al' and body[index + 1].mnemonic == 'xor')


def reject(position, branch):
    assert body[position + 1].mnemonic == branch and body[position + 1].operands[0].imm == deny


for operands, branch in (('rcx, rcx', 'je'), ('rdx, rdx', 'je'), ('dword ptr [rbx + 0xa18], 0', 'jne'),
                         ('byte ptr [rbx + 0x6848], 0', 'jne'), ('byte ptr [rbx + 0x6849], 0', 'jne'),
                         ('dword ptr [rdi + 0xe0], 0x5d7', 'jne'), ('rax, rax', 'je'),
                         ('rax, qword ptr [rdi + 8]', 'jne'), ('dword ptr [rbx + 0x3b8], eax', 'jne')):
    check = next(index for index, item in enumerate(body) if item.mnemonic in ('cmp', 'test') and item.op_str == operands)
    reject(check, branch)
for prefix, branch in (('?IsInWorld@Object@@', 'je'), ('?CanContact@Player@@', 'je'), ('?isSpawned@GameObject@@', 'je'),
                       ('?InSamePhase@WorldObject@@', 'je'), ('?canSeeOrDetect@WorldObject@@', 'je'),
                       ('?IsWithinLOSInMap@WorldObject@@', 'je'), ('?IsWithinDistInMap@WorldObject@@', 'je'),
                       ('?HasFlag@Object@@', 'jne'), ('?GetQuestRewardStatus@Player@@', 'jne')):
    positions = calls(prefix)
    assert len(positions) == (2 if prefix == '?IsInWorld@Object@@' else 1), f'Missing or ambiguous admission call: {prefix}'
    for position in positions:
        assert body[position + 1].op_str == 'al, al'
        reject(position + 1, branch)
klass = calls('?GetByteValue@Object@@')[0]
assert body[klass + 1].op_str == 'al, 2'
reject(klass + 1, 'jne')
distance = calls('?GetInteractionDistance@GameObject@@')[0]
assert body[distance + 1].op_str == 'xmm2, xmm0' and distance < calls('?IsWithinDistInMap@WorldObject@@')[0]
flags = calls('?HasFlag@Object@@')[0]
assert any(item.op_str == 'edx, 0x11' for item in body[flags - 3:flags]) and any(item.op_str == 'r8d, 0x10' for item in body[flags - 3:flags])
entry = calls('?GetUInt32Value@Object@@')[0]
assert body[entry + 1].op_str == 'eax, 0x3ce4c'
reject(entry + 1, 'jne')
statuses = calls('?GetQuestStatus@Player@@')
assert len(statuses) == 2 and all(any(item.op_str == 'edx, 0xa421' for item in body[index - 2:index]) for index in statuses)
assert body[statuses[0] + 1].op_str == 'eax, 3' and body[statuses[0] + 2].mnemonic == 'je'
assert body[statuses[0] + 2].operands[0].imm == body[statuses[1] + 3].address
assert body[statuses[1] + 1].op_str == 'eax, 1'
reject(statuses[1] + 1, 'jne')
scenario = calls('?GetScenario@ScenarioMgr@@')[0]
assert any(item.op_str == 'edx, dword ptr [rdi + 0x3b8]' for item in body[scenario - 3:scenario])
assert body[scenario + 2].op_str == 'rax, rax'
reject(scenario + 2, 'je')
identity = calls('?GetScenarioId@Scenario@@')[0]
assert body[identity + 1].op_str == 'eax, 0x43a'
reject(identity + 1, 'jne')
step = calls('?GetCurrentStep@Scenario@@')[0]
assert body[step + 1].op_str == 'eax, 5'
reject(step + 1, 'jne')
for prefix in (gossip, ai_gossip):
    callback = instructions(prefix)
    gate = next(index for index, item in enumerate(callback) if item.mnemonic == 'call'
                and item.operands[0].type == CS_OP_IMM and native['resolve'](item.operands[0].imm) == native['symbol'](helper))
    assert callback[gate + 1].op_str == 'al, al' and callback[gate + 2].mnemonic == 'je'
    rejected = callback[gate + 2].operands[0].imm
    rejected_index = next(index for index, item in enumerate(callback) if item.address == rejected)
    terminal = next(index for index in range(rejected_index, len(callback)) if callback[index].mnemonic == 'ret')
    assert any(item.mnemonic == 'mov' and item.op_str == 'al, 1' for item in callback[rejected_index:terminal])
    assert all(item.mnemonic in ('mov', 'add', 'pop') for item in callback[rejected_index:terminal])
    state = next(index for index, item in enumerate(callback) if item.mnemonic == 'call' and item.operands[0].type == CS_OP_IMM
                 and native['resolve'](item.operands[0].imm) == native['symbol']('?getLootState@GameObject@@'))
    assert callback[state + 1].op_str == 'eax, 3' and callback[state + 2].mnemonic == 'je' and callback[state + 2].operands[0].imm == rejected
    item_checks = [index for index, item in enumerate(callback) if item.mnemonic == 'call' and item.operands[0].type == CS_OP_IMM
                   and native['resolve'](item.operands[0].imm) == native['symbol']('?HasItemCount@Player@@')]
    assert len(item_checks) == (2 if prefix == gossip else 1)
    assert any(item.op_str == 'r9b, 1' for item in callback[item_checks[0] - 5:item_checks[0]])
    if prefix == gossip:
        assert any(item.mnemonic == 'xor' and item.op_str == 'r9d, r9d' for item in callback[item_checks[1] - 5:item_checks[1]])
        assert callback[item_checks[1] + 2].mnemonic == 'je' and callback[item_checks[1] + 2].operands[0].imm == rejected
callback = instructions(loot)


def clean_exit(address):
    first = next(index for index, item in enumerate(callback) if item.address == address)
    for item in callback[first:]:
        if item.mnemonic == 'ret':
            return
        assert item.mnemonic in ('mov', 'add', 'pop', 'nop'), 'Rejected state reaches calls or non-epilogue instructions'
    raise AssertionError('Rejected state has no terminal return')


gate = next(index for index, item in enumerate(callback) if item.mnemonic == 'call' and item.operands[0].type == CS_OP_IMM
            and native['resolve'](item.operands[0].imm) == native['symbol'](helper))
assert callback[gate + 1].op_str == 'al, al' and callback[gate + 2].mnemonic == 'je'
rejected = callback[gate + 2].operands[0].imm
for operands in ('r8d, 3', 'eax, 3'):
    position = next(index for index, item in enumerate(callback[:gate]) if item.mnemonic == 'cmp' and item.op_str == operands)
    assert callback[position + 1].mnemonic == 'jne'
    clean_exit(callback[position + 1].operands[0].imm)
owned = next(index for index, item in enumerate(callback) if item.mnemonic == 'call' and item.operands[0].type == CS_OP_IMM
             and native['resolve'](item.operands[0].imm) == native['symbol']('?HasItemCount@Player@@'))
assert callback[owned + 1].op_str == 'al, al' and callback[owned + 2].mnemonic == 'je' and callback[owned + 2].operands[0].imm == rejected
assert all(any(item.op_str == value for item in callback[owned - 8:owned]) for value in ('edx, 0x1f762', 'r8d, 1', 'r9d, r9d'))
effects = [index for index, item in enumerate(callback) if item.mnemonic == 'call' and item.operands[0].type == CS_OP_IMM
           and native['resolve'](item.operands[0].imm) == native['symbol']('?DoUpdateAchievementCriteria@InstanceScript@@')]
assert effects and min(effects) > owned > gate
assert not any(item.mnemonic == 'call' and item.operands[0].type == CS_OP_IMM
               and native['resolve'](item.operands[0].imm) == native['symbol']('?Delete@GameObject@@') for item in callback)
tables = []
for item in instructions('?AddSC_shields_rest@@'):
    for operand in item.operands:
        if item.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            address = item.address + item.size + operand.mem.disp
            offset = native['offset'](address)
            if all(struct.unpack_from('<Q', native['binary'], offset + slot)[0] == native['symbol'](prefix)
                   for slot, prefix in ((0x10, gossip), (0x50, loot), (0x60, factory))):
                tables.append(address)
assert len(tables) == 1
ai_tables = []
assert any(item.op_str == 'qword ptr [rax + 8], rbx' for item in instructions(factory))
for item in instructions(factory):
    if item.mnemonic == 'lea' and item.operands[1].type == CS_OP_MEM and item.operands[1].mem.base == X86_REG_RIP:
        address = item.address + item.size + item.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x48)[0] == native['symbol'](ai_gossip):
            ai_tables.append(address)
assert len(ai_tables) == 1
assert native['symbol']('?GetGameObjectAI@ScriptMgr@@') in native['calls']('?SelectGameObjectAI@FactorySelector@@')
assert any(item.mnemonic == 'jmp' and item.operands[0].type == CS_OP_MEM and item.operands[0].mem.disp == 0x60
           for item in instructions('?GetGameObjectAI@ScriptMgr@@'))
for prefix in ('?HandleGameobjectReportUse@WorldSession@@', '?SendLoot@Spell@@'):
    dispatch_body = instructions(prefix)
    slot = next(index for index, item in enumerate(dispatch_body) if item.mnemonic == 'mov' and item.op_str == 'r9, qword ptr [rcx + 0x48]')
    invocation = next(index for index in range(slot + 1, slot + 6) if dispatch_body[index].mnemonic == 'call')
    assert dispatch_body[invocation].op_str == 'r9' and any(item.op_str == 'rcx, rax' for item in dispatch_body[slot + 1:invocation])
    assert not any(item.op_str.startswith('r9,') for item in dispatch_body[slot + 1:invocation]), f'Loaded AI gossip slot replaced: {prefix}'
dispatch = '?OnGossipHello@ScriptMgr@@QEAA_NPEAVPlayer@@PEAVGameObject@@'
assert native['symbol'](dispatch) in native['calls']('?Use@GameObject@@QEAAXPEAVUnit@@')
assert native['symbol']('?Use@GameObject@@QEAAXPEAVUnit@@') in native['calls']('?HandleGameObjectUse@WorldSession@@')
print('PASS: installed Truthguard use/open/report factory and callback require live1495/1082/5/42017 Paladin context and reported/stored completed state plus carried128866. No grant; real loot link, equip acknowledgment and client remain pending.')

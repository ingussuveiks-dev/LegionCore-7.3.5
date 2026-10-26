"""Verify the selected initial Ashbringer context gate, not delayed spell success."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_ashbringer_pickup_completion_release.py')))
native, instructions = previous['native'], previous['instructions']
helper = '?CanRetrieve@AshbringerAcquisition@@'
gossip = '?OnGossipHello@go_bs_the_ashbringer@@'
ai_gossip = '?GossipHello@AI@go_bs_the_ashbringer@@'
factory = '?GetAI@go_bs_the_ashbringer@@'
loot = '?OnLootStateChanged@go_bs_the_ashbringer@@'
body = instructions(helper)


def calls(prefix, selected=None):
    selected = body if selected is None else selected
    targets = {address for name, address in native['symbols'].items() if name.startswith(prefix)}
    return [index for index, entry in enumerate(selected) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) in targets]


deny = next(entry.address for index, entry in enumerate(body[:-1]) if entry.op_str == 'rbx, qword ptr [rsp + 0x40]'
            and body[index + 1].mnemonic == 'xor' and body[index + 1].op_str == 'al, al')


def reject(position, branch):
    assert body[position + 1].mnemonic == branch and body[position + 1].operands[0].imm == deny


for operands, branch in (('rcx, rcx', 'je'), ('rdx, rdx', 'je'), ('dword ptr [rbx + 0xa18], 0', 'jne'),
                         ('byte ptr [rbx + 0x6848], 0', 'jne'), ('byte ptr [rbx + 0x6849], 0', 'jne'),
                         ('dword ptr [rdi + 0xe0], 0x5dc', 'jne'), ('rax, rax', 'je'),
                         ('rax, qword ptr [rdi + 8]', 'jne'), ('dword ptr [rbx + 0x3b8], eax', 'jne'),
                         ('rdi, rdi', 'je'), ('rbx, rbx', 'je')):
    check = next(index for index, entry in enumerate(body) if entry.mnemonic in ('cmp', 'test') and entry.op_str == operands)
    reject(check, branch)
for prefix, branch in (('?IsInWorld@Object@@', 'je'), ('?CanContact@Player@@', 'je'), ('?isSpawned@GameObject@@', 'je'),
                       ('?InSamePhase@WorldObject@@', 'je'), ('?canSeeOrDetect@WorldObject@@', 'je'),
                       ('?IsWithinLOSInMap@WorldObject@@', 'je'), ('?IsWithinDistInMap@WorldObject@@', 'je'),
                       ('?HasFlag@Object@@', 'jne'), ('?GetQuestRewardStatus@Player@@', 'jne')):
    positions = calls(prefix)
    assert len(positions) == (2 if prefix == '?IsInWorld@Object@@' else 1)
    for position in positions:
        assert body[position + 1].op_str == 'al, al'
        reject(position + 1, branch)
klass, = calls('?GetByteValue@Object@@')
assert body[klass + 1].op_str == 'al, 2'
reject(klass + 1, 'jne')
distance, = calls('?GetInteractionDistance@GameObject@@')
assert body[distance + 1].op_str == 'xmm2, xmm0' and distance < calls('?IsWithinDistInMap@WorldObject@@')[0]
flags, = calls('?HasFlag@Object@@')
assert all(any(entry.op_str == operand for entry in body[flags - 3:flags]) for operand in ('edx, 0x11', 'r8d, 0x10'))
entry, = calls('?GetUInt32Value@Object@@')
assert body[entry + 1].op_str == 'eax, 0x3c616'
reject(entry + 1, 'jne')
statuses = calls('?GetQuestStatus@Player@@')
assert len(statuses) == 2 and all(any(entry.op_str == 'edx, 0x95e8' for entry in body[index - 2:index]) for index in statuses)
assert body[statuses[0] + 1].op_str == 'eax, 3' and body[statuses[0] + 2].mnemonic == 'je'
assert body[statuses[0] + 2].operands[0].imm == body[statuses[1] + 3].address
assert body[statuses[1] + 1].op_str == 'eax, 1'
reject(statuses[1] + 1, 'jne')
scenario, = calls('?GetScenario@ScenarioMgr@@')
assert any(entry.op_str == 'edx, dword ptr [rdi + 0x3b8]' for entry in body[scenario - 3:scenario])
identity, = calls('?GetScenarioId@Scenario@@')
assert body[identity + 1].op_str == 'eax, 0x307'
reject(identity + 1, 'jne')
for prefix in ('?GetCurrentStep@Scenario@@', '?getScenarionStep@InstanceScript@@'):
    step, = calls(prefix)
    assert body[step + 1].op_str == 'eax, 3'
    reject(step + 1, 'jne')
assert len(calls('?GetInstanceScript@WorldObject@@')) == 1
for prefix in (gossip, ai_gossip):
    callback = instructions(prefix)
    gate, = calls(helper, callback)
    assert callback[gate + 1].mnemonic == 'xor' and callback[gate + 1].op_str == 'al, 1'
    assert [entry.mnemonic for entry in callback[gate + 2:]] == ['add', 'ret']
callback = instructions(loot)
gate, = calls(helper, callback)
assert callback[gate + 1].op_str == 'al, al' and callback[gate + 2].mnemonic == 'je'
rejected = callback[gate + 2].operands[0].imm
first = next(index for index, entry in enumerate(callback) if entry.address == rejected)
assert callback[first].op_str == 'rcx, qword ptr [rsp + 0xd8]' and callback[first + 1].op_str == 'rcx, rsp'
assert callback[first + 2].mnemonic == 'call'
assert all(entry.mnemonic in ('mov', 'add', 'pop', 'ret') for entry in callback[first + 3:])
assert gate < calls('?CanStoreItem@Player@@', callback)[0] < calls('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N', callback)[0]

tables = []
for entry in instructions('?AddSC_paladin_broken_shore@@'):
    if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if all(struct.unpack_from('<Q', native['binary'], native['offset'](address) + slot)[0] == native['symbol'](prefix)
               for slot, prefix in ((0x10, gossip), (0x50, loot), (0x60, factory))):
            tables.append(address)
assert len(tables) == 1
ai_tables = []
assert any(entry.op_str == 'qword ptr [rax + 8], rbx' for entry in instructions(factory))
for entry in instructions(factory):
    if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x48)[0] == native['symbol'](ai_gossip):
            ai_tables.append(address)
assert len(ai_tables) == 1
assert native['symbol']('?GetGameObjectAI@ScriptMgr@@') in native['calls']('?SelectGameObjectAI@FactorySelector@@')
assert any(entry.mnemonic == 'jmp' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x60
           for entry in instructions('?GetGameObjectAI@ScriptMgr@@'))
for prefix in ('?HandleGameobjectReportUse@WorldSession@@', '?SendLoot@Spell@@'):
    dispatch = instructions(prefix)
    slot = next(index for index, entry in enumerate(dispatch) if entry.op_str == 'r9, qword ptr [rcx + 0x48]')
    invocation = next(index for index in range(slot + 1, slot + 6) if dispatch[index].mnemonic == 'call')
    assert dispatch[invocation].op_str == 'r9'
    assert not any(entry.op_str.startswith('r9,') for entry in dispatch[slot + 1:invocation])
dispatch = '?OnGossipHello@ScriptMgr@@QEAA_NPEAVPlayer@@PEAVGameObject@@'
assert native['symbol'](dispatch) in native['calls']('?Use@GameObject@@QEAAXPEAVUnit@@')
assert native['symbol']('?Use@GameObject@@QEAAXPEAVUnit@@') in native['calls']('?HandleGameObjectUse@WorldSession@@')
print('PASS installed selected gossip10/loot50/factory60 and AI48: live247318/1500/same map-instance-phase/LOS/range/selectability, Paladin active38376 and native775/3 plus legacy3 gate precedes retrieval. Report-use ignores the AI return and retains native generic use credit/stealth removal; no retrieval effects in AI. Not delayed child success, eligible party fanout, actual loot/equip or client/VFX proof.')

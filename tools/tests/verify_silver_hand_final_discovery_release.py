"""Verify installed instance-owned late finale actor discovery and admission."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_final_cast_release.py')))
native, instructions, sites, leaf = (previous[key] for key in ('native', 'instructions', 'sites', 'leaf'))
helper_name = '?QueueFinaleDiscovery@instance_silver_hand_InstanceMapScript@'
helper = instructions(helper_name)
arm, = sites(helper, '?AddDelayedEvent@InstanceScript@@')
pending, = [index for index, entry in enumerate(helper) if entry.op_str == 'byte ptr [rcx + 0x308], 0']
assert helper[pending].mnemonic == 'cmp' and helper[pending + 1].mnemonic == 'jne'
assert helper[pending + 2].op_str == 'byte ptr [rcx + 0x308], 1'
skip = next(index for index, entry in enumerate(helper) if entry.address == helper[pending + 1].operands[0].imm)
assert pending < arm < skip and helper[skip].op_str == 'rcx, qword ptr [rsp + 0x100]'
assert [entry.op_str for entry in helper[arm - 3:arm]] == ['r8, [rsp + 0xa0]', 'edx, 0x3e8', 'rcx, rbx']
assert any(entry.op_str == 'rbx, rcx' for entry in helper[:pending])
assert any(entry.op_str == 'qword ptr [rsp + 0xa8], rbx' for entry in helper[:arm])
closure, = [name for name in native['symbols'] if name.startswith('?_Do_call@') and 'QueueFinaleDiscovery@instance_silver_hand_InstanceMapScript@' in name]
tables = []
for index, entry in enumerate(helper[:arm]):
    if entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x10)[0] == native['symbol'](closure):
            assert helper[index + 1].op_str == 'qword ptr [rsp + 0xa0], rax'
            tables.append(address)
assert len(tables) == 1
thunk = leaf(closure, 9)
lambda_name, = [name for name in native['symbols'] if name.startswith('??R<lambda_1>@') and 'QueueFinaleDiscovery@instance_silver_hand_InstanceMapScript@' in name]
assert thunk[0].op_str == 'rcx, 8' and thunk[1].mnemonic == 'jmp' and thunk[1].operands[0].imm == native['symbol'](lambda_name)
callback = instructions(lambda_name)
reset, = [index for index, entry in enumerate(callback) if entry.op_str == 'byte ptr [rax + 0x308], 0']
scenario, = sites(callback, '?GetScenario@ScenarioMgr@@')
scenario_id, = sites(callback, '?GetScenarioId@Scenario@@')
stage, = sites(callback, '?GetCurrentStep@Scenario@@')
dispatch, = [index for index, entry in enumerate(callback) if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax + 0x118]']
assert reset < scenario < scenario_id < stage < dispatch
assert callback[reset - 1].op_str == 'rax, qword ptr [rcx]'
assert [entry.op_str for entry in callback[scenario - 5:scenario - 3]] == ['rcx, qword ptr [rdi]', 'rcx, qword ptr [rcx + 0x10]']
gate, = [index for index, entry in enumerate(callback[:scenario_id]) if entry.op_str == 'rax, rax' and callback[index + 1].mnemonic == 'je']
deny = callback[gate + 1].operands[0].imm
for method, value in ((scenario_id, 'eax, 0x444'), (stage, 'eax, 0xa')):
    assert callback[method + 1].mnemonic == 'cmp' and callback[method + 1].op_str == value
    assert callback[method + 2].mnemonic == 'jne' and callback[method + 2].operands[0].imm == deny
assert [entry.op_str for entry in callback[dispatch - 3:dispatch]] == ['rcx, qword ptr [rdi]', 'rax, qword ptr [rcx]', 'edx, 0xa']
assert callback[dispatch + 1].mnemonic == 'jmp' and callback[dispatch + 1].operands[0].imm > deny
for body, message in ((helper, b'Silver Hand finale: missing actors; retry discovery in1000ms'),
                      (callback, b'Silver Hand finale: dropping stale actor discovery')):
    assert any(entry.mnemonic == 'lea' and len(entry.operands) == 2 and entry.operands[1].type == CS_OP_MEM
               and entry.operands[1].mem.base == X86_REG_RIP
               and native['binary'].startswith(message, native['offset'](entry.address + entry.size + entry.operands[1].mem.disp)) for entry in body)
    for method in ('?AddDelayedEvent@Unit@@', '?GetCreature@Map@@', '?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@', '?AddItem@Player@@'):
        assert not sites(body, method)
    for method in (name for name in native['symbols'] if name.startswith('?SummonCreature@')):
        assert not sites(body, method)
assert not sites(callback, '?AddDelayedEvent@InstanceScript@@') and not sites(callback, helper_name)
print('PASS selected finale discovery uses factory-zeroed308 pending byte, sets it before single1000ms native instance queue130 and captures only instance through actual function table/9-byte thunk. Callback clears latch before fresh actual-instance1092/exact10 checks and selected virtual118 reentry; stale work logs/drops without direct requeue. Linked proofs retain empty/exhausted participant triggers, independent live70yd actors and full128-bit speech/cast admission; no raw Player/actor capture, spawn, credit or item bypass. Not persisted attempt or client traversal/VFX proof.')

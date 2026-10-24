"""Verify the final Vigfus visibility policy and explicit stage-change refresh."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_admission_release.py')))
native = previous['native']
instructions = previous['instructions']
actor = '@npc_vigfus_bladewind_lastAI@npc_vigfus_bladewind_last@@'
visible = '?CanBeVisibleFor' + actor
body = instructions(visible)
values = {operand.imm for entry in body for operand in entry.operands if operand.type == CS_OP_IMM}
assert {98602, 1511, 944, 5, 7} <= values, 'Visibility policy lost exact actor/map/scenario/stage scope'
for value, branch in ((5, 'jb'), (7, 'jbe')):
    comparison = next(index for index, entry in enumerate(body) if entry.mnemonic == 'cmp' and entry.operands[-1].type == CS_OP_IMM and entry.operands[-1].imm == value)
    assert body[comparison + 1].mnemonic == branch, 'Visibility no longer admits exactly5..7 with terminal corpse retained'
for dependency in ('?GetScenario@ScenarioMgr@@', '?GetScenarioId@Scenario@@', '?GetCurrentStep@Scenario@@'):
    assert native['symbol'](dependency) in native['calls'](visible), f'Visibility uses stale rather than actual progress: {dependency}'


def tables_for(factory, required):
    tables = []
    for entry in instructions(factory):
        for operand in entry.operands:
            if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
                offset = native['offset'](entry.address + entry.size + operand.mem.disp)
                slots = struct.unpack_from('<128Q', native['binary'], offset)
                if all(slots[slot // 8] == native['symbol'](prefix) for slot, prefix in required):
                    tables.append(slots)
    assert len(tables) == 1, f'Factory does not select the expected callbacks: {factory}'
    return tables[0]


tables_for('?GetAI@npc_vigfus_bladewind_last@@', ((0x118, visible), (0xa8, '?DamageTakenAfterScaling' + actor)))
dispatches = [entry for entry in instructions('?IsNeverVisible@Creature@@') if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x118]
assert len(dispatches) == 2, 'Native visibility must query both current and suspended original AI'
instance = '@instance_warswords_InstanceMapScript@instance_warswords@@'
create = '?OnCreatureCreate' + instance
remove = '?OnCreatureRemove' + instance
step = '?onScenarionNextStep' + instance
tables_for('?GetInstanceScript@instance_warswords@@', ((0x18, create), (0x20, remove), (0x118, step)))
for consumer, slot in (('?AddToWorld@Creature@@', 0x18), ('?RemoveFromWorld@Creature@@', 0x20), ('?SetCurrentStep@Scenario@@', 0x118), ('?UpdateCurrentStep@Scenario@@', 0x118)):
    assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == slot for entry in instructions(consumer)), f'Native lifecycle misses the selected instance callback: {consumer}'

creation = instructions(create)
registration = next(index for index, entry in enumerate(creation) if entry.mnemonic == 'movups' and entry.op_str == 'xmmword ptr [rax + 0x20], xmm6')
bootstrap = next(index for index, entry in enumerate(creation) if entry.mnemonic == 'cmp' and entry.op_str == 'byte ptr [rbp + 0x300], 0')
assert registration < bootstrap, 'Late bootstrap skips final GUID registration'
for prefix in (create, remove, step):
    assert any(operand.type == CS_OP_IMM and operand.imm == 98602 for entry in instructions(prefix) for operand in entry.operands), 'Instance registry no longer scopes final98602'
    assert any(operand.type == CS_OP_IMM and operand.imm == 98007 for entry in instructions(prefix) for operand in entry.operands), 'Instance registry no longer scopes first98007'
assert native['symbol']('?OnCreatureRemove@InstanceScript@@') in native['calls'](remove), 'Removal bypasses native object/minion cleanup'
assert native['symbol']('??8ObjectGuid@@') in native['calls'](remove), 'Stale removal can erase a replacement actor'

transition = instructions(step)
lookup = native['symbol']('?GetCreature@Map@@')
assert lookup in native['calls'](step), 'Visibility refresh does not resolve the tracked actor on its actual instance map'
refresh = next(index for index, entry in enumerate(transition) if entry.mnemonic == 'mov' and entry.op_str == 'r8, qword ptr [rcx + 0xf0]')
assert transition[refresh + 1].op_str == 'dl, 1' and transition[refresh + 3].mnemonic == 'call' and transition[refresh + 3].op_str == 'r8', 'Stage transition does not force the real Unit visibility update'
bootstrap = next(index for index, entry in enumerate(transition) if entry.mnemonic == 'cmp' and entry.op_str == 'byte ptr [rdi + 0x300], 0')
assert refresh < bootstrap, 'Early myscenario gate skips stationary visibility changes'
assert native['symbol']('?UpdateVisibility@VisibilityUpdateTask@@') in native['calls']('?UpdateObjectVisibility@Unit@@'), 'Forced native Unit refresh is missing'
assert native['symbol']('?UpdateObjectVisibility@WorldObject@@') in native['calls']('?UpdateVisibility@VisibilityUpdateTask@@'), 'Native refresh does not reach nearby-object visibility'
print('PASS: canonical final98602 selected visibility slot0x118 admits actual1511/944/5..7 including terminal completion, native current/suspended-AI dispatch and instance create/remove/stage callbacks, pre-bootstrap GUID registration and forced Unit visibility refresh before myscenario; native notifier path retained. Helya, full client visibility/timing and all36 routes remain separate.')

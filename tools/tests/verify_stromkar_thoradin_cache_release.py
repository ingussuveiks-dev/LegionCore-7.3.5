"""Inspect the selected boss summon slot and bound condition's live GUID lookup."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM

previous = runpy.run_path(str(Path(__file__).with_name('verify_stromkar_thoradin_presence_release.py')))
native = previous['native']
instructions = previous['instructions']
policy = '?OnConditionCheck@condition_stromkar_thoradin_absence@@'
created = '?JustSummoned@npc_stromkar_zakajz@@'
assert native['symbol']('?JustSummoned@SmartAI@@') in native['calls'](created), 'Summon override bypasses native callback'
body = instructions(policy)
values = {operand.imm for entry in body for operand in entry.operands if operand.type == CS_OP_IMM}
assert {22, 104276, 10, 29, 104307, 50}.issubset(values), 'Inlined condition scope missing'
lookups = {address for name, address in native['symbols'].items() if name.startswith('?GetCreature@ObjectAccessor@@') or name.startswith('?GetCreature@Map@@')}
assert lookups & native['calls'](policy), 'Saved GUID is not resolved against the owner map'
assert native['symbol']('?FindNearestCreature@WorldObject@@') not in native['calls'](policy), 'GUID existence was replaced with the unsafe proximity test'
assert any(entry.mnemonic == 'movups' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x200 for entry in instructions(created)), 'Summon callback does not save the actual16-byte cache GUID'
assert any(entry.mnemonic == 'movups' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.disp == 0x200 for entry in body), 'Inlined policy does not read the callback cache field'
for dependency in ('?IsInWorld@Object@@', '?InSamePhase@WorldObject@@'):
    assert native['symbol'](dependency) in native['calls'](policy), f'Missing live actor guard: {dependency}'
assert any(entry.mnemonic == 'cmp' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0xa18 and entry.operands[-1].type == CS_OP_IMM and entry.operands[-1].imm == 0 for entry in body), 'Inlined alive-state check missing'
assert any(operand.type == CS_OP_IMM and operand.imm == 104307 for entry in instructions(created) for operand in entry.operands), 'Callback caches an unrelated entry'
assert native['symbol']('?OnConditionCheck@ScriptMgr@@') in native['calls']('?Meets@Condition@@'), 'Native condition evaluator omits script conjunction'
zakajz = runpy.run_path(str(Path(__file__).with_name('verify_stromkar_zakajz_release.py')))
assert struct.unpack_from('<Q', native['binary'], zakajz['tables'][0] + 0x140)[0] == native['symbol'](created), 'Selected boss vtable does not install summon tracking'
assert b'condition_stromkar_thoradin_absence' in native['binary'], 'Condition registration name missing'
selector = instructions('?selectAI@FactorySelector@@')
script_factory = native['symbol']('?GetCreatureAI@ScriptMgr@@')
script_call = next(index for index, entry in enumerate(selector) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == script_factory)
assert selector[script_call + 1].mnemonic == 'test' and selector[script_call + 1].op_str == 'rax, rax', 'Selector does not test the scripted AI result'
script_return = selector[script_call + 2]
assert script_return.mnemonic == 'jne' and script_return.operands[0].type == CS_OP_IMM, 'Scripted AI lost its nonnull early return'
name_lookup = native['symbol']('?GetAIName@Creature@@')
name_call = next(entry for entry in selector if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == name_lookup)
assert selector[script_call].address < name_call.address < script_return.operands[0].imm, 'AIName fallback precedes the ScriptName selection'
return_path = [entry for entry in selector if entry.address >= script_return.operands[0].imm]
return_path = return_path[:next(index for index, entry in enumerate(return_path) if entry.mnemonic == 'ret') + 1]
assert not any(entry.mnemonic == 'mov' and entry.operands[0].type != CS_OP_MEM and entry.op_str.startswith(('rax,', 'eax,')) for entry in return_path), 'Selector overwrites the returned scripted AI'
factory_dispatch = instructions('?GetCreatureAI@ScriptMgr@@')
assert native['symbol']('?GetScriptId@Creature@@') in native['calls']('?GetCreatureAI@ScriptMgr@@'), 'Script manager does not resolve the creature script binding'
assert any(entry.mnemonic == 'mov' and entry.operands[-1].type == CS_OP_MEM and entry.operands[-1].mem.disp == 0x50 for entry in factory_dispatch), 'CreatureScript GetAI virtual slot missing'
print('PASS: canonical ScriptName factory returns before AIName fallback; its boss vtable tracks real JustSummoned, preserves SmartAI dispatch and resolves live104307 by GUID without a range cutoff. The bound condition reaches that cache through native Condition::Meets; full client travel, visual and actor cleanup remain separate')

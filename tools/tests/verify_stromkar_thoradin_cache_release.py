"""Inspect the selected boss summon slot and bound condition's live GUID lookup."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG

previous = runpy.run_path(str(Path(__file__).with_name('verify_stromkar_thoradin_presence_release.py')))
native = previous['native']
instructions = previous['instructions']
policy = '?OnConditionCheck@condition_stromkar_thoradin_absence@@'
created = '?JustSummoned@npc_stromkar_zakajz@@'
claim = '?ClaimThoradin@npc_stromkar_zakajz@@'
valid = '?IsFinalThoradin@npc_stromkar_zakajz@@'
summoned = '?IsSummonedBy@npc_stromkar_thoradin@@'
assert native['symbol']('?JustSummoned@SmartAI@@') in native['calls'](created), 'Summon override bypasses native callback'
assert native['symbol'](claim) in native['calls'](created), 'Owner does not claim before SmartAI'
assert native['symbol'](claim) in native['calls'](summoned), 'Independent actor notification does not claim'
owner_body = list(instructions(created))
def transfers(body, target):
    return [x for x in body if x.mnemonic in ('call', 'jmp') and x.operands[0].type == CS_OP_IMM
            and native['resolve'](x.operands[0].imm) == native['symbol'](target)]
assert transfers(owner_body, claim)[0].address < transfers(owner_body, '?JustSummoned@SmartAI@@')[0].address, 'Owner claim follows reentrant SmartAI dispatch'
body = instructions(policy)
values = {operand.imm for entry in body for operand in entry.operands if operand.type == CS_OP_IMM}
assert {22, 104276, 10, 29, 104307, 50}.issubset(values), 'Inlined condition scope missing'
lookups = {address for name, address in native['symbols'].items() if name.startswith('?GetCreature@ObjectAccessor@@') or name.startswith('?GetCreature@Map@@')}
assert lookups & native['calls'](policy), 'Saved GUID is not resolved against the owner map'
assert native['symbol']('?FindNearestCreature@WorldObject@@') not in native['calls'](policy), 'GUID existence was replaced with the unsafe proximity test'
claim_body = list(instructions(claim))
cache_registers = {x.operands[0].reg for x in claim_body if x.mnemonic == 'lea'
                   and x.operands[0].type == CS_OP_REG and x.operands[1].type == CS_OP_MEM
                   and x.operands[1].mem.disp == 0x200}
assert cache_registers, 'Claim does not address the real cache field'
assert any(x.mnemonic in ('movups', 'movdqu') and x.operands[0].type == CS_OP_MEM
           and x.operands[0].size == 16 and x.operands[0].mem.base in cache_registers
           and x.operands[0].mem.disp == 0 for x in claim_body), 'Claim does not save the full16-byte actor GUID'
assert any(entry.mnemonic == 'movups' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.disp == 0x200 for entry in body), 'Inlined policy does not read the callback cache field'
assert native['symbol'](valid) in native['calls'](policy), 'Condition omits owned live actor validation'
for dependency in ('?IsInWorld@Object@@', '?InSamePhase@WorldObject@@', '?GetEntry@ObjectGuid@@'):
    assert native['symbol'](dependency) in native['calls'](valid), f'Missing live owned actor guard: {dependency}'
assert any(entry.mnemonic == 'cmp' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0xa18 and entry.operands[-1].type == CS_OP_IMM and entry.operands[-1].imm == 0 for entry in instructions(valid)), 'Owned actor alive-state check missing'
claim_values = {op.imm for x in claim_body for op in x.operands if op.type == CS_OP_IMM}
assert {104307, 104276, 1539, 1037, 9, 10} <= claim_values, 'Claim widens beyond the native finale'
actor_body = list(instructions(summoned))
initialized = next(x for x in actor_body if x.mnemonic == 'mov' and x.operands[0].type == CS_OP_MEM
                   and x.operands[0].size == 1 and x.operands[0].mem.disp == 0x200
                   and x.operands[1].type == CS_OP_IMM and x.operands[1].imm == 1)
assert initialized.address < transfers(actor_body, claim)[0].address < transfers(actor_body, '?IsSummonedBy@SmartAI@@')[0].address, 'Single initialization/claim must precede native intro'
assert any(x.mnemonic == 'cmp' and x.operands[0].type == CS_OP_MEM and x.operands[0].mem.disp == 0x200
           and x.operands[-1].type == CS_OP_IMM and x.operands[-1].imm == 0 for x in actor_body), 'Repeated actor initialization is not guarded'
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
print('PASS selected boss/actor factories claim before both native notification paths, retain full16-byte cache and single intro, preserve SmartAI and range-free owned/alive/phase GUID lookup through real conditions. Native regression checks reentry/recreation/removal; actual client duplication remains unproven')

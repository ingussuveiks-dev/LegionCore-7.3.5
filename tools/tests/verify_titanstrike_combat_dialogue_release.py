"""Verify the deployed combat-only cast gate and Conversation virtual cleanup."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM

previous = runpy.run_path(str(Path(__file__).with_name('verify_titanstrike_handoff_release.py')))
native = previous['native']
instructions = previous['instructions']
values = previous['values']
references = previous['references']
allow_name = '?Allowed@TitanstrikeCombatDialogue@@'
create_name = '?Create@TitanstrikeCombatDialogue@@'
update_name = '?Update@Story@TitanstrikeCombatDialogue@@'
check_name = '?Check@spell_titanstrike_combat_dialogue@@'
hook_name = '?Create@spell_titanstrike_combat_dialogue@@'
for scoped_name in [allow_name, create_name, update_name, hook_name]:
    assert {106714, 1609}.issubset(values(instructions(scoped_name))), 'Wrong inlined Thorim/map scope'
assert {1099, 2, 48020, 106730, 1}.issubset(values(instructions(allow_name))), 'Wrong active fight/defeat-tree/surrender gate'
for caller, callees in {
    allow_name: ['?GetCriteriaProgressMap@', '?FindNearestCreature@WorldObject@@', '?GetUInt64Value@Object@@', '?InSamePhase@WorldObject@@'],
    create_name: [allow_name, '?FindNearestCreature@WorldObject@@', '?GetGuidValue@Object@@', '?CreateConversation@Conversation@@'],
    update_name: ['?GetCreature@ObjectAccessor@@', allow_name, '?FindNearestCreature@WorldObject@@', '?GetGuidValue@Object@@', '??9ObjectGuid@@', '?InSamePhase@WorldObject@@', '?Remove@Conversation@@', '?Update@Conversation@@'],
    check_name: [allow_name],
    hook_name: ['?PreventHitDefaultEffect@SpellScript@@', '?GetExplTargetDest@SpellScript@@', '?GetHitDest@SpellScript@@', create_name],
}.items():
    targets = native['calls'](caller)
    for callee in callees:
        assert native['symbol'](callee) in targets, f'Missing live combat bridge {caller} -> {callee}'
table = previous['data_symbol']('??_7Story@TitanstrikeCombatDialogue@@6BWorldObject@@@')
assert references(instructions(create_name), table), 'Combat creator does not install scoped Story'
position = native['offset'](table)
base = native['offset'](previous['data_symbol']('??_7Conversation@@6BWorldObject@@@'))
slots = [slot for slot in range(0, 0x180, 8)
         if struct.unpack_from('<Q', native['binary'], position + slot)[0] == native['symbol'](update_name)]
assert len(slots) == 1, 'Missing combat Story Update slot'
assert struct.unpack_from('<Q', native['binary'], base + slots[0])[0] == native['symbol']('?Update@Conversation@@'), 'Wrong native virtual override'
validate = instructions('?Validate@spell_titanstrike_combat_dialogue@@')
assert {211824, 211826, 211827, 2152, 2153, 2154, 219}.issubset(values(validate)), 'Wrong native combat spell/effect/conversation identities'
register = instructions('?Register@spell_titanstrike_combat_dialogue@@')
assert references(register, native['symbol'](check_name)) and references(register, native['symbol'](hook_name)), 'Cast/effect handlers not registered'
hook = instructions(hook_name)
calls = [(index, native['resolve'](entry.operands[0].imm)) for index, entry in enumerate(hook)
         if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM]
suppressed = next(index for index, target in calls if target == native['symbol']('?PreventHitDefaultEffect@SpellScript@@'))
for identity in [106714, 1609]:
    scoped = next(index for index, entry in enumerate(hook) if entry.mnemonic == 'cmp' and
                  any(operand.type == CS_OP_IMM and operand.imm == identity for operand in entry.operands))
    assert scoped < suppressed and hook[scoped + 1].mnemonic == 'jne' and hook[scoped + 1].operands[0].imm > hook[suppressed].address, 'Unrelated casts no longer bypass the custom effect replacement'
allow = instructions(allow_name)
assert any(entry.op_str == 'edx, 0x38' for entry in allow), 'Actual health field changed'
assert any(entry.mnemonic == 'cmp' and entry.op_str == 'rax, 1' and allow[index + 1].mnemonic == 'jbe'
           for index, entry in enumerate(allow[:-1])), 'Actual1HP surrender no longer blocks combat speech'
update = instructions(update_name)
assert any(entry.mnemonic == 'lea' and entry.op_str == 'rdx, [rbx + 0x500]' for entry in update), 'Update no longer compares captured opponent identity'
assert any(entry.mnemonic == 'movups' and '[rsi + 0x500]' in entry.op_str
           for entry in instructions(create_name)), 'Creator does not retain the actual opponent GUID'
print('PASS: deployed combat219 hooks validate native2152/2153/2154, gate Thorim1609/1099/2 before defeat48020/1HP, preserve other casters/default effects and native destinations, install Story Update to retire stale combat dialogue while retaining base cleanup')

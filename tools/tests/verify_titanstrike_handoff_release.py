"""Verify the installed scoped conversation subclass, effect hook and late handoff."""
from pathlib import Path
import re
import runpy
import struct
import sys
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_titanstrike_pickup_admission_release.py')))
native = previous['native']
instructions = previous['instructions']


def values(body):
    return {operand.imm for entry in body for operand in entry.operands if operand.type == CS_OP_IMM}


def references(body, address):
    return any(operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP and
               entry.address + entry.size + operand.mem.disp == address
               for entry in body for operand in entry.operands)


def data_symbol(name):
    pattern = re.compile(r'\s+\w+:\w+\s+' + re.escape(name) + r'\s+([0-9a-fA-F]{16})\s')
    matches = [int(match[1], 16) for line in (Path(sys.argv[1]) / 'worldserver.map').read_text().splitlines()
               if (match := pattern.match(line))]
    assert len(matches) == 1, f'Expected one installed vtable: {name}'
    return matches[0]


current_name = '?Current@TitanstrikeAftermath@@'
advance_name = '?Advance@TitanstrikeAftermath@@'
request_name = '?Request@TitanstrikeAftermath@@'
update_name = '?Update@Story@TitanstrikeAftermath@@'
create_name = '?Create@spell_titanstrike_aftermath@@'
check_name = '?Check@spell_titanstrike_aftermath@@'
current = instructions(current_name)
advance = instructions(advance_name)
request = instructions(request_name)
update = instructions(update_name)
assert {1609, 1099, 42185, 48020, 2, 3}.issubset(values(current)), 'Wrong owner/defeat-tree scope'
assert {103154, 211967, 211970, 48094, 50997, 16777216, 92}.issubset(values(advance)), 'Wrong Hati/visual/credit identity'
for caller, callees in {
    current_name: ['?GetCriteriaProgressMap@', '?GetQuestStatus@Player@@', '?CanContact@Player@@'],
    advance_name: [current_name, '?FindNearestCreature@WorldObject@@', '?InSamePhase@WorldObject@@', '?HasAura@Unit@@', '?SetFlag@Object@@', '?UpdateAchievementCriteria@Player@@'],
    request_name: [current_name, '?CreateConversation@Conversation@@', '?BindQuestObject@Player@@', '?AddQuestDelayedEvent@Player@@', '?GetUInt32Value@Object@@'],
    update_name: [current_name, advance_name, '?GetPlayer@ObjectAccessor@@', '?IsQuestObjectExpired@WorldObject@@', '?InSamePhase@WorldObject@@', '?Remove@Conversation@@'],
    create_name: ['?PreventHitDefaultEffect@SpellScript@@', request_name],
    check_name: [current_name],
}.items():
    targets = native['calls'](caller)
    for callee in callees:
        assert native['symbol'](callee) in targets, f'Missing live bridge: {caller} -> {callee}'

assert {2167, 72398, 42185, 1000, 30}.issubset(values(request)), 'Creation budget or quest-bound retry changed'
assert any(entry.mnemonic == 'cmp' and entry.op_str == 'eax, 0x11ace' and request[index + 1].mnemonic == 'jb'
           for index, entry in enumerate(request[:-1])), 'Short conversations no longer fail closed'
story_table = data_symbol('??_7Story@TitanstrikeAftermath@@6BWorldObject@@@')
assert references(request, story_table), 'Actual creator does not install Story vtable'
story_position = native['offset'](story_table)
base_position = native['offset'](data_symbol('??_7Conversation@@6BWorldObject@@@'))
slots = [slot for slot in range(0, 0x180, 8)
         if struct.unpack_from('<Q', native['binary'], story_position + slot)[0] == native['symbol'](update_name)]
assert len(slots) == 1, 'Installed Story Update slot missing/ambiguous'
assert struct.unpack_from('<Q', native['binary'], base_position + slots[0])[0] == native['symbol']('?Update@Conversation@@'), 'Story overrides the wrong native virtual slot'
assert 1000 in values(update), 'Scoped actor retry is no longer rate-bounded to one second'

deadline = next(index for index, entry in enumerate(update)
                if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and
                native['resolve'](entry.operands[0].imm) == native['symbol']('?GetUInt32Value@Object@@'))
assert update[deadline - 2].op_str == 'edx, 0xc', 'Story reads wrong published deadline field'
assert update[deadline + 1].op_str == 'ebx, dword ptr [rsi + 0x2c]', 'Story does not subtract actual playback start'
assert update[deadline + 2].op_str == 'ebx, eax' and update[deadline + 3].mnemonic == 'jbe', 'Playback deadline can hand off early'
advances = [index for index, entry in enumerate(update) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and
            native['resolve'](entry.operands[0].imm) == native['symbol'](advance_name)]
assert len(advances) == 1 and advances[0] > deadline + 3, 'Handoff no longer follows playback deadline'
assert update[deadline + 3].operands[0].imm > update[advances[0]].address, 'Unexpired playback does not skip the actual handoff'
credit_calls = [index for index, entry in enumerate(advance) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and
                native['resolve'](entry.operands[0].imm) == native['symbol']('?UpdateAchievementCriteria@Player@@')]
assert len(credit_calls) == 1, 'Unexpected repeated credit call sites'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'byte ptr [rsi], 1'
           for entry in advance[max(0, credit_calls[0] - 12):credit_calls[0]]), 'Exactly-once flag is not written before credit'
for caller in [create_name, check_name]:
    assert any(entry.mnemonic == 'cmp' and entry.op_str == 'byte ptr [rax + 0x16], 4'
               for entry in instructions(caller)), 'Non-player cast admission changed'
validate = instructions('?Validate@spell_titanstrike_aftermath@@')
assert {211960, 219, 2167}.issubset(values(validate)), 'Native spell/effect/misc validation changed'
register = instructions('?Register@spell_titanstrike_aftermath@@')
assert references(register, native['symbol'](create_name)) and references(register, native['symbol'](check_name)), 'Actual cast/effect handlers are not registered'
assert b'spell_titanstrike_aftermath' in native['binary'], 'Spell factory name missing'
print('PASS: actual Release installs scoped Story Update vtable and219 hook; validates real owner/48020 defeat, suppresses default duplicate creation, checks published playback deadline before guarded50997/flag, retains quest-bound retries and phase/scope cleanup')

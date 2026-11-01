"""Inspect the actual registered scoped intro, native conversation and event path."""
from pathlib import Path
import hashlib
import json
import runpy
import struct

from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values, floats, fields

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
checked = []


def require(prefix, targets=(), constants=()):
    if prefix.startswith('?Validate@spell_fuzan_temple_'):
        # These call-free leaf predicates have no .pdata entry. Bound by the
        # next public symbol, keep the full branch graph and reject padding.
        address = n['symbol'](prefix)
        following = min(value for value in n['symbols'].values() if value > address)
        assert following - address <= 256
        body = list(n['md'].disasm(n['binary'][n['offset'](address):n['offset'](address) + following-address], address))
        while body and body[-1].mnemonic in ('int3', 'nop'):
            body.pop()
        assert body[-1].mnemonic == 'ret' and not any(op.mnemonic == 'call' for op in body)
        nodes = {op.address for op in body}
        assert all(op.operands[0].imm in nodes for op in body if op.mnemonic.startswith('j'))
    else:
        body = n['instructions'](prefix)
    for target in targets:
        assert sites(n, body, target), (prefix, target)
    assert set(constants) <= values(body), (prefix, set(constants) - values(body))
    checked.append(prefix)
    return body


def references(body):
    addresses = set()
    for op in body:
        for operand in op.operands:
            if operand.type == CS_OP_IMM:
                addresses.add(operand.imm)
            elif operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
                addresses.add(op.address + op.size + operand.mem.disp)
    return addresses


def installed_functions(body):
    installed = set()
    for address in references(body):
        try:
            for target in struct.unpack_from('<16Q', n['binary'], n['offset'](address)):
                if n['base'] <= target < n['base'] + 0x10000000:
                    installed.add(n['resolve'](target))
        except (ValueError, struct.error):
            continue
    return installed


world = require('?World@FuZanTempleIntro@@',
                ('?IsInWorld@Object@@', '?IsPreDelete@Object@@', '?GetScenario@ScenarioMgr@@'),
                (109413, 373392, 1625, 1137))
require('?Receipt@FuZanTempleIntro@@', ('?GetAchievementMgr@Scenario@@',), (49115,))
require('?Current@Actor@FuZanTempleIntro@@',
        ('?getQuestStatus@Player@@', '?CanContact@Player@@', '?InSamePhase@WorldObject@@',
         '?GetQuestRewardStatus@Player@@', '?GetQuestStatus@Player@@', '??8ObjectGuid@@'), (42765, 10, 1, 3))
los = require('?MoveInLineOfSight@Actor@FuZanTempleIntro@@',
              ('?IsObjectMeetingSmartEventConditions@ConditionMgr@@', '?AddQuestDelayedEvent@Player@@',
               '?IsWithinLOSInMap@WorldObject@@', '?MoveInLineOfSight@SmartAI@@'), (109413, 200, 42765))
assert 16.0 in floats(n, los)
update = require('?UpdateAI@Actor@FuZanTempleIntro@@',
                 ('?Current@Actor@FuZanTempleIntro@@', '?Receipt@FuZanTempleIntro@@',
                  '?GetConversation@ObjectAccessor@@', '?Exhaust@Actor@FuZanTempleIntro@@',
                  '?CastSpell@Unit@@', '?UpdateAI@SmartAI@@'), (120000, 30, 217774))
assert any(op.mnemonic == 'sete' and op.op_str == 'r8b' and update[i+1].mnemonic == 'add' and update[i+1].op_str == 'r8d, 0x352ae'
           for i, op in enumerate(update[:-1])), 'Native start217775/end217774 selection'
ticket = require('?Ticket@Actor@FuZanTempleIntro@@',
                 ('?Contact@Actor@FuZanTempleIntro@@', '?Receipt@FuZanTempleIntro@@'), (2000,))
assert any(op.mnemonic == 'movups' and op.op_str == 'xmmword ptr [rbx + 0x27c], xmm0' for op in ticket), 'Retain full128-bit issued cast GUID'
require('?Valid@Actor@FuZanTempleIntro@@', ('??8ObjectGuid@@', '?Receipt@FuZanTempleIntro@@', '?Contact@Actor@FuZanTempleIntro@@'))
create = require('?Create@spell_fuzan_temple_intro@@',
                 ('?PreventHitDefaultEffect@SpellScript@@', '?CreateConversation@Conversation@@',
                  '?GetCreature@Conversation@@', '?GetLowPart@ObjectGuid@@', '?GetHighPart@ObjectGuid@@',
                  '?GetPlayer@ObjectAccessor@@', '?BindQuestObject@Player@@'), (2770, 109353, 109413, 373394, 42765))
played = require('?Played@Story@FuZanTempleIntro@@', ('?GetUInt32Value@Object@@',), (156,))
assert {0x4e0, 0x4f0} <= fields(played), 'Actual native caster GUID and native playing map'
story = require('?Update@Story@FuZanTempleIntro@@',
                ('?GetCreature@ObjectAccessor@@', '?Owns@Actor@FuZanTempleIntro@@',
                 '?IsQuestObjectExpired@WorldObject@@', '?Played@Story@FuZanTempleIntro@@', '?Remove@Conversation@@'))
assert not sites(n, story, '?FindNearestCreature@WorldObject@@'), 'Full original source GUID survives walking beyond lookup radius'
require('?Owns@Actor@FuZanTempleIntro@@', ('??8ObjectGuid@@', '?Current@Actor@FuZanTempleIntro@@'))
guard = require('?Guard@spell_fuzan_temple_complete@@',
                ('??8ObjectGuid@@', '?Receipt@FuZanTempleIntro@@', '?Contact@Actor@FuZanTempleIntro@@',
                 '?PreventHitDefaultEffect@SpellScript@@'))
assert any(op.mnemonic == 'mov' and op.op_str == 'byte ptr [rbx + 0x2b9], 1' for op in guard), 'Reserve before native event/reentry'
assert 50.0 in floats(n, guard)
require('?Finished@spell_fuzan_temple_complete@@', ('?Receipt@FuZanTempleIntro@@', '?Cancel@Actor@FuZanTempleIntro@@'))
for name, handler, effect in (('intro', 'Create', 219), ('complete', 'Guard', 61)):
    require(f'?Check@spell_fuzan_temple_{name}@@', ('?Ticket@Actor@FuZanTempleIntro@@', '?Valid@Actor@FuZanTempleIntro@@'), (1625,))
    register = require(f'?Register@spell_fuzan_temple_{name}@@', constants=(effect,))
    for method in ('Check', handler, 'Finished'):
        assert n['symbol'](f'?{method}@spell_fuzan_temple_{name}@@') in references(register), (name, method)
require('?Validate@spell_fuzan_temple_intro@@', constants=(217775, 2770, 219, 6, 4))
require('?Validate@spell_fuzan_temple_complete@@', constants=(217774, 61, 52411))
registration = require('?AddSC_instance_temple_of_the_jade_serpent@@', constants=(1625,))
installed = installed_functions(registration)
for prefix in ('?GetAI@?$GenericCreatureScript@Unpc_fuzan_temple_intro@@',
               '?GetSpellScript@?$GenericSpellScriptLoader@Vspell_fuzan_temple_intro@@',
               '?GetSpellScript@?$GenericSpellScriptLoader@Vspell_fuzan_temple_complete@@'):
    assert n['symbol'](prefix) in installed, ('Actual registered vtable factory', prefix)
for body in (los, update, create, story, guard):
    for target in ('?UpdateAchievementCriteria@Player@@', '?UpdateAchievementCriteria@Scenario@@',
                   '?AddItem@Player@@', '?KilledMonsterCredit@Player@@', '?CompleteQuest@Player@@', '?SetQuestObjectiveData@Player@@'):
        assert not sites(n, body, target), target
print('PASS linked Fu Zan intro: actual three registrations, scoped source/admission, full cast/quest ownership, native2770 payload/player clock, finite recovery and guarded ordinary217774/native52411 receipt; no direct progression grant. Client playback remains separate.')
print(json.dumps({'exe_sha256': hashlib.sha256(n['binary']).hexdigest(), 'checked_functions': len(checked)}))

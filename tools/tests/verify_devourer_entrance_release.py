"""Actual registered final-Akaari entry controller, receipt and combat route."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values, floats
from merlis_intro_pe import reachable

p = runpy.run_path(str(Path(__file__).with_name('verify_nearest_player_release.py')))
n, code = p['n'], p['code']
scope = 'npc_akaari_shadowgore_lastAI@npc_akaari_shadowgore_last@@'
method = lambda name: '?' + name + '@' + scope
pairs = lambda body: {(x.mnemonic, x.op_str) for x in body}


def refs(body):
    return {x.address + x.size + operand.mem.disp for x in body for operand in x.operands
            if operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP}


with (n['args'].release / 'worldserver.map').open(errors='replace') as stream:
    tables = [int(m[2], 16) for line in stream
              if (m := re.match(r'\s+\w+:\w+\s+(\?\S+)\s+([0-9a-fA-F]{16})', line))
              and m[1].startswith('??_7npc_akaari_shadowgore_last@@6BScriptObject@@@')]
script, = tables
assert script in refs(code('?AddSC_Devourer@@'))
assert struct.unpack_from('<12Q', n['binary'], n['offset'](script))[10] == n['symbol']('?GetAI@npc_akaari_shadowgore_last@@')
factory = code('?GetAI@npc_akaari_shadowgore_last@@')
callbacks = {n['symbol'](method(x)) for x in ('UpdateAI', 'JustDied', 'Reset', 'EnterCombat')}
selected = [a for a in refs(factory) if callbacks <= set(struct.unpack_from('<82Q', n['binary'], n['offset'](a)))]
assert len(selected) == 1
assert {('mov', 'ecx, 0xa0'), ('mov', 'qword ptr [rsi + 0x80], rbp'),
        ('mov', 'dword ptr [rsi + 0x90], ebp')} <= pairs(factory)
eligible, recipient, entry, update = [code(method(x)) for x in ('EntryPlayer', 'EntryRecipient', 'UpdateEntry', 'UpdateAI')]
assert {105660, 1607, 1078, 41924, 53045, 47367} <= values(eligible)
assert {91.0, 25.0} <= floats(n, eligible)
for target in ('?IsInWorld@Object@@', '?IsPreDelete@Object@@', '?IsQuestObjectExpired@WorldObject@@',
               '?GetInstanceScript@WorldObject@@', '?CanContact@Player@@', '?InSamePhase@WorldObject@@',
               '?IsWithinDistInMap@WorldObject@@', '?CanTargetQuestObject@WorldObject@@',
               '?GetQuestStatus@Player@@', '?GetQuestRewardStatus@Player@@', '?CanUpdateCriteria@Scenario@@'):
    assert sites(n, eligible, target), target
lookup, = sites(n, recipient, '?GetPlayer@ObjectAccessor@@')
identity, = sites(n, recipient, '??8ObjectGuid@@')
qualify, = sites(n, recipient, method('EntryPlayer'))
receipt, = sites(n, recipient, '?GetQuestCancellationReceipt@Player@@')
assert lookup < identity < qualify < receipt
assert recipient[identity + 2].mnemonic == 'je'
assert recipient[receipt].address not in reachable(recipient, recipient[identity + 2].operands[0].imm)
assert {('movzx', 'eax, byte ptr [rax]'), ('movups', 'xmm0, xmmword ptr [rbx + 0x70]'),
        ('cmp', 'qword ptr [rax], rcx')} <= pairs(recipient)
cast, = sites(n, entry, '?CastSpell@Unit@@')
scenes = sites(n, entry, '?HasSceneStatus@Player@@')
credits = sites(n, entry, '?UpdateAchievementCriteria@Player@@')
receivers = sites(n, entry, method('EntryRecipient'))
assert len(scenes) == len(credits) == 2 and receivers[0] < cast < scenes[0] < scenes[1] < receivers[1] < credits[0] < credits[1]
assert {209626, 1254, 50404, 50299, 53045, 47367, 1000} <= values(entry)
assert {('mov', 'dword ptr [rsp + 0x70], 1'), ('mov', 'dword ptr [rsp + 0xc0], 3'),
        ('mov', 'byte ptr [r15 + 0x91], 1'), ('mov', 'byte ptr [r15 + 0x91], 0'),
        ('mov', 'dword ptr [rcx + 0x6c], 0x3e8')} <= pairs(entry)
assert sites(n, entry, '?GetPlayerListInGrid@WorldObject@@') and sites(n, entry, '?GetQuestCancellationReceipt@Player@@')
assert len(sites(n, entry, '?GetPlayer@ObjectAccessor@@')) == len(sites(n, entry, '??8ObjectGuid@@')) == 2
leaves = sites(n, entry, method('EntryLeaf'))
assert len(leaves) == 4 and leaves[-2] > credits[-1]
latch, = [x for x in entry if (x.mnemonic, x.op_str) == ('mov', 'byte ptr [rax + 0x40], 1')]
assert entry[leaves[-1]].address < latch.address
assert {('mov', 'byte ptr [r15 + 0x93], 1'), ('cmp', 'byte ptr [r15 + 0x92], 0')} <= pairs(entry)
start, = sites(n, update, method('UpdateEntry'))
combat, = sites(n, update, '?UpdateVictim@CreatureAI@@')
assert start < combat and {206866, 224990, 209233, 42000, 11000, 14000} <= values(update)
for target in ('?FindNearestPlayer@WorldObject@@', '?AddItem@Player@@', '?CompleteQuest@Player@@'):
    assert not sites(n, entry, target)
print('PASS registered final Akaari105660 entry: qualified actual Rogue/41924/1607/1078, original25-yard/Z91 and1s poll; fullGUID/explicit cancellation/current receipt revalidation, real native1254 launch status before visual latch, missing-only50404/50299 and both native leaves before one-shot, original combat tail/owner death/pickup separate. Client90s scene playback and whole chain unverified.')

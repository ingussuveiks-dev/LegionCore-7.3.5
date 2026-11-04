"""Registered Clay controller, both native request routes and exact live actor."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values
from merlis_intro_pe import reachable

p = runpy.run_path(str(Path(__file__).with_name('verify_kingslayers_intro_release.py')))
n, ins = p['native'], p['instructions']
suffix = 'npc_kingslayers_small_eventsAI@npc_kingslayers_small_events@@'
method = lambda name: '?' + name + '@' + suffix
pairs = lambda rows: {(x.mnemonic, x.op_str) for x in rows}


def refs(rows):
    return {op.address + op.size + v.mem.disp for op in rows for v in op.operands
            if v.type == CS_OP_MEM and v.mem.base == X86_REG_RIP}


def table(prefix):
    found = []
    with (n['args'].release / 'worldserver.map').open(errors='replace') as stream:
        for line in stream:
            row = re.match(r'\s+\w+:\w+\s+(\?\S+)\s+([0-9a-fA-F]{16})', line)
            if row and row[1].startswith(prefix):
                found.append(int(row[2], 16))
    assert len(found) == 1, (prefix, len(found))
    return found[0]


factory = ins('?GetAI@npc_kingslayers_small_events@@')
targets = {name: n['symbol'](method(name)) for name in ('MoveInLineOfSight', 'UpdateAI', 'SetGUID')}
tables = []
for address in refs(factory):
    raw = struct.unpack_from('<82Q', n['binary'], n['offset'](address))
    if all(value in raw for value in targets.values()):
        tables.append(raw)
assert len(tables) == 1, 'Actual factory must select all three controller callbacks'
assert ('mov', 'ecx, 0x60') in pairs(factory)
assert {('mov', 'dword ptr [rdi + 0x54], 0'), ('mov', 'word ptr [rdi + 0x58], 0'),
        ('mov', 'byte ptr [rdi + 0x38], 0')} <= pairs(factory)
for name, caller in [('UpdateAI', '?Update@Creature@@'), ('MoveInLineOfSight', '?MoveInLineOfSight_Safe@CreatureAI@@')]:
    slot, = [i for i, value in enumerate(tables[0]) if n['resolve'](value) == targets[name]]
    assert any(op.mnemonic in ('call', 'jmp') and op.operands[0].type == CS_OP_MEM and op.operands[0].mem.disp == slot * 8 for op in ins(caller))
guid_slot, = [i for i, value in enumerate(tables[0]) if n['resolve'](value) == targets['SetGUID']]
# Actual static script registration, not an unreferenced helper in the binary.
script = table('??_7npc_kingslayers_small_events@@6BScriptObject@@@')
assert script in refs(ins('?AddSC_Kingslayers@@'))
assert struct.unpack_from('<12Q', n['binary'], n['offset'](script))[10] == n['symbol']('?GetAI@npc_kingslayers_small_events@@')
assert {('mov', 'r8, qword ptr [rcx + 0x50]'), ('jmp', 'r8')} <= pairs(ins('?GetCreatureAI@ScriptMgr@@'))

eligible, pending, request, bridge, update, los = [ins(method(x)) for x in
    ('GatePlayer', 'GateRecipient', 'RequestGateOrder', 'SetGUID', 'UpdateAI', 'MoveInLineOfSight')]
assert {108057, 1620, 1123, 42504, 42627, 1, 3} <= values(eligible)
for target, count in [('?IsInWorld@Object@@', 2), ('?IsPreDelete@Object@@', 2),
                      ('?IsQuestObjectExpired@WorldObject@@', 2), ('?CanContact@Player@@', 1),
                      ('?InSamePhase@WorldObject@@', 1), ('?InSamePhaseId@WorldObject@@Q', 1),
                      ('?GetVisibilityRange@WorldObject@@', 1), ('?IsWithinDistInMap@WorldObject@@', 1),
                      ('?CanTargetQuestObject@WorldObject@@', 2), ('?GetQuestRewardStatus@Player@@', 1), ('?GetQuestStatus@Player@@', 2)]:
    assert len(sites(n, eligible, target)) == count, target
assert {('test', 'byte ptr [rbx + 0x4ff0], 1'), ('cmp', 'al, 4'),
        ('cmp', 'qword ptr [rbx + 8], rax'), ('cmp', 'byte ptr [rbx + 0x6848], 0'),
        ('cmp', 'byte ptr [rbx + 0x6849], 0'), ('cmp', 'byte ptr [rbx + 0x6851], 0')} <= pairs(eligible)
for body in (pending, bridge):
    lookup, = sites(n, body, '?GetPlayer@ObjectAccessor@@')
    identity, = sites(n, body, '??8ObjectGuid@@')
    qualified, = sites(n, body, method('GatePlayer') if body is pending else method('RequestGateOrder'))
    assert lookup < identity < qualified and body[identity + 2].mnemonic == 'je'
    assert body[qualified].address not in reachable(body, body[identity + 2].operands[0].imm)
assert {2384, 108057, 1620} <= values(bridge)
assert sites(n, request, method('GatePlayer'))[0] < sites(n, request, method('GateRecipient'))[0]
created, = sites(n, request, '?CreateConversation@Conversation@@')
assert 2384 in values(request[created - 8:created])
assert request[created + 1].op_str == 'al, al' and request[created + 2].mnemonic == 'jne'
latch, = [op for op in request if (op.mnemonic, op.op_str) == ('mov', 'byte ptr [rsi + 0x38], 1')]
assert latch.address == request[created + 2].operands[0].imm
assert latch.address not in reachable(request, request[created + 3].address)
assert {('cmp', 'byte ptr [rcx + 0x38], 0'), ('lea', 'rbx, [rcx + 0x58]'),
        ('cmp', 'dword ptr [rsi + 0x54], 0'), ('mov', 'dword ptr [rsi + 0x54], 0x3e8'),
        ('mov', 'byte ptr [rbx], 1'), ('mov', 'byte ptr [rbx], 0'), ('add', 'rsi, 0x59')} <= pairs(request)
# UpdateGateOrder is inlined by Release. Certify its actual counter, fresh
# recipient/cancellation path and retry call in the registered UpdateAI.
assert {108057, 1620} <= values(update)
assert {('cmp', 'byte ptr [rbx + 0x38], sil'), ('cmp', 'byte ptr [rbx + 0x58], sil'),
        ('mov', 'eax, dword ptr [rbx + 0x54]'), ('cmp', 'eax, edi'), ('sub', 'eax, edi'),
        ('mov', 'dword ptr [rbx + 0x54], eax'), ('mov', 'dword ptr [rbx + 0x54], esi')} <= pairs(update)
assert sites(n, update, method('GateRecipient'))[0] < sites(n, update, method('RequestGateOrder'))[0]
assert sites(n, update, '??$_Clear@ObjectGuid@@') or sites(n, update, '?Clear@ObjectGuid@@')
assert sites(n, los, method('RequestGateOrder')) and {108057, 1620} <= values(los)
opening = p['update']
nearest, = sites(n, opening, '?FindNearestCreature@WorldObject@@')
visibility, = sites(n, opening, '?GetVisibilityRange@WorldObject@@')
assert visibility < nearest and 108057 in values(opening[nearest - 10:nearest])
# Release loads SetGUID's actual virtual slot into RBX before fetching the
# Player GUID, then calls that saved function pointer with the same AI.
assert guid_slot * 8 == 0x80
load, = [i for i, op in enumerate(opening) if (op.mnemonic, op.op_str) == ('mov', 'rbx, qword ptr [rax + 0x80]')]
assert nearest < load
sent = [i for i, op in enumerate(opening[load:load + 15], load) if (op.mnemonic, op.op_str) == ('call', 'rbx')]
assert len(sent) == 1
at = sent[0]
assert {('mov', 'r8d, 0x950'), ('mov', 'rdx, rax'), ('mov', 'rcx, r14')} <= pairs(opening[load:at])
assert sites(n, opening[load:at], '?GetGuidValue@Object@@')


subtype = 'GateConversation@' + suffix
derived = table('??_7' + subtype + '6BWorldObject@@@')
base = table('??_7Conversation@@6BWorldObject@@@')
raw = struct.unpack_from('<50Q', n['binary'], n['offset'](base))
slot, = [i for i, value in enumerate(raw) if n['resolve'](value) == n['symbol']('?FindCreatureActor@Conversation@@')]
actual = struct.unpack_from('<' + str(slot + 1) + 'Q', n['binary'], n['offset'](derived))
assert n['resolve'](actual[slot]) == n['symbol']('?FindCreatureActor@' + subtype)
assert n['resolve'](actual[1]) == n['symbol']('?AddToWorld@' + subtype)
assert derived in refs(request) and ('movups', 'xmmword ptr [r12 + 0x508], xmm6') in pairs(request)
actor, admit = ins('?FindCreatureActor@' + subtype), ins('?AddToWorld@' + subtype)
for body in (actor, admit):
    assert {108057, 1620} <= values(body) and sites(n, body, '?GetCreature@ObjectAccessor@@')
    assert sites(n, body, '?IsInWorld@Object@@') and sites(n, body, '?IsPreDelete@Object@@') and sites(n, body, '?IsQuestObjectExpired@WorldObject@@')
assert sites(n, actor, '??9ObjectGuid@@') and sites(n, admit, '??8ObjectGuid@@')
assert not sites(n, actor, '?FindNearestCreature@WorldObject@@')
for target in ('?IsWithinDistInMap@WorldObject@@', '?InSamePhase@WorldObject@@', '?InSamePhaseId@WorldObject@@Q', '?CanTargetQuestObject@WorldObject@@'):
    assert sites(n, actor, target)
scope, = sites(n, admit, '?InheritQuestObjectScope@WorldObject@@')
publish, = sites(n, admit, '?AddToWorld@Conversation@@')
identity, = sites(n, admit, '??8ObjectGuid@@')
assert identity < scope < publish
for at in (identity, scope):
    assert admit[at + 2].mnemonic == 'je' and admit[publish].address not in reachable(admit, admit[at + 2].operands[0].imm)
for body in (request, eligible, pending, bridge):
    for target in ('?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@', '?CompleteQuest@Player@@', '?AddItem@Player@@'):
        assert not sites(n, body, target)
print('PASS registered Clay gate controller: both LOS/native GUID routes share admitted2384 receipt; real Rogue/quest/scenario/map/range/phases/scope and fullGUID admission, success-only one-shot,1s actor-owned qualified retry/cancellation/reentry, exact live source subtype/publication/scope; original native audience/payload and mandatory opening progression separate. Client playback/full chains unverified.')

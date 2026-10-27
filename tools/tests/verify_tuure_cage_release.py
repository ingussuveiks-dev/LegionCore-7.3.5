"""Verify the selected crate AI and the actual post-lock native dispatch."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP, X86_REG_EAX, X86_REG_R8D

p = runpy.run_path(str(Path(__file__).with_name('verify_tuure_first_route_release.py')))
n, instructions, values = p['n'], p['instructions'], p['values']
scope = 'go_tuure_crateAI@go_tuure_crate@@'
m = {name: '?' + name + '@' + scope for name in ('Current', 'Opened', 'Eligible', 'Actor', 'GossipHello', 'GossipUse', 'ReleaseEarned', 'TryOpening', 'UpdateAI', 'Reset')}
def references(method):
    for x in instructions(method):
        for op in x.operands:
            if op.type == CS_OP_MEM and op.mem.base == X86_REG_RIP:
                yield x.address + x.size + op.mem.disp
    if method == '?GetInstanceScript@instance_tuure@@':
        constructor = '??0instance_tuure_InstanceMapScript@instance_tuure@@'
        if any(s.startswith(constructor) for s in n['symbols']):
            assert n['symbol'](constructor) in n['calls'](method)
            yield from references(constructor)
tables = []
for address in references('?GetAI@go_tuure_crate@@'):
    table = struct.unpack_from('<24Q', n['binary'], n['offset'](address))
    if all(table[slot] == n['symbol'](m[name]) for slot, name in ((1, 'UpdateAI'), (4, 'Reset'), (8, 'GossipUse'), (9, 'GossipHello'))):
        tables.append(table)
assert len(tables) == 1, 'Actual factory does not select crate admission/use/lifecycle'
assert {1489, 249693, 134192, 1085, 10} <= values(m['Current'])
assert 47561 in values(m['Opened'])
assert {41993, 5} <= values(m['Eligible'])
assert {106134, 361419, 106253, 361433, 106107, 361436, 2102} <= values(m['ReleaseEarned'])
assert {50633, 5000} <= values(m['TryOpening'])
assert n['symbol']('?UpdateAchievementCriteria@Player@@') in n['calls'](m['TryOpening'])
assert n['symbol'](m['Opened']) in n['calls'](m['TryOpening'])
assert n['symbol'](m['TryOpening']) in n['calls'](m['UpdateAI'])
assert n['symbol'](m['ReleaseEarned']) in n['calls'](m['TryOpening'])
assert n['symbol']('?AddQuestDelayedEvent@Player@@') in n['calls'](m['GossipHello'])
assert n['symbol']('?Use@GameObject@@') in n['calls'](m['ReleaseEarned'])
assert n['symbol']('?GetAutoCloseTime@GameObjectTemplate@@') in n['calls'](m['ReleaseEarned'])
assert n['symbol']('?SendChat@CreatureTextMgr@@') in n['calls'](m['ReleaseEarned'])
assert n['symbol']('?Delete@GameObject@@') in n['calls'](m['UpdateAI'])
for name in ('GossipHello', 'TryOpening', 'ReleaseEarned'):
    assert n['symbol']('?Delete@GameObject@@') not in n['calls'](m[name]), 'Cage removed before native interval'
    for forbidden in ('?AddItem@Player@@', '?KilledMonsterCredit@Player@@', '?CompleteQuest@Player@@', '?SetHealth@Unit@@'):
        assert not {n['resolve'](a) for s, a in n['symbols'].items() if s.startswith(forbidden)} & n['calls'](m[name])
assert not any(s.startswith('?IsWithinLOSInMap@') and n['resolve'](a) in n['calls'](m['Eligible']) for s, a in n['symbols'].items()), 'Native requireLOS0 was tightened'
body = instructions('?EffectOpenLock@Spell@@')
can_open = n['symbol']('?CanOpenLock@Spell@@')
native = [i for i, x in enumerate(body) if x.mnemonic == 'call' and x.operands[0].type == CS_OP_IMM and n['resolve'](x.operands[0].imm) == can_open]
crate = [i for i, x in enumerate(body) if x.mnemonic == 'cmp' and any(op.type == CS_OP_IMM and op.imm == 249693 for op in x.operands)]
assert len(native) == len(crate) == 1 and native[0] < crate[0]
assert {3365, 1489, 134192, 1690} <= values('?EffectOpenLock@Spell@@')
# The instance's actual selected GUID return supplies the scoped actor lookups.
getter = '?GetGuidData@instance_tuure_InstanceMapScript@instance_tuure@@'
assert any(struct.unpack_from('<Q', n['binary'], n['offset'](address) + 18*8)[0] == n['symbol'](getter)
           for address in references('?GetInstanceScript@instance_tuure@@'))
body = instructions(getter)
assert any(x.mnemonic == 'mov' and x.operands[0].type == CS_OP_REG and x.operands[0].reg == X86_REG_EAX and x.operands[1].type == CS_OP_REG and x.operands[1].reg == X86_REG_R8D for x in body)
# MSVC lowers this sparse switch to cumulative subtractions, not three literal
# entry comparisons. Follow every equality branch to its full128-bit return.
base, branches = 0, {}
for i, x in enumerate(body[:-1]):
    if x.mnemonic in ('sub', 'cmp') and x.operands[0].type == CS_OP_REG and x.operands[0].reg == X86_REG_EAX and x.operands[1].type == CS_OP_IMM:
        if x.mnemonic == 'sub': base += x.operands[1].imm
        key = base if x.mnemonic == 'sub' else base + x.operands[1].imm
        assert body[i+1].mnemonic == 'je'
        branches[key] = body[i+1].operands[0].imm
assert set(branches) == {106107, 106134, 106137, 106253}
fields = set()
for target in branches.values():
    first = next(x for x in body if x.address == target)
    assert first.mnemonic == 'movups' and first.operands[1].type == CS_OP_MEM and first.operands[1].size == 16
    fields.add(first.operands[1].mem.disp)
assert len(fields) == 4
created = instructions('?OnCreatureCreate@instance_tuure_InstanceMapScript@instance_tuure@@')
assert fields <= {x.operands[0].mem.disp for x in created if x.mnemonic == 'movups' and x.operands[0].type == CS_OP_MEM}
print('PASS actual selected crate AI/factory and native post-CanOpenLock3365/1690/249693/134192 dispatch, exact1085/47561/41993 admission, actual fullGUID actor getter, native goober Use/auto-close and lifecycle-only Delete; no nearest stranger, forced quest/item/health or added requireLOS gate. Native lock/goober host owns failed-cast/retry/lease/recovery/3s cases; client pending.')

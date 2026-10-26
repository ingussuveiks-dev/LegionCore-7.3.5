"""Follow the selected Tuure instance and the real Kill/update virtual dispatch."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM, CS_OP_IMM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP

p = runpy.run_path(str(Path(__file__).with_name('verify_tuure_native_criteria_release.py')))
n, instructions = p['n'], p['instructions']
def values(method):
    return {op.imm for x in instructions(method) for op in x.operands if op.type == CS_OP_IMM}
def references(method):
    for x in instructions(method):
        for op in x.operands:
            if op.type == CS_OP_MEM and op.mem.base == X86_REG_RIP:
                yield x.address + x.size + op.mem.disp
scope = 'instance_tuure_InstanceMapScript@instance_tuure@@'
methods = {name: '?' + name + '@' + scope for name in
           ('OnCreatureCreate', 'CreatureDies', 'OnUnitDeath', 'Update', 'GetBoros', 'BeginUnlock')}
tables = []
for address in references('?GetInstanceScript@instance_tuure@@'):
    table = struct.unpack_from('<40Q', n['binary'], n['offset'](address))
    if all(table[slot] == n['symbol'](methods[name]) for name, slot in
           (('OnCreatureCreate', 3), ('CreatureDies', 5), ('OnUnitDeath', 15), ('Update', 29))):
        tables.append(table)
assert len(tables) == 1, 'Actual instance factory does not select repaired hooks'
assert {106201, 361428, 1085} <= values(methods['CreatureDies'])
assert {106134, 361419, 106221, 361429} <= values(methods['OnCreatureCreate'])
assert {5000, 50562} <= values(methods['Update'])
assert 16000 in values(methods['BeginUnlock'])
assert 1000 in values('?CancelFirstFight@' + scope)
assert 47519 in values('?HasVictory@TuureFirstFight@@')
for prefix in ('?Recipient@TuureFirstFight@@', '?AddQuestDelayedEvent@Player@@', '?getQuestStatus@Player@@'):
    assert n['symbol'](prefix) in n['calls'](methods['CreatureDies']), prefix
for prefix in ('?Eligible@TuureFirstFight@@', '?UpdateAchievementCriteria@Player@@',
               '?HasVictory@TuureFirstFight@@', methods['GetBoros'], methods['BeginUnlock']):
    assert n['symbol'](prefix) in n['calls'](methods['Update']), prefix
assert n['symbol']('?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@') in n['calls']('?Recipient@TuureFirstFight@@')
assert {106134, 106137, 106253} <= values('?Recipient@TuureFirstFight@@')
assert 2009 in values(methods['BeginUnlock'])
assert n['symbol'](methods['GetBoros']) in n['calls'](methods['BeginUnlock'])
for method, slot in (('?Kill@Unit@@', 0x28), ('?Update@InstanceMap@@', 0xe8)):
    body = instructions(method)
    direct = any(x.mnemonic == 'call' and x.operands[0].type == CS_OP_MEM and
                 x.operands[0].mem.disp == slot for x in body)
    loaded = any(x.mnemonic == 'mov' and len(x.operands) == 2 and
                 x.operands[0].type == CS_OP_REG and x.operands[1].type == CS_OP_MEM and
                 x.operands[1].mem.disp == slot and
                 any(y.mnemonic == 'call' and y.operands[0].type == CS_OP_REG and
                     y.operands[0].reg == x.operands[0].reg for y in body[i+1:i+5])
                 for i, x in enumerate(body))
    assert direct or loaded, method
evade = n['symbol']('?EnterEvadeMode@npc_vindicator_borosAI@npc_vindicator_boros@@')
raw = n['binary'][n['offset'](evade):n['offset'](evade)+3]
assert raw[:1] == b'\xc3' or raw == b'\xc2\x00\x00', 'Evade still grants first-fight victory'
for method in (methods['CreatureDies'], methods['Update']):
    for forbidden in ('?AddItem@Player@@', '?CompleteQuest@Player@@', '?SetHealth@Unit@@', '?CompleteScenario@'):
        assert not {n['resolve'](a) for name, a in n['symbols'].items() if name.startswith(forbidden)} & n['calls'](method)
print('PASS selected Tuure instance hooks: actual Kill slot0x28, InstanceMap Update slot0xe8, real106201/361428 death, native owner/quest lease, accepted47519/native50562 and1s/16s/5s continuation; empty evade override. Host/native catalog supply lifecycle and criteria metadata coverage; actual client pending.')

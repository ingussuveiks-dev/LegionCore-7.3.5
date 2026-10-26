"""Follow actual Tuure GO/AI and spell-loader factories in the linked Release binary."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

p = runpy.run_path(str(Path(__file__).with_name('verify_tuure_presentation_release.py')))
n, instructions = p['native'], p['instructions']


def values(method):
    return {op.imm for ins in instructions(method) for op in ins.operands if op.type == CS_OP_IMM}


def references(method):
    for ins in instructions(method):
        for op in ins.operands:
            if op.type == CS_OP_MEM and op.mem.base == X86_REG_RIP:
                yield ins.address + ins.size + op.mem.disp


gate = '?CanTake@TuureAcquisition@@'
hello, loot, factory = '?OnGossipHello@go_art_tuure@@', '?OnLootStateChanged@go_art_tuure@@', '?GetAI@go_art_tuure@@'
update, begin = '?UpdateAI@AI@go_art_tuure@@', '?SetGUID@AI@go_art_tuure@@'
ready, depart = '?Ready@State@TuureCast@@', '?Depart@State@TuureCast@@'
assert {1489, 249705, 41993, 1085, 6} <= values(gate)
assert 47656 in values(gate) or 47656 in values('?HasLeaf@TuureAcquisition@@')
assert {14000, 30000, 5000, 210818, 47679} <= values(update)
assert {41993, 128825, 210733} <= values(begin)
assert n['symbol']('?AddQuestDelayedEvent@Player@@') in n['calls'](begin)
assert n['symbol']('?getQuestStatus@Player@@') in n['calls'](begin)
assert n['symbol'](gate) in n['calls'](hello) and n['symbol'](gate) in n['calls'](loot)
# Keep the original ordinary (triggered=false) departure and its native cast checks.
cast=n['symbol']('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_NPEAVItem@@PEBVAuraEffect@@VObjectGuid@@')
body=instructions(update)
sites=[i for i,ins in enumerate(body) if ins.mnemonic=='call' and ins.operands[0].type==CS_OP_IMM and n['resolve'](ins.operands[0].imm)==cast]
assert len(sites)==1
r9=[ins for ins in body[max(0,sites[0]-14):sites[0]] if ins.mnemonic in ('mov','xor') and ins.op_str.split(',')[0] in ('r9','r9d','r9b')]
assert r9 and ((r9[-1].mnemonic=='xor' and r9[-1].op_str=='r9d, r9d') or (r9[-1].mnemonic=='mov' and r9[-1].operands[-1].type==CS_OP_IMM and r9[-1].operands[-1].imm==0)), 'Departure bypasses ordinary cast admission with triggered=true'

for method in (hello, loot, update, begin):
    for prefix in ('?AddItem@Player@@', '?UpdateAchievementCriteria@Player@@', '?AddDelayedEvent@Unit@@'):
        matches = {n['resolve'](address) for name, address in n['symbols'].items() if name.startswith(prefix)}
        assert not matches & n['calls'](method), f'Premature grant/credit/raw player callback remains in {method}'

tables = [address for address in references('?AddSC_Tuure@@')
          if struct.unpack_from('<Q', n['binary'], n['offset'](address) + 16)[0] == n['symbol'](hello)]
assert len(tables) == 1
table = n['offset'](tables[0])
assert struct.unpack_from('<Q', n['binary'], table + 0x50)[0] == n['symbol'](loot)
assert struct.unpack_from('<Q', n['binary'], table + 0x60)[0] == n['symbol'](factory)
selected = [address for address in references(factory)
            if struct.unpack_from('<Q', n['binary'], n['offset'](address) + 8)[0] == n['symbol'](update)]
assert len(selected) == 1
slots = struct.unpack_from('<32Q', n['binary'], n['offset'](selected[0]))
for method in (begin, '?GetGUID@AI@go_art_tuure@@', '?GetData@AI@go_art_tuure@@', '?SetData@AI@go_art_tuure@@'):
    assert n['symbol'](method) in slots, f'Pending claim AI does not select {method}'

for script in ('spell_tuure_pickup_cast', 'spell_tuure_departure'):
    getter = f'?GetSpellScript@?$GenericSpellScriptLoader@V{script}@@@@'
    register, load = f'?Register@{script}@@', f'?Load@{script}@@'
    assert any(struct.unpack_from('<Q', n['binary'], n['offset'](address) + 8)[0] == n['symbol'](getter)
               for address in references('?AddSC_Tuure@@')), f'No actual loader registration for {script}'
    assert any(n['symbol'](register) in struct.unpack_from('<16Q', n['binary'], n['offset'](address)) and
               n['symbol'](load) in struct.unpack_from('<16Q', n['binary'], n['offset'](address))
               for address in references(getter)), f'Loader misses derived {script} vtable'
    callbacks = set(references(register)) | values(register)
    for prefix in (f'?Check@{script}@@', f'?Hit@{script}@@'):
        assert n['symbol'](prefix) in callbacks, f'Actual hook list omits {prefix}'

assert {14000, 1265, 47679} <= values(ready)
assert {1220, 50702} <= values(depart)
assert n['symbol']('?EffectTeleportUnits@Spell@@') in n['calls'](depart), 'Departure bypasses actual native teleport effect'
assert n['symbol']('?UpdateAchievementCriteria@Player@@') in n['calls'](depart)
for prefix in ('?TeleportTo@Player@@', '?AddItem@Player@@'):
    matches = {n['resolve'](address) for name, address in n['symbols'].items() if name.startswith(prefix)}
    assert not matches & n['calls'](depart), 'Departure substitutes a forced destination/item grant'
assert n['symbol'](depart) in n['calls']('?Hit@spell_tuure_departure@@')
print('PASS actual Tuure script/AI and both registered spell factories/hooks: real completed loot, native210733/210734 lease guards,14s minimum/30s finite retry, real equip/50679, actual native teleport effect then50702; no activation grant, raw14s player callback or explicit1085 completion. Native full-flow/condition fixtures own failed/stale/deferred tests; client traversal pending.')

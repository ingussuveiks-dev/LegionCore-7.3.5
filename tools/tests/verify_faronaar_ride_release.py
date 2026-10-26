"""Verify selected Faronaar native-summon update, passenger AI and credit hook."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
p = runpy.run_path(str(Path(__file__).with_name('verify_faronaar_companion_release.py')))
n, instructions = p['n'], p['instructions']

def values(method):
    return {o.imm for i in instructions(method) for o in i.operands if o.type == CS_OP_IMM}

def references(method):
    for ins in instructions(method):
        for op in ins.operands:
            if ins.mnemonic == 'lea' and op.type == CS_OP_MEM and op.mem.base == X86_REG_RIP:
                yield ins.address + ins.size + op.mem.disp

update = '?UpdateRide@Faronaar@@'
passenger = '?PassengerBoarded@npc_faronaar_stellagosa_ride@@'
credit = '?Credit@spell_faronaar_stellagosa_ride@@'
register = '?Register@spell_faronaar_stellagosa_ride@@'
ai_factory = '?GetAI@?$GenericCreatureScript@Unpc_faronaar_stellagosa_ride@@@@'
spell_factory = '?GetSpellScript@?$GenericSpellScriptLoader@Vspell_faronaar_stellagosa_ride@@@@'
assert n['symbol'](update) in n['calls']('?OnUpdate@player_faronaar_chain@@')
assert {5000, 178923, 180768, 90982, 300000} <= values(update)
assert n['symbol']('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@MMMI_N') in n['calls'](update)
assert not ({address for name,address in n['symbols'].items() if name.startswith('?SummonCreature@WorldObject@@')} & n['calls'](update)), 'Raw untracked summon retained'
assert n['symbol']('?PassengerBoarded@SmartAI@@') in n['calls'](passenger)
assert n['symbol']('?ExitVehicle@Unit@@') in n['calls'](passenger)
assert n['symbol']('?RideEligible@Faronaar@@') in n['calls'](passenger)
assert 90982 in values(passenger) and 90982 in values(credit)
assert n['symbol']('?PreventHitDefaultEffect@SpellScript@@') in n['calls'](credit)
assert n['symbol'](credit) in references(register) and {2, 90} <= values(register)
for method in (update, credit, passenger):
    for forbidden in ('?KilledMonsterCredit@Player@@', '?QuestObjectiveSatisfy@Player@@', '?CompleteQuest@Player@@'):
        assert n['symbol'](forbidden) not in n['calls'](method), 'Must retain native boarding event credit'

mapping=(n['args'].release/'worldserver.map').read_text(errors='replace')
def table(name):
    match=re.search(r'\s[0-9a-f]+:[0-9a-f]+\s+'+re.escape(name)+r'\s+([0-9a-fA-F]{16})',mapping)
    assert match, name
    return int(match[1],16)

ai=table('??_7npc_faronaar_stellagosa_ride@@6B@')
assert ai in references(ai_factory)
slots=struct.unpack_from('<80Q',n['binary'],n['offset'](ai))
assert n['symbol'](passenger) in slots and n['symbol']('?UpdateAI@SmartAI@@') in slots
spell=table('??_7spell_faronaar_stellagosa_ride@@6B@')
assert spell in references(spell_factory)
assert n['symbol'](register) in struct.unpack_from('<12Q',n['binary'],n['offset'](spell))
registered=set(references('?AddSC_faronaar_chain@@'))
assert table('??_7?$GenericCreatureScript@Unpc_faronaar_stellagosa_ride@@@@6BScriptObject@@@') in registered
assert table('??_7?$GenericSpellScriptLoader@Vspell_faronaar_stellagosa_ride@@@@6B@') in registered
print('PASS selected Faronaar update uses native178923 and bounded same-actor180768 retry; registered derived SmartAI filters real owner boarding, inherited route update retained; actual effect2/90 hook defers early credit. Client flight/camera remains pending.')

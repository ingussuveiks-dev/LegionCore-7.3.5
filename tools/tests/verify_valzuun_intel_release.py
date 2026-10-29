"""Follow the registered derived SmartAI and actual delayed-credit code in PE."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

p = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
n, ins = p['native'], p['instructions']

def refs(method):
    return {x.address + x.size + o.mem.disp for x in ins(method) for o in x.operands
            if o.type == CS_OP_MEM and o.mem.base == X86_REG_RIP}

def table(address, count):
    return struct.unpack_from('<' + str(count) + 'Q', n['binary'], n['offset'](address))

def needs(method, *dependencies):
    for dependency in dependencies:
        assert n['symbol'](dependency) in n['calls'](method), (method, dependency)

def values(method):
    return {o.imm for x in ins(method) for o in x.operands if o.type == CS_OP_IMM}

factory = '?GetAI@?$GenericCreatureScript@Unpc_valzuun_intel@@'
select = '?sGossipSelect@npc_valzuun_intel@@'
update = '?UpdateAI@npc_valzuun_intel@@'
reset = '?Reset@npc_valzuun_intel@@'
current = '?Current@ValzuunIntel@@'
eligible = '?Eligible@ValzuunIntel@@'
text = '?TextExist@CreatureTextMgr@@'
talk = '?Talk@CreatureAI@@QEAAXE'
credit = '?RewardPlayerAndGroupAtEvent@Player@@'
mapping = (n['args'].release / 'worldserver.map').read_text(errors='replace')
vtable = re.search(r'\?\?_7npc_valzuun_intel@@6B@\s+([0-9a-fA-F]{16})', mapping)
assert vtable, 'Missing linked actor vtable'
address = int(vtable.group(1), 16)
assert address in refs(factory), 'Registered factory does not construct the repaired actor'
slots = table(address, 128)
assert all(n['symbol'](m) in slots for m in (select, update, reset)), 'Actual vtable lacks a repaired callback'
assert any(n['symbol'](factory) in table(a, 16) for a in refs('?AddSC_artifact_acquisition@@')), 'Factory not registered'
needs(select, eligible, current, text, '?getQuestStatus@Player@@', '?sGossipSelect@SmartAI@@', '?SendCloseGossip@PlayerMenu@@', talk)
needs(update, '?UpdateAI@SmartAI@@', current, text, '?GetPlayer@ObjectAccessor@@', talk, credit)
needs(current, eligible, '?getQuestStatus@Player@@')
needs(reset, '?Reset@SmartAI@@')
assert {105333,41919} <= values(eligible)
assert {19472,5000,90000,4} <= values(select)
assert {105333,10000,8000,2000,4} <= values(update)
assert n['symbol'](credit) not in n['calls'](select), 'Selection still grants early group credit'
for method in (select, update):
    for forbidden in ('?CompleteQuest@Player@@', '?AddItem@Player@@', '?KilledMonsterCredit@Player@@', '?TeleportTo@Player@@'):
        assert not {n['resolve'](a) for s, a in n['symbols'].items() if s.startswith(forbidden)} & n['calls'](method), (method, forbidden)
print('PASS registered Valzuun factory/actual vtable and native callbacks: own quest/life gate, four text groups,5s/10s/8s/2s finite controller, credit only in Update via native group eligibility, preserved SmartAI delegation/reset and no direct item/quest completion. Host regressions cover lifecycle; client/public audio timing unverified.')

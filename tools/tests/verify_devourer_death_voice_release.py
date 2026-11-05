"""Real final Akaari death factory installs its exact corpse actor subtype."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP
from sharasdal_erunak_entry_pe import sites, values
from merlis_intro_pe import reachable

previous = runpy.run_path(str(Path(__file__).with_name('verify_devourer_entrance_release.py')))
n = previous['n']
ins = previous['code']
suffix = 'DeathConversation@npc_akaari_shadowgore_lastAI@npc_akaari_shadowgore_last@@'
text = (n['args'].release / 'worldserver.map').read_text(errors='replace')

def table(prefix):
    rows = re.findall(r'\s+\w+:\w+\s+' + re.escape(prefix) + r'\s+([0-9a-fA-F]{16})', text)
    assert len(rows) == 1, prefix
    return int(rows[0], 16)

def refs(body):
    return {op.address + op.size + v.mem.disp for op in body for v in op.operands
            if v.type == CS_OP_MEM and v.mem.base == X86_REG_RIP}

pairs = lambda body: {(op.mnemonic, op.op_str) for op in body}
base = table('??_7Conversation@@6BWorldObject@@@')
derived = table('??_7' + suffix + '6BWorldObject@@@')
raw = struct.unpack_from('<50Q', n['binary'], n['offset'](base))
slot, = [i for i, pointer in enumerate(raw) if n['resolve'](pointer) == n['symbol']('?FindCreatureActor@Conversation@@')]
actual = struct.unpack_from('<' + str(slot + 1) + 'Q', n['binary'], n['offset'](derived))
assert n['resolve'](actual[slot]) == n['symbol']('?FindCreatureActor@' + suffix)
assert n['resolve'](actual[1]) == n['symbol']('?AddToWorld@' + suffix)
for i in range(2, slot):
    assert n['resolve'](actual[i]) == n['resolve'](raw[i]), ('Other Conversation slot changed', i)
assert any(op.mnemonic == 'call' and op.operands[0].type == CS_OP_MEM and op.operands[0].mem.disp == slot * 8
           for op in ins('?GenerateDynamicFieldData@Conversation@@'))
default = ins('?FindCreatureActor@Conversation@@')
assert ('mov', 'r9b, 1') in pairs(default) and sites(n, default, '?FindNearestCreature@WorldObject@@')
hook = ins('?JustDied@npc_akaari_shadowgore_lastAI@npc_akaari_shadowgore_last@@')
assert derived in refs(hook)
captures = {op.op_str.split(',')[0] for op in hook if op.mnemonic == 'movups' and op.op_str.endswith(', xmmword ptr [rax]')}
assert captures and any(op.mnemonic == 'movups' and '+ 0x508]' in op.op_str and op.op_str.split(', ')[-1] in captures for op in hook), 'Full original source GUID must reach derived storage'
assert any(op.mnemonic == 'call' and op.operands[0].type == CS_OP_MEM and op.operands[0].mem.disp == 0x130 for op in ins('?Kill@Unit@@'))
actor = ins('?FindCreatureActor@' + suffix)
assert {105660, 1607} <= values(actor)
for prefix, count in [('?GetCreature@ObjectAccessor@@', 1), ('??9ObjectGuid@@', 1), ('?IsInWorld@Object@@', 1),
                      ('?IsPreDelete@Object@@', 1), ('?IsQuestObjectExpired@WorldObject@@', 2),
                      ('?IsWithinDistInMap@WorldObject@@', 1), ('?InSamePhase@WorldObject@@', 1),
                      ('?InSamePhaseId@WorldObject@@Q', 1), ('?CanTargetQuestObject@WorldObject@@', 2)]:
    assert len(sites(n, actor, prefix)) == count, prefix
assert ('test', 'edx, edx') in pairs(actor) and not sites(n, actor, '?FindNearestCreature@WorldObject@@')
identity, = sites(n, actor, '??9ObjectGuid@@')
assert actor[identity + 1].op_str == 'al, al' and actor[identity + 2].mnemonic == 'jne'
success, = [op.address for op in actor if (op.mnemonic, op.op_str) == ('mov', 'rax, rbx')]
assert success not in reachable(actor, actor[identity + 2].operands[0].imm)
admit = ins('?AddToWorld@' + suffix)
lookup, = sites(n, admit, '?GetCreature@ObjectAccessor@@')
identity, = sites(n, admit, '??8ObjectGuid@@')
scope, = sites(n, admit, '?InheritQuestObjectScope@WorldObject@@')
publish, = sites(n, admit, '?AddToWorld@Conversation@@')
assert lookup < identity < scope < publish and 105660 in values(admit)
for at in (identity, scope):
    assert admit[at + 1].op_str == 'al, al' and admit[at + 2].mnemonic == 'je'
    assert admit[publish].address not in reachable(admit, admit[at + 2].operands[0].imm)
print('PASS actual registered Akaari death1947/Unit.Kill: installs scoped corpse subtype and stores full128-bit source GUID; native actor virtual dispatch, explicit full-GUID validation before actor return and publication, original slot/entry/map/world/lifetime/range/phases/reciprocal scope, actual source-scope inheritance before AddToWorld. Ordinary actor lookup remains alive-only; owner50300/entrance/combat/pickup retained, client playback separate.')

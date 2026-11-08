"""Select the registered wave factory and real point-cleanup callback."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

p = runpy.run_path(str(Path(__file__).with_name('verify_sharasdal_graveyard_release.py')))
n, ins = p['n'], p['ins']
method = '?MovementInform@npc_sharasdal_waterwallAI@npc_sharasdal_waterwall@@'
factory_name = '?GetAI@npc_sharasdal_waterwall@@'
body, factory = ins(method), ins(factory_name)

def sites(prefix):
    targets = {n['resolve'](v) for k, v in n['symbols'].items() if k.startswith(prefix)}
    return [i for i, x in enumerate(body) if x.mnemonic == 'call' and x.operands[0].type == CS_OP_IMM
            and n['resolve'](x.operands[0].imm) in targets]

def refs(rows):
    return [x.address+x.size+o.mem.disp for x in rows if x.mnemonic == 'lea'
            for o in x.operands if o.type == CS_OP_MEM and o.mem.base == X86_REG_RIP]

remove, = sites('?RemoveAllPassengers@Vehicle@@')
despawn, = sites('?DespawnOrUnsummon@Creature@@')
# The optional vehicle branch must join exactly the post-ejection path, not return.
assert body[remove-1].mnemonic == 'je' and body[remove-1].operands[0].imm == body[remove+1].address, 'Release lacks kit-independent Sharasdal wave cleanup'
assert body[0].op_str == 'edx, 8' and body[0].mnemonic == 'cmp'
assert body[1].mnemonic == 'jne' and body[1].operands[0].imm == body[-1].address and body[-1].mnemonic == 'ret'
assert [(x.mnemonic, x.op_str) for x in body[remove-3:remove]] == [
    ('mov', 'rcx, qword ptr [rax + 0x30e0]'), ('test', 'rcx, rcx'), ('je', hex(body[remove+1].address))]
assert [(x.mnemonic, x.op_str) for x in body[remove+1:despawn]] == [
    ('mov', 'rcx, qword ptr [rbx + 0x20]'), ('lea', 'r8, [rsp + 0x48]'),
    ('mov', 'edx, 0x64'), ('mov', 'qword ptr [rsp + 0x48], 0')]
assert remove < despawn and len([x for x in body if x.mnemonic == 'call']) == 2
tables = [at for at in refs(factory) if n['symbol'](method) in struct.unpack_from('<82Q', n['binary'], n['offset'](at))]
table, = tables
install, = [i for i, x in enumerate(factory) if x.mnemonic == 'lea' and any(
    o.type == CS_OP_MEM and o.mem.base == X86_REG_RIP and x.address+x.size+o.mem.disp == table for o in x.operands)]
assert factory[install+1].op_str == 'qword ptr [rdi], rax'
assert [('xor', 'r8d, r8d'), ('xor', 'edx, edx'), ('mov', 'rcx, qword ptr [rdi + 0x20]')] == [(x.mnemonic, x.op_str) for x in factory[install+2:install+5]]
pattern = re.compile(r'^\s+\w+:\w+\s+\?\?_7npc_sharasdal_waterwall@@6BScriptObject@@@\s+([0-9a-fA-F]{16})\s', re.M)
descriptors = [int(v, 16) for v in pattern.findall((n['args'].release/'worldserver.map').read_text(errors='replace'))]
descriptor, = descriptors
# One ScriptObject destructor plus the ten CreatureScript methods put GetAI at10.
assert struct.unpack_from('<11Q', n['binary'], n['offset'](descriptor))[10] == n['symbol'](factory_name)
assert descriptor in refs(ins('?AddSC_Sharasdal@@')), 'Wave script descriptor not registered'
rows = struct.unpack_from('<82Q', n['binary'], n['offset'](table))
slot, = [i*8 for i, value in enumerate(rows) if value == n['symbol'](method)]
native_point = ins('?DoFinalize@?$PointMovementGenerator@VCreature@@@@')
assert any(o.type == CS_OP_IMM and o.imm == 8 for x in native_point for o in x.operands)
assert any(o.type == CS_OP_MEM and o.mem.disp == slot for x in native_point for o in x.operands)
for prefix in ('?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@', '?CompleteQuest@Player@@', '?AddItem@Player@@', '?TeleportTo@Player@@'):
    assert not sites(prefix), prefix
print('PASS linked Sharasdal wave:registered script descriptor/GetAI/passive actual AI vtable/MovementInform;POINT8 only,optional native vehicle ejection joins unconditional original100ms/zero-respawn cleanup;no progression,item or teleport calls. Retained native acquisition/ascent/gauntlet/death/cemetery chain;physical travel/contact/client separate.')

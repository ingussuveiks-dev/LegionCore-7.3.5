"""Check actual map teardown releases active spawn locks before grid deletion."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native, instructions = previous['native'], previous['instructions']
add_name = '?AddToActive@Map@@QEAAXPEAVCreature'
remove_name = '?RemoveFromActive@Map@@QEAAXPEAVCreature'
unload_name = '?UnloadAll@Map@@'


def sites(body, prefix):
    target = native['symbol'](prefix)
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == target]


body = instructions(unload_name)
remove, = sites(body, remove_name)
deactivate, = sites(body, '?setActive@WorldObject@@')
grid, = sites(body, '?UnloadGrid@Map@@')
assert remove < deactivate < grid
assert body[deactivate - 2].op_str == 'edx, edx' and body[deactivate - 2].mnemonic == 'xor'
assert body[deactivate + 1].op_str == 'qword ptr [rdi + 0x7a8], 0'
assert body[deactivate + 2].mnemonic == 'jne' and body[deactivate + 2].operands[0].imm < body[remove].address
assert any(entry.op_str == 'byte ptr [rbx + 0x16], 3' and entry.mnemonic == 'cmp' for entry in body[remove - 5:remove])
assert body[grid - 2].op_str == 'r8b, 1', 'Full teardown must still force ordinary grid cleanup'
assert sites(body, '?Update@FunctionProcessor@@')[0] < remove
assert sites(body, '?LogoutPlayer@WorldSession@@')[0] < remove
assert sites(body, '?wait@ThreadPoolMap@@')[0] > grid
for caller in ('?UnloadAll@InstanceMap@@', '?UnloadAll@MapInstanced@@'):
    assert native['symbol'](unload_name) in native['calls'](caller)
for name, compare, branch, lock, action in ((add_name, 'rdx, r8', 'jne', '?incUnloadActiveLock@GridInfo@@', b'added'),
                                          (remove_name, 'rdx, r9', 'je', '?decUnloadActiveLock@GridInfo@@', b'removed')):
    selected = instructions(name)
    guard = next(index for index, entry in enumerate(selected) if entry.mnemonic == 'cmp' and entry.op_str == compare)
    position, = sites(selected, lock)
    respawn, = sites(selected, '?GetRespawnPosition@Creature@@')
    assert guard < respawn < position and selected[guard + 1].mnemonic == branch
    assert selected[guard + 1].operands[0].imm > selected[position].address
    diagnostic = b'Active creature (GUID: %u Entry: %u) ' + action + b' to grid' if action == b'added' else b'Active creature (GUID: %u Entry: %u) removed from grid'
    assert any(entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP
               and native['binary'][native['offset'](entry.address + entry.size + entry.operands[1].mem.disp):].startswith(diagnostic)
               for entry in selected), 'Genuine missing-grid error must remain in the selected handler'
assert native['symbol'](remove_name) in native['calls']('?setActive@WorldObject@@')
print('PASS supplied Release actual InstanceMap/MapInstanced -> Map::UnloadAll: active creature lock release and setActive(false) drain precede forced UnloadGrid; deferred/session/pool order retained. Native add/remove membership guards precede respawn lookup/inc/dec, missing-grid diagnostics retained. Not a live Grif/client path or every map lifecycle certification.')

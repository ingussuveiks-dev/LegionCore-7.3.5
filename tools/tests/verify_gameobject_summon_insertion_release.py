"""Verify both actual insertion-result branches and canonical cleanup vtables."""
from pathlib import Path
import re
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM

previous = runpy.run_path(str(Path(__file__).with_name('verify_smart_fixed_summon_release.py')))
native = previous['native']
instructions = previous['instructions']
admission = native['symbol']('??$AddToMap@VGameObject@@@Map@@')
cleanup_slot = 0xb8
for prefix in ('?SummonGameObject@WorldObject@@QEAAPEAVGameObject@@IM', '?SummonGameObject@Map@@QEAAPEAVGameObject@@IM'):
    body = instructions(prefix)
    by_address = {entry.address: index for index, entry in enumerate(body)}
    calls = [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == admission]
    assert len(calls) == 1, 'Summon path no longer invokes actual Map::AddToMap exactly once'
    position = calls[0]
    assert body[position + 1].mnemonic == 'test' and body[position + 1].op_str == 'al, al', 'Insertion result ignored'
    branch = body[position + 2]
    assert branch.mnemonic == 'jne' and branch.operands[0].type == CS_OP_IMM, 'Successful insertion does not bypass rollback'
    success = branch.operands[0].imm
    success_entry = body[by_address[success]]
    assert success_entry.mnemonic == 'mov' and success_entry.op_str.startswith('rax, '), 'Successful insertion no longer returns the created object'
    object_register = success_entry.op_str.split(', ')[1]
    failure = [entry for entry in body[position + 3:] if entry.address < success]
    cleanups = [index for index, entry in enumerate(failure) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == cleanup_slot]
    assert len(cleanups) == 1, 'Failure omits canonical virtual cleanup'
    cleanup_index = cleanups[0]
    assert failure[cleanup_index - 1].op_str == f'rcx, {object_register}', 'Cleanup does not receive the created object'
    tail = failure[cleanup_index + 1:]
    deletion = next(index for index, entry in enumerate(tail) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0)
    assert tail[deletion - 1].op_str == f'rcx, {object_register}' and tail[deletion - 2].op_str == 'edx, 1', 'Rejected allocation not deleted through its actual destructor'
    returned = tail[deletion + 1]
    if returned.mnemonic == 'jmp':
        returned = body[by_address[returned.operands[0].imm]]
    assert returned.mnemonic == 'xor' and returned.op_str == 'eax, eax', 'Failure returns a nonnull pointer'

mapping = (native['args'].release / 'worldserver.map').read_text(errors='replace')
for name, expected in (('??_7GameObject@@6BWorldObject@@@', '?CleanupsBeforeDelete@GameObject@@'), ('??_7StaticTransport@@6BWorldObject@@@', '?CleanupsBeforeDelete@Transport@@')):
    addresses = re.findall(r'\s' + re.escape(name) + r'\s+([0-9a-fA-F]{16})\s', mapping)
    assert len(addresses) == 1, f'Missing or ambiguous actual cleanup vtable: {name}'
    offset = native['offset'](int(addresses[0], 16))
    assert struct.unpack_from('<Q', native['binary'], offset + cleanup_slot)[0] == native['symbol'](expected), 'Allocated type dispatches the wrong cleanup policy'


def dependencies(prefix):
    return {native['resolve'](entry.operands[0].imm) for entry in instructions(prefix) if entry.mnemonic in {'call', 'jmp'} and entry.operands and entry.operands[0].type == CS_OP_IMM}


assert native['symbol']('?CleanupsBeforeDelete@WorldObject@@') in dependencies('?CleanupsBeforeDelete@GameObject@@'), 'Cleanup loses phase/transport teardown'
assert native['symbol']('?RemoveFromOwner@GameObject@@') in dependencies('?CleanupsBeforeDelete@GameObject@@'), 'Cleanup loses owner teardown'
assert native['symbol']('?RemoveGameObject@Unit@@QEAAXPEAVGameObject@@') in dependencies('?RemoveFromOwner@GameObject@@'), 'Owner list removal missing'
assert native['symbol']('?RemovePassenger@Transport@@') in dependencies('?CleanupsBeforeDelete@WorldObject@@'), 'Transport passenger removal missing'
assert native['symbol']('?CleanupsBeforeDelete@GameObject@@') in dependencies('?CleanupsBeforeDelete@Transport@@'), 'Static transport cleanup omits the GameObject base'
transport_cleanup = dependencies('?CleanupsBeforeDelete@Transport@@')
assert native['symbol']('?UnloadStaticPassengers@Transport@@') in transport_cleanup or native['symbol']('?AddObjectToRemoveList@WorldObject@@') in transport_cleanup, 'Neither native static-passenger unload nor its inlined removal dispatch is present'
assert native['symbol']('?RemovePassenger@Transport@@') in transport_cleanup, 'Dynamic passenger cleanup missing'
assert b'WorldObject::SummonGameObject: failed to add gameobject' in native['binary'] and b'Map::SummonGameObject: failed to add gameobject' in native['binary']
print('PASS: actual canonical WorldObject/Map admission bool selects rollback ->native GameObject/StaticTransport cleanup ->destruction ->null, success returns object; native owner-list/phase/transport cleanup dependencies and SmartAI null-result retry inspected')

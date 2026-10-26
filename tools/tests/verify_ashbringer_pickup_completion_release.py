"""Verify selected Ashbringer capacity and reported-cast failure admission."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_ashbringer_balnazzar_surrender_release.py')))
native, instructions = previous['native'], previous['instructions']
callback = '?OnLootStateChanged@go_bs_the_ashbringer@@'
body = instructions(callback)


def sites(prefix):
    targets = {address for name, address in native['symbols'].items() if name.startswith(prefix)}
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) in targets]


def reaches(start, destination):
    positions = {entry.address: index for index, entry in enumerate(body)}
    pending, visited = [start], set()
    while pending:
        address = pending.pop()
        if address == destination:
            return True
        if address in visited or address not in positions:
            continue
        visited.add(address)
        entry = body[positions[address]]
        if entry.mnemonic in ('ret', 'int3'):
            continue
        if entry.mnemonic.startswith('j'):
            pending.append(entry.operands[0].imm)
            if entry.mnemonic == 'jmp':
                continue
        pending.append(entry.address + entry.size)
    return False


credit, = sites('?DoUpdateAchievementCriteria@InstanceScript@@')
cast, = sites('?CastSpell@Unit@@QEAA?AW4SpellCastResult@@PEAV1@I_N')
assert all(any(entry.op_str == operand for entry in body[credit - 7:credit]) for operand in ('edx, 0x5c', 'r8d, 0xb432', 'r9d, r9d'))
assert body[0].op_str == 'r9, r9' and body[1].mnemonic == 'je' and body[1].operands[0].imm == body[-1].address
assert any(entry.op_str == 'byte ptr [r9 + 0x16], 4' for entry in body[:credit])
instance, = sites('?GetInstanceScript@WorldObject@@')
stage, = sites('?getScenarionStep@InstanceScript@@')
owned = sites('?HasItemCount@Player@@')
assert len(owned) == 2 and instance < stage < owned[0] < owned[1] < credit
assert body[instance + 2].op_str == 'rax, rax' and body[instance + 3].mnemonic == 'je'
assert body[stage + 1].op_str == 'eax, 3' and body[stage + 2].mnemonic == 'jne'
rejected = body[stage + 2].operands[0].imm
assert rejected > body[credit].address and body[instance + 3].operands[0].imm == rejected
assert body[stage + 3].op_str == 'r14d, eax' and body[stage + 4].mnemonic == 'je'
assert body[stage + 4].operands[0].imm == body[owned[0] + 3].address
assert body[owned[0] + 1].op_str == 'al, al' and body[owned[0] + 2].mnemonic == 'je' and body[owned[0] + 2].operands[0].imm == rejected
for position in owned:
    assert all(any(entry.op_str == operand for entry in body[position - 6:position]) for operand in ('edx, 0x1d892', 'r8d, 1', 'r9d, r9d'))
assert body[owned[1] + 1].op_str == 'al, al' and body[owned[1] + 2].mnemonic == 'jne' and body[owned[1] + 2].operands[0].imm == body[cast - 10].address
capacity, = sites('?CanStoreItem@Player@@')
error, = sites('?SendEquipError@Player@@')
reopens = sites('?SetLootState@GameObject@@')
assert len(reopens) == 2
reopen, cast_reopen = reopens
assert owned[1] < capacity < error < reopen < cast < cast_reopen < credit
assert all(any(entry.op_str == operand for entry in body[capacity - 12:capacity]) for operand in ('edx, edx', 'r8b, 0xff', 'r9, [rsp + 0x60]', 'dword ptr [rsp + 0x20], 0x1d892', 'dword ptr [rsp + 0x28], 1'))
assert body[capacity + 1].op_str == 'al, al' and body[capacity + 2].mnemonic == 'je'
assert any(entry.op_str == 'edx, al' and entry.mnemonic == 'movzx' for entry in body[capacity + 3:error])
assert any(entry.op_str == 'dword ptr [rsp + 0x20], 0x1d892' for entry in body[capacity + 3:error])
for position in reopens:
    assert all(any(entry.op_str == operand for entry in body[position - 4:position]) for operand in ('r8d, r8d', 'edx, 1', 'rcx, rdi'))
assert not reaches(body[capacity + 3].address, body[credit].address)
assert reaches(body[capacity + 2].operands[0].imm, body[credit].address)
nearest, = sites('?FindNearestCreature@WorldObject@@')
despawn, = sites('?DespawnOrUnsummon@Creature@@')
delete, = sites('?Delete@GameObject@@')
assert cast < credit < nearest < despawn < delete
assert any(entry.op_str == 'edx, 0x1941e' for entry in body[nearest - 4:nearest])
distance, = [entry for entry in body[nearest - 5:nearest] if entry.mnemonic == 'movss']
assert distance.mnemonic == 'movss' and distance.operands[1].mem.base == X86_REG_RIP
assert struct.unpack_from('<f', native['binary'], native['offset'](distance.address + distance.size + distance.operands[1].mem.disp))[0] == 10.0
assert body[nearest + 1].op_str == 'rax, rax' and body[nearest + 2].mnemonic == 'je' and body[nearest + 2].operands[0].imm == body[despawn + 1].address
assert all(any(entry.op_str == operand for entry in body[cast - 11:cast]) for operand in ('r8d, 0x2c272', 'r9b, 1', 'rdx, rbx', 'rcx, rbx'))
assert body[cast + 1].op_str == 'r15d, eax' and body[cast + 2].op_str == 'eax, eax'
assert body[cast + 3].mnemonic == 'je' and body[cast + 3].operands[0].imm == body[credit - 6].address
failed = body[cast + 4].address
assert not any(reaches(failed, body[position].address) for position in (credit, nearest, despawn, delete))
assert reaches(failed, body[cast_reopen].address)
diagnostic = b'Ashbringer pickup: retrieval 180850 failed with result %u for player %u; retain object for retry'
assert any(entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP
           and native['binary'][native['offset'](entry.address + entry.size + entry.operands[1].mem.disp):].startswith(diagnostic)
           for entry in body[cast:credit])
assert any(entry.op_str == 'dword ptr [rsp + 0x50], r15d' for entry in body[cast:credit])
activated = next(index for index, entry in enumerate(body[despawn:], despawn) if entry.op_str == 'r14d, 2' and entry.mnemonic == 'cmp')
assert body[activated + 1].mnemonic == 'jne' and body[activated + 1].operands[0].imm == rejected

tables = []
for entry in instructions('?AddSC_paladin_broken_shore@@'):
    if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        if struct.unpack_from('<Q', native['binary'], native['offset'](address) + 0x50)[0] == native['symbol'](callback):
            tables.append(address)
assert len(tables) == 1
dispatch = instructions('?OnGameObjectLootStateChanged@ScriptMgr@@')
loaded = next(index for index, entry in enumerate(dispatch) if entry.op_str == 'r10, qword ptr [rcx + 0x50]')
assert dispatch[loaded + 2].mnemonic == 'call' and dispatch[loaded + 2].op_str == 'r10'
assert native['symbol']('?OnGameObjectLootStateChanged@ScriptMgr@@') in native['calls']('?SetLootState@GameObject@@')
update = instructions('?UpdateCurrentStep@Scenario@@')
reward = next(index for index, entry in enumerate(update) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
              and native['resolve'](entry.operands[0].imm) == native['symbol']('?Reward@Scenario@@'))
assert any(entry.op_str == 'r11b, r9b' for entry in update[reward - 18:reward])
print('PASS selected pickup slot50: original capacity error/reset retained; self180850 remains triggered, now before direct46130/103454 cleanup/Delete. Reported nonzero cast result is preserved in diagnostic and READY1/null-target reset; decoded failure CFG cannot reach those effects. Zero result alone reaches original effects/activated2 Delete; no152/775. OK is prepare admission, NOT delayed199827 storage/equip or whole-client/VFX proof.')

"""Check installed summoner/rider identity gates and native boarding dispatch order."""
from pathlib import Path
import runpy
import struct
from capstone import Cs, CS_ARCH_X86, CS_MODE_64, CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_truthguard_travel_click_release.py')))
native = previous['native']
instructions = previous['instructions']
suffix = '@npc_sr_argent_hippogryphAI@npc_sr_argent_hippogryph@@'
rider = '?GetRider' + suffix
boarded = '?PassengerBoarded' + suffix
arrived = '?MovementInform' + suffix


def find(body, operand, mnemonic=None):
    return next(index for index, entry in enumerate(body) if entry.op_str == operand and (not mnemonic or entry.mnemonic == mnemonic))


def call_index(body, prefix):
    return next(index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
                and native['resolve'](entry.operands[0].imm) == native['symbol'](prefix))


body = instructions(rider)
for operand in ('eax, 0x19d9e', 'dword ptr [rcx + 0xe0], 0x4c4', 'eax, 0x19edd', 'dword ptr [rcx + 0xe0], 0x5d7', 'byte ptr [rcx + 0x30e8], 1'):
    find(body, operand)
summoner = call_index(body, '?GetSummoner@TempSummon@@')
assert body[summoner + 2].op_str == 'rax, rax' and body[summoner + 3].mnemonic == 'je'
player = find(body, 'byte ptr [rax + 0x16], 4', 'cmp')
assert body[player + 1].mnemonic == 'jne'
vehicle = call_index(body, '?GetVehicleBase@Unit@@')
assert summoner < player < vehicle
assert body[vehicle + 1].op_str == 'rax, qword ptr [rdi + 0x20]' and body[vehicle + 2].mnemonic == 'je'
assert body[vehicle + 3].op_str == 'rbx, rsi' and body[vehicle + 4].op_str == 'rax, rbx'
assert native['symbol']('?GetAnyOwner@Unit@@') not in native['calls'](rider), 'Mutable charmer can redirect the rider'
body = instructions(boarded)
identity = call_index(body, rider)
move = call_index(body, '?MovePath@MotionMaster@@QEAAXI_NMM')
assert find(body, 'r9b, r9b', 'test') < find(body, 'rdx, rdx', 'test') < identity < move
assert body[identity + 1].op_str == 'rbx, rax' and body[identity + 2].mnemonic == 'jne'
assert body[identity + 2].operands[0].imm > body[move].address
assert find(body, 'edx, eax, 0x64', 'imul') < move and body[move - 1].op_str == 'r8d, r8d'
body = instructions(arrived)
identity = call_index(body, rider)
assert find(body, 'edx, 2', 'cmp') < find(body, 'r8d, 4', 'cmp') < identity
assert body[identity + 2].op_str == 'rax, rax' and body[identity + 3].mnemonic == 'je'
reject = body[identity + 3].operands[0].imm
assert not any(entry.mnemonic == 'call' for entry in body if entry.address >= reject), 'Missing rider reaches endpoint side effects'
for operand in ('word ptr [rbx + 0x6ba8], 0x43a', 'r8d, 0x333f4', 'r8d, 0x335b8', 'edx, 0x1f4'):
    find(body, operand)
assert native['symbol']('?GetAnyOwner@Unit@@') not in native['calls'](arrived)
tables = []
for entry in instructions('?GetAI@npc_sr_argent_hippogryph@@'):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            slots = struct.unpack_from('<80Q', native['binary'], native['offset'](entry.address + entry.size + operand.mem.disp))
            if native['symbol'](boarded) in slots:
                tables.append(slots)
assert len(tables) == 1 and tables[0][0x210 // 8] == native['symbol'](boarded) and tables[0][0x1a8 // 8] == native['symbol'](arrived)
join = instructions('?Execute@VehicleJoinEvent@@')
store = find(join, 'qword ptr [rcx + 0x30d8], rax', 'mov')
callback = next(index for index, entry in enumerate(join) if entry.mnemonic == 'call' and entry.op_str == 'qword ptr [rax + 0x210]')
assert store < callback and join[store - 2].op_str == 'rcx, qword ptr [rdi + 0x50]' and join[store - 1].op_str == 'rax, qword ptr [rdi + 0x40]'
assert join[callback - 1].op_str == 'rdx, qword ptr [rdi + 0x50]'
leaf_address = native['symbol']('?GetVehicleBase@Unit@@')
decoder = Cs(CS_ARCH_X86, CS_MODE_64)
leaf = list(decoder.disasm(native['binary'][native['offset'](leaf_address):native['offset'](leaf_address) + 32], leaf_address))
assert leaf[0].op_str == 'rcx, qword ptr [rcx + 0x30d8]' and leaf[1].op_str == 'rcx, rcx'
assert leaf[2].mnemonic == 'jne' and int(leaf[2].op_str, 16) == native['symbol']('?GetBase@Vehicle@@')
waypoint = instructions('?OnArrived@?$WaypointMovementGenerator@VCreature@@@@')
waypoint_slot = find(waypoint, 'r9, qword ptr [r8 + 0x1a8]', 'mov')
assert [entry.op_str for entry in waypoint[waypoint_slot - 1:waypoint_slot + 4]] == [
    'r8, qword ptr [rcx]', 'r9, qword ptr [r8 + 0x1a8]', 'r8d, dword ptr [r15]', 'edx, 2', 'r9'], 'Native waypoint does not dispatch selected MovementInform/current node'
print('PASS: installed selected boarding/waypoint callbacks require the original resolved Player summoner aboard the matching bird/map; native vehicle pointer is assigned before boarding dispatch. Original paths, scenario1082, endpoint spell IDs and500ms despawn retained. Cast success, exactly-once flight and client remain separate.')

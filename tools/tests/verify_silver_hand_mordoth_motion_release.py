"""Verify retained SmartAI combat/home dispatch beside the data-only spawn repair."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_silver_hand_initial_motion_release.py')))
native, instructions, sites = previous['native'], previous['instructions'], previous['sites']
attack_name = '?AttackStart@SmartAI@@'
home_name = '?JustReachedHome@SmartAI@@'
body = instructions(attack_name)
attack = sites(body, '?Attack@Unit@@')
chase, = sites(body, '?MoveChase@MotionMaster@@')
stop, = sites(body, '?StopMoving@Unit@@')
slot, = sites(body, '?GetMotionSlotType@MotionMaster@@')
assert len(attack) == 2 and attack[0] < attack[1] < slot < stop < chase
assert body[attack[0]].mnemonic == 'jmp' and body[attack[1]].mnemonic == 'call'
flags, = sites(body, '?HasFlag@Object@@')
assert any(entry.op_str == 'r8d, 0x1000000' for entry in body[:flags])
assert any(entry.op_str == 'edx, 0x61' for entry in body[:flags])
assert body[attack[1] + 1].op_str == 'al, al' and body[attack[1] + 2].mnemonic == 'je'
assert body[attack[1] + 2].operands[0].imm > body[chase].address
combat = next(index for index, entry in enumerate(body) if entry.op_str == 'byte ptr [rdi + 0x1dc], 0')
assert body[combat + 4].mnemonic == 'je' and body[combat + 4].operands[0].imm > body[chase].address
assert any(entry.op_str == 'edx, 1' for entry in body[combat:slot])
assert any(entry.op_str == 'al, 2' for entry in body[slot:stop]) and any(entry.op_str == 'al, 8' for entry in body[slot:stop])

body = instructions(home_name)
event, = sites(body, '?ProcessEventsFor@SmartScript@@')
path, = sites(body, '?MovePath@MotionMaster@@QEAAXI_NMM')
resume, = sites(body, '?ResumeMovement@Unit@@')
idle, = sites(body, '?MoveIdle@MotionMaster@@')
assert event < idle < path < resume
assert any(entry.op_str == 'edx, 0x15' for entry in body[:event])
addon = next(index for index, entry in enumerate(body) if entry.op_str == 'edx, dword ptr [rcx + 0x3b20]')
assert body[addon + 1].op_str == 'edx, edx' and body[addon + 2].mnemonic == 'je'
assert body[addon + 2].operands[0].imm > body[path].address and body[addon + 2].operands[0].imm < body[resume].address
assert any(entry.op_str == 'r8b, 1' for entry in body[addon:path])

constructor = instructions('??0SmartAI@@')
assert any(entry.op_str == 'word ptr [rsi + 0x1db], 0x101' for entry in constructor)
tables = []
for entry in constructor:
    if entry.mnemonic != 'lea' or len(entry.operands) != 2 or entry.operands[1].type != CS_OP_MEM or entry.operands[1].mem.base != X86_REG_RIP:
        continue
    address = entry.address + entry.size + entry.operands[1].mem.disp
    slots = struct.unpack_from('<80Q', native['binary'], native['offset'](address))
    if native['symbol'](attack_name) in slots and native['symbol'](home_name) in slots:
        tables.append(address)
assert len(tables) == 1
print('PASS canonical SmartAI constructor table retains AttackStart/home callbacks, combat-movement default, controlled/null/failed-attack gates, active-slot chase and zero-addon home resume. Static-spawn data only; no live combat/VFX certification.')

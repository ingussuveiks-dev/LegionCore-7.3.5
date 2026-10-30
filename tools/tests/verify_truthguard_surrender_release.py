"""Verify installed Yrgrim final-damage protection and actual selected vtable."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native = previous['native']
instructions = previous['instructions']
post = '?DamageTakenAfterScaling@npc_sr_yrgrim_the_truthseekerAI@'
factory = '?GetAI@npc_sr_yrgrim_the_truthseeker@@'
body = instructions(post)


def positions(dependency):
    target = native['symbol'](dependency)
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call'
            and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == target]


health = positions('?GetHealth@Unit@@QEBA_KPEAV1@@')[0]
assert not any(entry.operands and entry.operands[0].type == CS_OP_REG
               and entry.reg_name(entry.operands[0].reg) in ('rdx', 'edx', 'dx', 'dl')
               for entry in body[:health]), 'Target-relative health lost incoming attacker context'
assert body[health + 1].mnemonic == 'cmp' and body[health + 1].op_str == 'rbx, rax'
assert body[health + 2].mnemonic == 'jae', 'Final lethal damage does not reach surrender'
zero = next(index for index, entry in enumerate(body) if entry.mnemonic == 'xor' and entry.op_str == 'ebp, ebp')
assert body[health + 2].operands[0].imm == body[zero].address
assert body[zero + 1].op_str == 'dword ptr [rsi], ebp', 'Damage must be zeroed before side effects'
assert body[zero + 2].op_str == 'byte ptr [rdi + 0xca], bpl' and body[zero + 3].mnemonic == 'jne', 'Exactly-once claim gate missing'
claim = next(index for index, entry in enumerate(body) if entry.op_str == 'byte ptr [rdi + 0xca], 1')
assert claim > zero and claim < positions('?SetHealth@Unit@@')[0]
assert any(entry.mnemonic == 'cmp' and entry.op_str == 'rbp, rax'
           and body[index + 1].mnemonic == 'jae' and body[index + 1].operands[0].imm == body[zero + 3].operands[0].imm
           for index, entry in enumerate(body[:zero])), 'Nonlethal/non-low-health exit missing'
for dependency in ('?RemoveAllAuras@Unit@@', '?SetHomePosition@Creature@@QEAAXAEBUPosition', '?Reset@EventMap@@', '?Talk@CreatureAI@@QEAAXE', '?ScheduleEvent@EventMap@@QEAAXIIGG'):
    assert positions(dependency), f'Missing original surrender side effect: {dependency}'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'dl, 2' for entry in body), 'Native surrender Talk2 changed'
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM
           and entry.operands[0].mem.disp == 0x120 for entry in body[claim:]), 'Evade dispatch missing'
assert any(entry.op_str == 'r8d, 0x382' for entry in body), 'Original immunity/nonattackability flags missing'
schedule = positions('?ScheduleEvent@EventMap@@QEAAXIIGG')[0]
assert body[schedule - 1].op_str == 'r8d, 0xbb8'
assert any(entry.op_str == 'edx, 1' for entry in body[schedule - 5:schedule]), 'Original event1/3000ms handoff missing'
assert not positions('?DoUpdateAchievementCriteria@InstanceScript@@'), 'Damage hook must not directly grant progression'

tables = []
for entry in instructions(factory):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            offset = native['offset'](entry.address + entry.size + operand.mem.disp)
            if struct.unpack_from('<Q', native['binary'], offset + 0xa8)[0] == native['symbol'](post):
                tables.append(offset)
assert len(tables) == 1, 'Selected Yrgrim factory does not install final-damage slot0xa8'
raw = struct.unpack_from('<Q', native['binary'], tables[0] + 0xa0)[0]
assert raw == native['symbol']('?DamageTaken@UnitAI@@'), 'Old raw-damage surrender still installed'
raw_offset = native['offset'](raw)
assert native['binary'][raw_offset] == 0xc3 or native['binary'][raw_offset:raw_offset + 3] == b'\xc2\x00\x00', 'Inherited raw callback is not the empty native hook'
assert b'npc_sr_yrgrim_the_truthseeker' in native['binary'], 'Yrgrim registration missing'

runpy.run_path(str(Path(__file__).with_name('damage_scaling_release_check.py')))['check'](native, instructions)
print('PASS: deployed Yrgrim factory selects slot0xa8 after native scaling; attacker-relative health, zero-before-once-only claim, original heal/aura/home/Talk2/evade/flags/event1 in3000ms retained. No direct damage credit; not client traversal.')

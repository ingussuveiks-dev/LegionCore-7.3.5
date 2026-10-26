"""Verify selected Balnazzar final scaled lethal protection and finale."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM, CS_OP_REG
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_truthguard_surrender_release.py')))
native, instructions = previous['native'], previous['instructions']
scope = 'npc_bs_balnazzarAI@npc_bs_balnazzar@@'
post = '?DamageTakenAfterScaling@' + scope
factory = instructions('?GetAI@npc_bs_balnazzar@@')
tables = []
for entry in factory:
    if entry.mnemonic == 'lea' and entry.operands[1].type == CS_OP_MEM and entry.operands[1].mem.base == X86_REG_RIP:
        address = entry.address + entry.size + entry.operands[1].mem.disp
        slots = struct.unpack_from('<80Q', native['binary'], native['offset'](address))
        if slots[0xa8 // 8] == native['symbol'](post):
            tables.append(slots)
assert len(tables) == 1 and tables[0][0xa0 // 8] == native['symbol']('?DamageTaken@UnitAI@@')
body = instructions(post)


def sites(prefix):
    target = native['symbol'](prefix)
    return [index for index, entry in enumerate(body) if entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_IMM
            and native['resolve'](entry.operands[0].imm) == target]


health, = sites('?GetHealth@Unit@@QEBA_KPEAV1@@')
assert not any(entry.operands and entry.operands[0].type == CS_OP_REG and entry.reg_name(entry.operands[0].reg) in ('rdx', 'edx', 'dx', 'dl') for entry in body[:health])
assert [entry.op_str for entry in body[health + 1:health + 3]] == ['ebp, ebp', 'rbx, rax']
assert body[health + 3].mnemonic == 'jb' and body[health + 4].op_str == 'dword ptr [rsi], ebp'
below, = sites('?HealthBelowPct@Unit@@')
assert body[health + 3].operands[0].imm == body[below - 2].address and body[below - 1].op_str == 'edx, 0xf'
claim_gate, = [index for index, entry in enumerate(body) if entry.op_str == 'byte ptr [rdi + 0xc9], bpl']
assert body[health + 5].mnemonic == 'jmp' and body[health + 5].operands[0].imm == body[claim_gate].address
assert body[below + 1].op_str == 'al, al' and body[below + 2].mnemonic == 'je'
assert body[claim_gate + 1].mnemonic == 'jne' and body[claim_gate + 1].operands[0].imm == body[below + 2].operands[0].imm
casts = sites('?DoCast@UnitAI@@QEAAXPEAVUnit@@I_N@Z')
assert len(casts) == 2
claim, = [index for index, entry in enumerate(body) if entry.op_str == 'byte ptr [rdi + 0xc9], 1']
assert claim_gate < claim < casts[0] < casts[1]
assert any(entry.op_str == 'r8d, 0x37a0d' for entry in body[claim_gate:casts[0]])
assert any(entry.op_str == 'r8d, 0x254f5' for entry in body[casts[0]:casts[1]])
for cast in casts:
    assert any(entry.op_str == 'r9b, 1' for entry in body[cast - 6:cast])
assert sites('?Reset@EventMap@@') and sites('?DespawnAll@SummonList@@') and sites('?StopAttack@Creature@@')
assert any(entry.op_str == 'r8d, 0x2000180' for entry in body)
assert sites('?SendPlaySpellVisualKit@Unit@@') and any(entry.op_str == 'edx, 0xee6c' for entry in body)
stage, = sites('?getScenarionStep@InstanceScript@@')
credit, = sites('?DoUpdateAchievementCriteria@InstanceScript@@')
assert casts[1] < stage < credit
assert body[stage + 1].op_str == 'eax, 5' and body[stage + 2].mnemonic == 'jne' and body[stage + 2].operands[0].imm == body[credit + 1].address
assert all(any(entry.op_str == operands for entry in body[stage:credit]) for operands in ('edx, 0x5c', 'r8d, 0xb445'))
assert any(entry.op_str == 'edx, 0xbb8' for entry in body[credit:])
print('PASS selected90981 factory postslotA8/raw native empty A0, incoming attacker-relative health/final lethal zero before once-onlyC9 claim, original strict15pct alternative and227853/152821/kit61036/cleanup/flags/stage5 type92 event46149/3000ms retained. Shared actual primary+suspended dispatch follows scaling. Not actual area-credit eligibility, accepted effects or client combat/VFX proof.')

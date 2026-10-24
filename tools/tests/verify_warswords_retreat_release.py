"""Verify actual first Vigfus scaled retreat and its native movement-state handoff."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP

previous = runpy.run_path(str(Path(__file__).with_name('verify_warswords_final_credit_release.py')))
native = previous['native']
instructions = previous['instructions']
suffix = '@npc_vigfus_bladewindAI@npc_vigfus_bladewind@@'
post = '?DamageTakenAfterScaling' + suffix
pre = '?DamageTaken' + suffix
move = '?MovementInform' + suffix
reset = '?Reset' + suffix
factory = '?GetAI@npc_vigfus_bladewind@@'
body = instructions(post)
values = {operand.imm for entry in body for operand in entry.operands if operand.type == CS_OP_IMM}
assert {98007, 1511, 944, 4, 5, 40043, 46982, 50} <= values, 'Wrong native retreat scope/credit'
dependencies = ('?GetScenario@ScenarioMgr@@', '?GetScenarioId@Scenario@@', '?GetCurrentStep@Scenario@@',
                '?GetHealth@Unit@@QEBA_KPEAV1@@', '?CountPctFromMaxHealth@Unit@@', '?SetHealth@Unit@@',
                '?GetCharmerOrOwnerPlayerOrPlayerItself@Unit@@', '?CanContact@Player@@', '?GetQuestStatus@Player@@',
                '?GetQuestRewardStatus@Player@@', '?IsWithinDistInMap@WorldObject@@', '?UpdateAchievementCriteria@Player@@',
                '?MovePoint@MotionMaster@@', '?Talk@CreatureAI@@QEAAXE', '?SetReactState@Creature@@')
for dependency in dependencies:
    assert native['symbol'](dependency) in native['calls'](post), f'Missing actual retreat dependency: {dependency}'


def calls_to(prefix, dependency):
    target = native['symbol'](dependency)
    return [index for index, entry in enumerate(instructions(prefix)) if entry.mnemonic in ('call', 'jmp')
            and entry.operands[0].type == CS_OP_IMM and native['resolve'](entry.operands[0].imm) == target]


health = calls_to(post, '?GetHealth@Unit@@QEBA_KPEAV1@@')[0]
assert body[health + 3].mnemonic == 'cmp' and body[health + 3].op_str == 'rcx, rax'
assert body[health + 4].mnemonic == 'jae', 'Lethal final damage bypasses retreat'
thresholds = calls_to(post, '?CountPctFromMaxHealth@Unit@@')
assert len(thresholds) == 2
assert any(entry.op_str == 'r8, rbp' for entry in body[thresholds[0] - 5:thresholds[0]]), 'Projected threshold lost actual attacker context'
assert any(entry.mnemonic == 'sub' and entry.op_str == 'rdi, rcx' for entry in body[health:thresholds[0]]), 'Projected remaining health is not computed'
assert body[thresholds[0] + 1].op_str == 'rdi, rax' and body[thresholds[0] + 2].mnemonic == 'jae', 'Strict below-half boundary changed'
assert any(entry.op_str == 'r8d, r8d' and entry.mnemonic == 'xor' for entry in body[thresholds[1] - 5:thresholds[1]]), 'Raw-health clamp uses scaled max instead of template max'
set_health = calls_to(post, '?SetHealth@Unit@@')[0]
assert any(entry.mnemonic == 'jbe' for entry in body[thresholds[1]:set_health]), 'Retreat clamp can heal an already lower actor'
claim = next(entry for entry in body if entry.mnemonic == 'mov' and entry.op_str == 'byte ptr [rbx + 0xc4], 1')
assert claim.address < body[calls_to(post, '?UpdateAchievementCriteria@Player@@')[0]].address, 'Native progression precedes exactly-once claim'
assert sum(entry.mnemonic == 'mov' and entry.op_str == 'dword ptr [rsi], 0' for entry in body) == 2, 'Pending and crossed-threshold damage must both be zeroed'
tables = []
for entry in instructions(factory):
    for operand in entry.operands:
        if entry.mnemonic == 'lea' and operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
            table = native['offset'](entry.address + entry.size + operand.mem.disp)
            if struct.unpack_from('<Q', native['binary'], table + 0xa8)[0] == native['symbol'](post):
                tables.append(table)
assert len(tables) == 1, 'Actual first-boss factory omits post-scaled policy'
assert struct.unpack_from('<Q', native['binary'], tables[0] + 0xa0)[0] == native['symbol'](pre), 'Native second-leg damage callback replaced'
assert native['symbol']('?UpdateAchievementCriteria@Player@@') not in native['calls'](pre), 'First retreat still advances before scaling'
movement = instructions(move)
for displacement, branch in ((0xc4, 'je'), (0xc5, 'jne'), (0xc6, 'jne')):
    position = next(index for index, entry in enumerate(movement) if entry.mnemonic == 'cmp'
                    and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == displacement)
    assert movement[position + 1].mnemonic == branch, 'Arrival/pending/second-leg guard changed'
arrival_id = next(index for index, entry in enumerate(movement) if entry.mnemonic == 'cmp' and entry.op_str == 'r8d, 1')
assert movement[arrival_id + 1].mnemonic == 'jne', 'Native arrival ID must equal1'
armed = next(entry for entry in movement if entry.mnemonic == 'mov' and entry.op_str == 'byte ptr [rcx + 0xc5], r8b')
assert not any(entry.operands and entry.operands[0].type != CS_OP_MEM and entry.op_str.split(',')[0] in ('r8', 'r8d', 'r8w', 'r8b')
               and entry.mnemonic not in ('cmp', 'test') for entry in movement[arrival_id + 1:] if entry.address < armed.address), 'Validated arrival1 register changed before pending store'
assert armed.address < movement[calls_to(move, '?DoCast@UnitAI@@QEAAXI@Z')[0]].address, 'Pending arrival claim must precede reentrant channel cast'
assert any(entry.mnemonic == 'mov' and entry.op_str == 'word ptr [rcx + 0xc4], 0' for entry in instructions(reset))
assert any(entry.mnemonic == 'mov' and entry.op_str == 'byte ptr [rcx + 0xc6], 0' for entry in instructions(reset))
assert any(entry.mnemonic == 'mov' and entry.op_str == 'word ptr [rcx + 0xc5], 0x100' for entry in instructions(pre)), 'Second leg does not clear pending and claim started together'
damage = instructions('?DealDamage@Unit@@')
division = next(index for index, entry in enumerate(damage) if entry.mnemonic == 'divss' and entry.op_str == 'xmm1, xmm0')
assert any(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0x180
           for entry in damage[division - 8:division]), 'Actual native multiplier call missing'
assert sum(entry.mnemonic == 'call' and entry.operands[0].type == CS_OP_MEM and entry.operands[0].mem.disp == 0xa8
           for entry in damage[division + 3:division + 70]) == 2, 'Current/suspended AIs do not receive final damage'
print('PASS: deployed first98007/1511/944/4 retreat uses final damage and attacker-relative half-health, no-heal raw clamp, eligible owner and once state; native second-leg/reset/vtable and both post-scaling dispatches retained. No client visual/path proof.')

"""Bind idle-or-completed-original-route approach recovery to the selected Release controller."""
from pathlib import Path
import runpy
import struct
from capstone import CS_OP_IMM, CS_OP_MEM
from capstone.x86_const import X86_REG_RIP


def check(native, instructions):
    suffix = '@npc_hrutnirAI@npc_hrutnir@@'

    def calls(body, prefix):
        targets = {native['resolve'](address) for name, address in native['symbols'].items() if name.startswith(prefix)}
        return [i for i, op in enumerate(body) if op.mnemonic in ('call', 'jmp') and op.operands[0].type == CS_OP_IMM and native['resolve'](op.operands[0].imm) in targets]

    def values(body):
        return {operand.imm for op in body for operand in op.operands if operand.type == CS_OP_IMM}

    selection = instructions('?sGossipSelect' + suffix)
    assert calls(selection, '?ScheduleMagnarApproach' + suffix)[0] < calls(selection, '?MovePoint@MotionMaster@@')[0], 'Initial request is not armed before movement callbacks'
    schedule = instructions('?ScheduleMagnarApproach' + suffix)
    assert calls(schedule, '?AddDelayedEvent@FunctionProcessor@@') and 1000 in values(schedule)
    assert any('0x36a0' in op.op_str for op in schedule), 'Retry must use the actual Unit queue'
    callbacks = [name for name in native['symbols'] if name.startswith('?_Do_call@') and '??ScheduleMagnarApproach' + suffix in name]
    assert len(callbacks) == 1
    bridge = instructions(callbacks[0])
    operators = [name for name in native['symbols'] if name.startswith(('??$invoke@', '??R<lambda_1>')) and '??ScheduleMagnarApproach' + suffix in name]
    if not calls(bridge, '?RetryMagnarApproach' + suffix):
        assert len(operators) == 1 and calls(bridge, operators[0]), 'Installed bridge does not reach its actual weak operator'
        bridge += instructions(operators[0])
    assert calls(bridge, '?RetryMagnarApproach' + suffix)
    assert any(op.mnemonic == 'lock cmpxchg' for op in bridge), 'Retry lost weak ownership'
    retry = instructions('?RetryMagnarApproach' + suffix)
    for dependency in ('?HasMagnarChamberRequest' + suffix, '?GetPlayer@ObjectAccessor@@', '?CanContact@Player@@',
                       '?GetQuestCancellationReceipt@Player@@', '?GetQuestStatus@Player@@', '?GetQuestRewardStatus@Player@@',
                       '?IsWithinDistInMap@WorldObject@@', '?IsMovementPreventedByCasting@Unit@@',
                       '?GetCurrentMovementGeneratorType@MotionMaster@@', '?ScheduleMagnarApproach' + suffix,
                       '?CancelMagnarStart' + suffix, '?MovePoint@MotionMaster@@'):
        assert calls(retry, dependency), 'Missing selected recovery endpoint: ' + dependency
    move = calls(retry, '?MovePoint@MotionMaster@@')
    assert len(move) == 1
    assert {96468, 1495, 909, 3, 39191, 7, 9859004} <= values(retry)
    def dominates(candidate, target):
        addresses = {op.address: i for i, op in enumerate(retry)}
        pending, seen = [0], set()
        while pending:
            i = pending.pop()
            if i == candidate or i in seen:
                continue
            if i == target:
                return False
            seen.add(i)
            op = retry[i]
            if op.mnemonic == 'ret':
                continue
            if op.mnemonic.startswith('j'):
                if op.operands[0].type == CS_OP_IMM and op.operands[0].imm in addresses:
                    pending.append(addresses[op.operands[0].imm])
                if op.mnemonic == 'jmp':
                    continue
            if i + 1 < len(retry):
                pending.append(i + 1)
        return True
    assert any(dominates(at, move[0]) for at in calls(retry, '?ScheduleMagnarApproach' + suffix)), 'Movement is reachable without first arming its next retry'
    assert calls(retry, '?GetCurrentMovementGeneratorType@MotionMaster@@')[0] < move[0]
    constants = set()
    for op in retry[max(0, move[0] - 20):move[0]]:
        for operand in op.operands:
            if operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
                offset = native['offset'](op.address + op.size + operand.mem.disp)
                constants.add(native['binary'][offset:offset + 4])
    assert {struct.pack('<f', value) for value in (4720.80, 347.91, -37.71)} <= constants
    assert any(op.op_str == 'byte ptr [rsp + 0x28], 1' for op in retry[max(0, move[0] - 20):move[0]]), 'Recovery bypasses ordinary native path generation'
    for dependency in ('?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@', '?AddItem@Player@@',
                       '?CompleteQuest@Player@@', '?TeleportTo@Player@@', '?NearTeleportTo@',
                       '?StartMagnarConversation' + suffix, '?MovementInform' + suffix, '?TryMagnarStart' + suffix):
        assert not calls(retry, dependency), 'Movement retry fabricates arrival, presentation or progression'
    print('PASS selected Hruthnir original-owner/quest-receipt/context/idle-or-completed-original-route native MovePoint7 recovery, actual weak Unit1000ms queue armed before movement callbacks; no synthetic arrival/story/credit/teleport. Native stopped-point and source lifetime/deferral cases remain separate; full client path pending.')


if __name__ == '__main__':
    previous = runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_cleanup_release.py')))
    check(previous['native'], previous['instructions'])

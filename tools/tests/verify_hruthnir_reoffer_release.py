"""Bind retryable ready-offer routing to the selected native controller."""
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

    update = instructions('?UpdateAI' + suffix)
    assert calls(update, '?UpdateMagnarReoffer' + suffix)[0] < calls(update, '?TryPillikEntry' + suffix)[0] < calls(update, '?UpdateVictim@CreatureAI@@')[0]
    offer = instructions('?UpdateMagnarReoffer' + suffix)
    for dependency in ('?IsMagnarOfferContext' + suffix, '?HasMagnarChamberRequest' + suffix,
                       '?IsMagnarIntroPlayer' + suffix,
                       '?FindNearestCreature@WorldObject@@', '?IsMovementPreventedByCasting@Unit@@',
                       '?GetCurrentMovementGeneratorType@MotionMaster@@', '?RemoveFlag@Object@@', '?MovePoint@MotionMaster@@'):
        assert calls(offer, dependency), 'Missing selected offer guard/endpoint: ' + dependency
    assert {1000, 96034, 1065, 1062, 9859004, 6} <= values(offer)
    receipt = [i for i, op in enumerate(offer) if op.mnemonic == 'call' and op.op_str == 'qword ptr [rax + 0x50]']
    assert len(receipt) == 1 and offer[receipt[0] - 1].op_str == 'edx, 0x426', 'Reoffer ignores actual Magnar acknowledgement'
    move = calls(offer, '?MovePoint@MotionMaster@@')
    assert len(move) == 1
    # This predicate is inlined in the selected build. Retain its actual XYZ
    # subtract/square/add/radius flow and early exit for an undisplaced offer.
    comparison = next(i for i, op in enumerate(offer) if op.mnemonic == 'comiss')
    start = max(i for i, op in enumerate(offer[:comparison]) if op.op_str == 'rax, qword ptr [rcx + 0x20]')
    shape = [('mov', 'rax, qword ptr [rcx + 0x20]'),
             ('movss', 'xmm0, dword ptr [rax + 0xd0]'), ('subss', None),
             ('movss', 'xmm1, dword ptr [rax + 0xd4]'), ('subss', None),
             ('movss', 'xmm2, dword ptr [rax + 0xd8]'), ('subss', 'xmm2, xmm6'),
             ('mulss', 'xmm0, xmm0'), ('mulss', 'xmm1, xmm1'), ('mulss', 'xmm2, xmm2'),
             ('addss', 'xmm1, xmm0'), ('movss', None), ('addss', 'xmm1, xmm2'), ('comiss', 'xmm0, xmm1')]
    body = offer[start:comparison + 1]
    assert len(body) == len(shape)
    for op, (mnemonic, operand) in zip(body, shape):
        assert op.mnemonic == mnemonic and (operand is None or op.op_str == operand)
    def constant(op, value):
        operand = op.operands[1]
        assert operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP
        offset = native['offset'](op.address + op.size + operand.mem.disp)
        assert native['binary'][offset:offset + 4] == struct.pack('<f', value)
    for index, value in ((2, 4763.73), (4, 340.87), (11, 4.0)):
        constant(body[index], value)
    constant(next(op for op in offer[:start] if op.mnemonic == 'movss' and op.op_str.startswith('xmm6,')), -37.19)
    assert offer[comparison + 1].mnemonic == 'jae' and offer[comparison + 1].operands[0].imm > offer[move[0]].address
    constants = set()
    for op in offer[max(0, move[0] - 24):move[0]]:
        for operand in op.operands:
            if operand.type == CS_OP_MEM and operand.mem.base == X86_REG_RIP:
                offset = native['offset'](op.address + op.size + operand.mem.disp)
                constants.add(native['binary'][offset:offset + 4])
    assert {struct.pack('<f', value) for value in (4763.73, 340.87)} <= constants
    window = offer[max(0, move[0] - 24):move[0]]
    assert any(op.op_str == 'dword ptr [rsp + 0x20], xmm6' for op in window), 'Native Z is not the verified retained -37.19 register'
    # XMM6 is nonvolatile; its other local writes are terminal restorations.
    for i, op in enumerate(offer[:move[0]]):
        if op.mnemonic == 'movaps' and op.op_str.startswith('xmm6,'):
            assert any(end.mnemonic == 'ret' for end in offer[i:i + 6])
    assert any(op.mnemonic == 'mov' and op.op_str == 'r14d, 1' for op in offer[:move[0]])
    assert all(op.op_str == 'r14d, 1' for op in offer[:move[0]] if op.mnemonic == 'mov' and op.op_str.split(',')[0] in ('r14', 'r14d', 'r14b'))
    assert any(op.op_str == 'byte ptr [rsp + 0x28], r14b' for op in window), 'Offer retry bypasses native pathfinding'
    for dependency in ('?UpdateAchievementCriteria@Player@@', '?KilledMonsterCredit@Player@@', '?CompleteQuest@Player@@',
                       '?AddItem@Player@@', '?TeleportTo@Player@@', '?NearTeleportTo@', '?Talk@CreatureAI@@',
                       '?StartMagnarConversation' + suffix, '?MovementInform' + suffix, '?DoAction'):
        assert not calls(offer, dependency), 'Offer recovery fabricates dialogue, arrival or progression'
    assert calls(instructions('?MovementInform' + suffix), '?AddDelayedEvent@FunctionProcessor@@'), 'Actual arrival lost original offer clock'
    print('PASS selected ready-offer retry: current chamber/epoch/original actor/eligible participant/native boss receipt,1000ms before victim gate,actual original point6/pathfinding endpoint and retired displaced offer; no direct speech/story/arrival/credit/item/teleport grant. Original full13s/6s and actual native reset/reaccept remain separate checks; client UI/full chain pending.')


if __name__ == '__main__':
    previous = runpy.run_path(str(Path(__file__).with_name('verify_hruthnir_cleanup_release.py')))
    check(previous['native'], previous['instructions'])

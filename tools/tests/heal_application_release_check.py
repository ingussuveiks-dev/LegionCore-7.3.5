"""Prove native heal callbacks precede actual bounded health application."""
from capstone import CS_OP_IMM, CS_OP_MEM

def check(native, instructions):
    body = instructions('?DealHeal@Unit@@')
    def sites(prefix):
        targets = {native['resolve'](value) for name, value in native['symbols'].items() if name.startswith(prefix)}
        return [i for i, op in enumerate(body) if op.mnemonic in ('call', 'jmp') and op.operands[0].type == CS_OP_IMM and native['resolve'](op.operands[0].imm) in targets]
    received, = [i for i, op in enumerate(body) if op.mnemonic == 'call' and op.operands[0].type == CS_OP_MEM and op.operands[0].mem.disp == 0xb0]
    done, = [i for i, op in enumerate(body) if op.mnemonic == 'call' and op.operands[0].type == CS_OP_MEM and op.operands[0].mem.disp == 0xb8]
    legacy = sites('?ModifyHealth@Unit@@')
    if legacy:
        applied, = legacy
        assert received < done < applied
        return applied
    health, = sites('?GetHealth@Unit@@QEBA_KPEAV1@@')
    maximum, = sites('?GetMaxHealth@Unit@@QEBA_KPEAV1@@')
    applied, = sites('?SetHealthScal@Unit@@')
    notified, = sites('?NotifyHealApplied64@Creature@@')
    assert received < done < health < maximum < applied < notified
    # Both virtual hooks receive the same actual mutable uint64 healing amount.
    assert any(op.mnemonic == 'lea' and op.op_str == 'r8, [rbp - 0x59]' for op in body[received-4:received])
    assert any(op.mnemonic == 'lea' and op.op_str == 'r8, [rbp - 0x59]' for op in body[done-4:done])
    assert body[done+1].mnemonic == 'mov' and body[done+1].op_str == 'r8, qword ptr [rbp - 0x59]'
    pairs = {(op.mnemonic, op.op_str) for op in body[maximum:applied]}
    assert {('sub', 'rcx, rbx'), ('cmp', 'rax, rbx'), ('cmovbe', 'rcx, r13'),
            ('cmp', 'rcx, r14'), ('cmovb', 'r14, rcx'), ('lea', 'rdx, [r14 + rbx]')} <= pairs, 'Actual uint64 missing-health bound and sum must precede SetHealthScal'
    assert any(op.mnemonic == 'mov' and op.op_str == 'r9, r14' for op in body[applied:notified]), 'Applied-heal notification receives the actual bounded uint64 gain'
    print('PASS actual native HealReceived64 -> HealDone64 -> bounded AddHealth/SetHealthScal -> NotifyHealApplied64 order; no signed32 application assumption')
    return applied

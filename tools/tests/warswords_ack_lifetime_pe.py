"""Compose weak AI lifetime with the later real native channel acknowledgement."""
from pathlib import Path
import runpy
from capstone import CS_OP_IMM, CS_OP_MEM
from linked_player_abi import unit_queue_offset


def guards(p):
    n, ins, reachable = p['native'], p['instructions'], p['reachable']
    scope = '@' + p['actor']
    outer, inner = ins(p['outer']), ins(p['callback'])
    def sites(body, prefix):
        targets = {n['resolve'](value) for name, value in n['symbols'].items() if name.startswith(prefix)}
        return [i for i, x in enumerate(body) if x.mnemonic in ('call', 'jmp')
                and x.operands[0].type == CS_OP_IMM and n['resolve'](x.operands[0].imm) in targets]
    checked = []
    def guard(body, start, ctx, control, owner, epoch, capture, count_zero, pending_zero):
        window = body[start:start+36]
        load = next(i for i, x in enumerate(window) if x.mnemonic == 'mov'
                    and x.op_str == f'{control}, qword ptr [{ctx} + 0x10]')
        tested = load + 1
        if window[tested].mnemonic == 'mov' and window[tested].op_str == 'rdi, rcx':
            tested += 1
        assert (window[tested].mnemonic, window[tested].op_str) == ('test', f'{control}, {control}')
        assert window[tested+1].mnemonic == 'je'
        assert (window[tested+2].mnemonic, window[tested+2].op_str) == ('cmp', f'dword ptr [{control} + 8], {count_zero}')
        assert window[tested+3].mnemonic == 'je'
        owner_at = next(i for i in range(tested+4, len(window)) if window[i].mnemonic == 'mov'
                        and window[i].op_str == f'{owner}, qword ptr [{ctx}]')
        epoch_at = next(i for i in range(owner_at+1, owner_at+8) if window[i].mnemonic == 'mov'
                        and window[i].op_str == f'{epoch}, dword ptr [{owner} + 0xc8]')
        assert (window[epoch_at+1].mnemonic, window[epoch_at+1].op_str) == ('cmp', f'dword ptr [{capture}], {epoch}')
        assert window[epoch_at+2].mnemonic == 'jne'
        pending = next(i for i in range(epoch_at+3, epoch_at+6) if window[i].mnemonic == 'cmp'
                       and window[i].op_str == f'byte ptr [{owner} + 0xc7], {pending_zero}')
        assert window[pending+1].mnemonic == 'je'
        forbidden = {body[i].address for prefix in (
            '?IsIntroTarget'+scope, '?IsIntroHelya'+scope, '?GetPlayer@ObjectAccessor@@',
            '?GetCreature@ObjectAccessor@@', '?CastSpell@Unit@@', '?RemoveAura@Unit@@',
            '?UpdateBlessing'+scope, '?CancelIntro'+scope, '?RemoveIntroConversation'+scope)
                     for i in sites(body, prefix)}
        forbidden |= {x.address for x in body if x.mnemonic == 'call' and
                      x.operands[0].type == CS_OP_MEM and x.operands[0].mem.disp == 0x10}
        forbidden |= {window[owner_at].address, window[epoch_at].address}
        for at in (tested+1, tested+3, epoch_at+2, pending+1):
            branch = window[at]
            assert not forbidden & reachable(body, branch.operands[0].imm), 'Expired/stale/inactive AI reaches a dependent call'
            checked.append((p['outer'] if body is outer else p['callback'], branch.address))
    cast, = sites(outer, '?CastSpell@Unit@@')
    zeros = [i for i, x in enumerate(outer[:cast]) if x.mnemonic == 'xor' and x.op_str == 'ebx, ebx']
    assert len(zeros) == 1, 'Weak count comparison lacks its unique zero initialization'
    zero, = zeros
    assert all(x.reg_name(reg) not in ('rbx', 'ebx', 'bx', 'bl', 'bh')
               for x in outer[zero+1:cast] for reg in x.regs_access()[1]), 'Weak count comparison lost its preserved zero'
    removal, = sites(inner, '?RemoveAura@Unit@@')
    guard(outer, 0, 'rcx', 'rax', 'rcx', 'eax', 'rsi', '0', '0')
    guard(outer, cast+1, 'rdi', 'rcx', 'rdx', 'ecx', 'rsi', 'ebx', 'bl')
    guard(inner, 0, 'rcx', 'rax', 'rcx', 'eax', 'rdi + 0x18', '0', '0')
    guard(inner, removal+1, 'rdi', 'rax', 'rcx', 'eax', 'rdi + 0x18', '0', '0')
    assert len(checked) == 16
    return checked, sites


def verify(p):
    checked, sites = guards(p)
    n, ins = p['native'], p['instructions']
    initial, outer = ins(p['opening']), ins(p['outer'])
    pairs = lambda rows: {(x.mnemonic, x.op_str) for x in rows}
    assert {('movups', 'xmmword ptr [rax + 0x24], xmm7'),
            ('movups', 'xmmword ptr [rax + 0x34], xmm8'),
            ('mov', 'rbx, qword ptr [rdi + 0x120]'),
            ('mov', 'r14, qword ptr [rdi + 0x118]')} <= pairs(initial)
    queued, = sites(initial, '?AddDelayedEvent@FunctionProcessor@@')
    assert ('mov', 'edx, 0x5dc0') in pairs(initial[queued-5:queued])
    ack = ins('?UpdateBlessing@' + p['actor'])
    release, = sites(ack, '?AddDelayedEvent@FunctionProcessor@@')
    assert ('mov', 'edx, 0x1b58') in pairs(ack[release-5:release])
    offset = unit_queue_offset()
    for body, at in ((initial, queued), (ack, release)):
        assert any(o.type == CS_OP_MEM and o.mem.disp == offset or o.type == CS_OP_IMM and o.imm == offset
                   for x in body[at-8:at] for o in x.operands), 'Timer lost its native Unit queue'
    # This independent retained certificate verifies actual pending-channel
    # acknowledgement, complete GUIDs, once-only aura release and cancellation.
    acknowledged = runpy.run_path(str(Path(__file__).with_name('verify_warswords_helya_ack_release.py')))
    assert acknowledged['n']['binary'] == n['binary']
    print('PASS current queued-channel lifetime:four weak guards/16 refusal edges,original full identities,native24s+acknowledged7s queues and complete retained native acknowledgement certificate;client separate')
    return {'lifetime_verified': True, 'body': outer, 'sites': sites}

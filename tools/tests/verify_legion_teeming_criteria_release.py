"""Check both inlined effective-step paths in the actual final x64 PE."""
from pathlib import Path
import runpy

from sharasdal_erunak_entry_pe import sites

n = runpy.run_path(str(Path(__file__).with_name('verify_smart_linked_cast_retry_release.py')))
getter = n['instructions']('?GetScenarioCriteriaByStep@Scenario@@')
update = n['instructions']('?UpdateCurrentStep@Scenario@@')


def effective_order(body, zero, fallback):
    affix, = sites(n, body, '?HasAffix@Challenge@@')
    variant, = sites(n, body, '?HasAffixesTeeming@ScenarioMgr@@')
    assert affix < variant
    assert body[affix - 1].mnemonic == 'mov' and body[affix - 1].op_str == 'edx, 5'
    assert any(x.mnemonic == 'test' and x.op_str == 'rcx, rcx' for x in body[affix - 4:affix])
    assert any(x.mnemonic == 'movzx' and x.op_str == 'edx, word ptr [rdi + 0x1c]'
               for x in body[affix:variant])
    test, reject, clear, jump, original = body[variant + 1:variant + 6]
    assert test.mnemonic == 'test' and test.op_str == 'al, al'
    assert reject.mnemonic == 'je' and reject.operands[0].imm == original.address
    assert clear.mnemonic == 'xor' and clear.op_str == zero
    assert jump.mnemonic == 'jmp' and jump.operands[0].imm == body[variant + 6].address
    assert original.mnemonic == 'movzx' and original.op_str == fallback
    # Missing challenge and nonTeeming keys use the same immutable DB2 order.
    assert sum(x.mnemonic == 'je' and x.operands[0].imm == original.address
               for x in body[affix - 3:variant]) == 2
    return variant


lookup = effective_order(getter, 'al, al', 'eax, byte ptr [rdi + 0x1a]')
assert getter[lookup + 6].mnemonic == 'cmp' and getter[lookup + 6].op_str == 'r14b, al'
progress = effective_order(update, 'dil, dil', 'edi, byte ptr [rdi + 0x1a]')
assert update[progress + 6].mnemonic == 'cmp' and update[progress + 6].op_str == 'byte ptr [rbx + 0x270], dil'
complete, = sites(n, update, '?IsCompletedScenarioTree@')
assert progress < complete
assert any(x.mnemonic == 'inc' and x.op_str == 'dil' for x in update[complete:complete + 6])
assert any(x.mnemonic == 'mov' and x.op_str == 'byte ptr [rbx + 0x270], dil'
           for x in update[complete:complete + 9])
assert sites(n, update, '?GetScenarioCriteriaByStep@Scenario@@')
print('PASS linked Teeming criterion order: actual key5+recognized native variant gates,immutable DB2 fallback,effective zero in lookup and native completion increment')
